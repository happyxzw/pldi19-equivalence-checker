// Copyright 2013-2019 Stanford University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>

#include "src/cfg/cfg.h"
#include "src/cfg/paths.h"
#include "src/validator/handlers/combo_handler.h"
#include "src/validator/filters/default.h"
#include "src/validator/filters/forbidden_dereference.h"
#include "src/validator/obligation_checker.h"
#include "src/validator/smt_obligation_checker.h"
#include "src/validator/invariants/false.h"
#include "src/validator/invariants/true.h"

#include "src/ext/cpputil/include/command_line/command_line.h"
#include "src/ext/cpputil/include/signal/debug_handler.h"
#include "src/ext/cpputil/include/io/filterstream.h"
#include "src/ext/cpputil/include/io/column.h"
#include "src/ext/cpputil/include/io/console.h"

#include "tools/gadgets/cost_function.h"
#include "tools/gadgets/functions.h"
#include "tools/gadgets/sandbox.h"
#include "tools/gadgets/seed.h"
#include "tools/gadgets/solver.h"
#include "tools/gadgets/target.h"
#include "tools/gadgets/testcases.h"
#include "tools/io/state_diff.h"

#define DEBUG(X) { }

using namespace cpputil;
using namespace std;
using namespace stoke;
using namespace x64asm;

auto& bound_arg =
  ValueArg<size_t>::create("bound")
  .usage("<int>")
  .description("Bound on loop iterations")
  .default_val(1);

auto& debug_arg = FlagArg::create("debug")
                  .description("Print some information about what we're doing");

auto& mutants_arg = ValueArg<size_t>::create("mutants")
                    .description("For each testcase generated by SMT solver, create this many more with a search")
                    .default_val(5);

auto& iterations_arg = ValueArg<size_t>::create("iterations")
                       .description("Number of iterations for mutation")
                       .default_val(1000);

auto& output_arg = ValueArg<string>::create("output")
                   .description("file to write testcases");

auto& stop_at = ValueArg<size_t>::create("max_tcs")
                .description("once this many testcases are generated, stop")
                .default_val(0);

auto& randomize_order_arg = FlagArg::create("randomize_order")
                            .description("Output test cases in random order");

typedef struct {
  unsigned long size,resident,share,text,lib,data,dt;
} statm_t;

CpuState debug_bad_output;

// source: https://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c
void read_memory_status(statm_t& result)
{
  unsigned long dummy;
  const char* statm_path = "/proc/self/statm";

  FILE *f = fopen(statm_path,"r");
  if (!f) {
    perror(statm_path);
    abort();
  }
  if (7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
                  &result.size,&result.resident,&result.share,&result.text,&result.lib,&result.data,&result.dt)) {
    perror(statm_path);
    abort();
  }
  fclose(f);
}

/** Get a vector of all non-empty memory segments for a testcase */
vector<Memory*> get_segments(CpuState& cs) {

  vector<Memory*> segments;
  if (cs.heap.size())
    segments.push_back(&cs.heap);
  if (cs.stack.size())
    segments.push_back(&cs.stack);
  if (cs.data.size())
    segments.push_back(&cs.data);
  for (auto& it : cs.segments)
    if (it.size())
      segments.push_back(&it);

  return segments;
}

/** Make sure that a testcase is valid for the program */
bool check_testcase(CpuState cs, Sandbox& sb) {

  sb.clear_inputs();
  sb.insert_input(cs);
  sb.run(0);
  auto code = sb.get_output(0)->code;
  if (code != ErrorCode::NORMAL) {
    debug_bad_output = *sb.get_output(0);
  }

  return code == ErrorCode::NORMAL;
}

/** Make a different testcase for the program. */
CpuState mutate(CpuState cs, size_t iterations,
                Sandbox& sb, default_random_engine& gen) {

  // Run iterations
  for (size_t i = 0; i < iterations; ++i) {

    CpuState candidate = cs;

    switch (gen() % 2) {
    case 0: {
      // Mutate one byte of one gp register
      size_t reg_choice = gen() % 16;
      candidate.gp[reg_choice].get_fixed_byte(gen() % 8) ^= (int8_t)(gen() % 256);
      break;
    }

    case 1: {
      // Mutate one byte of memory
      auto segments = get_segments(candidate);
      if (segments.size()) {
        auto memory = segments[gen() % segments.size()];
        auto addr = (gen() % memory->size()) + memory->lower_bound();
        memory->set_valid(addr, true);
        (*memory)[addr] ^= (int8_t)(gen() % 256);
      }
      break;
    }
    }

    // Test
    if (check_testcase(candidate, sb))
      cs = candidate;
  }

  return cs;
}

void make_tc_different_memory(
  SmtObligationChecker& checker,
  const Cfg& target,
  const Cfg& rewrite,
  vector<CpuState>& outputs, CpuState tc,
  CfgPath p,
  CfgPath rewrite_path,
  Sandbox& sb,
  default_random_engine& gen) {
// Now, lets find another testcase that touches *different* memory.
  ComboHandler handler;
  auto _false = make_shared<FalseInvariant>();
  auto _true = make_shared<TrueInvariant>();

  auto segments = get_segments(tc);
  if (segments.size()) {

    vector<uint64_t> low;
    vector<uint64_t> high;

    for (auto segment : segments) {
      low.push_back(segment->lower_bound());

      // watch for overflow!
      if (segment->upper_bound() == 0)
        high.push_back((uint64_t)(-1));
      else
        high.push_back(segment->upper_bound());
    }

    ForbiddenDereferenceFilter filter(handler, low, high);
    SmtObligationChecker oc(checker.get_solver(), filter);
    oc.set_check_counterexamples(false);
    oc.set_alias_strategy(ObligationChecker::AliasStrategy::FLAT);
    oc.set_separate_stack(false);

    vector<pair<CpuState, CpuState>> testcases;
    auto result = oc.check_wait(target, rewrite, target.get_entry(), rewrite.get_entry(), p, rewrite_path, _true, _false, testcases, false);

    if (result.has_ceg) {
      auto tc2 = result.target_ceg;

      if (!check_testcase(tc2, sb)) {
        cerr << "Warning: skipping over invalid testcase." << endl;
        return;
      }

      outputs.push_back(tc2);
      for (size_t i = 0; i < mutants_arg.value(); ++i) {
        auto mutated = mutate(tc2, iterations_arg.value(), sb, gen);
        outputs.push_back(mutated);
      }
    }
  }
}

void fix_cache_lines(CpuState& tc, default_random_engine& gen) {
  auto segments = tc.get_segments();
  for (auto segment : segments) {
    for (uint64_t i = 0; i < (uint64_t)segment->size(); ++i) {
      uint64_t addr = segment->lower_bound() + i;
      if (segment->is_valid(addr)) {
        uint64_t low =  addr & 0xffffffffffffff00;
        uint64_t high = low+15;
        assert(low < high);
        for (uint64_t fix = low; low <= fix && fix <= high; fix++) {
          if (!segment->is_valid(addr)) {
            segment->set_valid(addr, true);
            (*segment)[addr] = gen() % 256;
          }
        }
      }
    }
  }
}

int main(int argc, char** argv) {

  statm_t memory_usage;

  CommandLineConfig::strict_with_convenience(argc, argv);

  FunctionsGadget aux_fxns;
  TargetGadget target(aux_fxns, false);

  CpuStates empty_set;
  SandboxGadget sb(empty_set, aux_fxns);

  // Setup sandbox
  sb.reset();
  sb.insert_function(target);
  sb.set_entrypoint(target.get_function().get_leading_label());

  SeedGadget seed;
  default_random_engine gen;
  gen.seed((default_random_engine::result_type)seed);

  SolverGadget solver;
  ComboHandler handler;
  DefaultFilter filter(handler);
  SmtObligationChecker checker(solver, filter);
  checker.set_check_counterexamples(false);
  checker.set_alias_strategy(ObligationChecker::AliasStrategy::FLAT);
  checker.set_separate_stack(false);

  // Step 1: enumerate paths up to a certain bound
  vector<CfgPath> paths;
  paths = CfgPaths::enumerate_paths(target, bound_arg.value());

  // Handle the shorter paths first
  auto by_length = [](const CfgPath& lhs, const CfgPath& rhs) {
    return lhs.size() < rhs.size();
  };
  sort(paths.begin(), paths.end(), by_length);

  if (debug_arg.value())
    cerr << "Number of paths: " << paths.size() << endl;

  // Step 2: for each path, find a testcase if possible
  // (there's lots of silly setup for this)

  x64asm::Code rewrite_code;
  std::stringstream ss;
  ss << ".silly:" << std::endl;
  ss << "retq" << std::endl;
  ss >> rewrite_code;
  Cfg rewrite(rewrite_code, x64asm::RegSet::all_gps(), x64asm::RegSet::empty());
  auto rewrite_path = CfgPaths::enumerate_paths(rewrite, 1)[0];


  auto _false = make_shared<FalseInvariant>();
  auto _true = make_shared<TrueInvariant>();

  size_t found = 0;

  CpuStates outputs;
  for (auto p : paths) {

    cout << "Working on path " << p << endl;
    DEBUG(cout << "Output argument: " << output_arg.value() << endl;)
    DEBUG(cout << "Output count: " << outputs.size() << endl;)

    found = outputs.size();
    if (stop_at.value() && found > stop_at.value()) {
      return 0;
    }

    if (debug_arg.value()) {
      cerr << "Looking for testcase on path " << p << endl;
    }

    vector<pair<CpuState, CpuState>> testcases;
    auto result = checker.check_wait(target, rewrite, target.get_entry(), rewrite.get_entry(), p, rewrite_path, _true, _false, testcases, false);

    if (result.has_ceg) {
      auto tc = result.target_ceg;

      if (!check_testcase(tc, sb)) {
        cerr << "Warning: skipping over invalid (original) testcase" << endl;
        cerr << tc << endl;
        cerr << "Output state" << endl;
        cerr << debug_bad_output << endl;
        continue;
      }

      outputs.push_back(tc);
      if (debug_arg.value()) {
        cerr << " * Found testcase" << endl;
      }

      /** Change some register values. */
      for (size_t i = 0; i < mutants_arg.value(); ++i) {
        auto mutated = mutate(tc, iterations_arg.value(), sb, gen);
        outputs.push_back(mutated);
      }

      /** Use SMT Solver to make yet another testcase with different memory. */
      make_tc_different_memory(checker, target, rewrite, outputs, tc, p, rewrite_path, sb, gen);

    } else {
      if (debug_arg.value())
        cerr << " * No testcase found" << endl;
    }
  }

  // Go through final testcases and mark memory locations in the same cache line
  // (e.g. 16-byte chunk) as valid.
  for (auto& tc : outputs) {
    fix_cache_lines(tc, gen);
  }

  DEBUG(cout << "Output argument: " << output_arg.value() << endl;)
  DEBUG(cout << "Output count: " << outputs.size() << endl;)

  if (randomize_order_arg.value()) {
    random_shuffle(outputs.begin(), outputs.end());
  }

  // Print anything we have so far
  if (outputs.size() && output_arg.value() == "") {
    outputs.write_text(cout);
  } else if (outputs.size()) {
    ofstream ofs(output_arg.value(), ios_base::app);
    outputs.write_text(ofs);
  }

  return 0;
}



