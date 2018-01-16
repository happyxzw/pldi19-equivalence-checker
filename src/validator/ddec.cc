// Copyright 2013-2016 Stanford University
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/cfg/dominators.h"
#include "src/cfg/paths.h"
#include "src/cfg/sccs.h"
#include "src/validator/bounded.h"
#include "src/validator/data_collector.h"
#include "src/validator/dual.h"
#include "src/validator/ddec.h"
#include "src/validator/flow_invariant_learner.h"
#include "src/validator/indexer.h"
#include "src/validator/null.h"
#include "src/validator/invariants/conjunction.h"
#include "src/validator/invariants/disjunction.h"
#include "src/validator/invariants/equality.h"
#include "src/validator/invariants/false.h"
#include "src/validator/invariants/flag.h"
#include "src/validator/invariants/implication.h"
#include "src/validator/invariants/memory_equality.h"
#include "src/validator/invariants/no_signals.h"
#include "src/validator/invariants/nonzero.h"
#include "src/validator/invariants/sign.h"
#include "src/validator/invariants/state_equality.h"
#include "src/validator/invariants/true.h"

#include <algorithm>
#include <set>

// this is configurable via build system
#ifdef STOKE_DEBUG_DDEC
#define DDEC_DEBUG(X) { X }
#else
#define DDEC_DEBUG(X) {   }
#endif

#define DDEC_TC_DEBUG(X) { }

using namespace std;
using namespace stoke;
using namespace x64asm;

Instruction get_last_instr(const Cfg& cfg, Cfg::id_type block) {
  auto start_bs = cfg.num_instrs(block);
  return cfg.get_code()[cfg.get_index(Cfg::loc_type(block, start_bs - 1))];
}

/** Returns an invariant representing the fact that the first state transition in the path is taken. */
Invariant* get_jump_inv(const Cfg& cfg, Cfg::id_type sb, const CfgPath& p, bool is_rewrite) {
  auto jump_type = ObligationChecker::is_jump(cfg, sb, p, 0);

  if (jump_type == ObligationChecker::JumpType::NONE) {
    return new TrueInvariant();
  }

  auto start_block = p[0];
  if (p.size() == 1)
    start_block = sb;
  auto start_bs = cfg.num_instrs(start_block);
  assert(start_bs > 0);
  auto jump_instr = cfg.get_code()[cfg.get_index(Cfg::loc_type(start_block, start_bs - 1))];

  if (!jump_instr.is_jcc()) {
    return new TrueInvariant();
  }

  bool is_fallthrough = jump_type == ObligationChecker::JumpType::FALL_THROUGH;
  auto jump_inv = new FlagInvariant(jump_instr, is_rewrite, is_fallthrough);
  return jump_inv;
}


// takes conjunction of the form (A1 and A2 ... Ak) and returns one of form
// ((B => A1) and (B => A2) ... (B => Ak))
ConjunctionInvariant* transform_with_assumption(Invariant* assume, ConjunctionInvariant* conjunction) {

  ConjunctionInvariant* output = new ConjunctionInvariant();

  for (size_t i = 0; i < conjunction->size(); ++i) {
    output->add_invariant(new ImplicationInvariant(assume, (*conjunction)[i]));
  }

  delete conjunction;
  return output;
}



void DdecValidator::make_tcs(const Cfg& target, const Cfg& rewrite) {

  if (no_bv_) //if not using the bounded validator for testcases, skip this entirely.
    return;

  auto target_paths = CfgPaths::enumerate_paths(target, bound_);
  auto rewrite_paths = CfgPaths::enumerate_paths(rewrite, bound_);

  Code nop_code;
  nop_code.push_back(x64asm::Instruction(x64asm::NOP));
  Cfg nop_cfg(nop_code);
  vector<size_t> empty_path;
  empty_path.push_back(1);

  FalseInvariant _false;
  TrueInvariant _true;

  for (auto p : target_paths) {
    DDEC_DEBUG(cout << "Trying path " << p << " ; on target" << endl;)
    bool equiv = check(target, nop_cfg, target.get_entry(), nop_cfg.get_entry(), p, empty_path, _true, _false);
    if (!equiv && checker_has_ceg()) {
      sandbox_->insert_input(checker_get_target_ceg());
    }
  }
  for (auto p : rewrite_paths) {
    DDEC_DEBUG(cout << "Trying path " << p << " ; on rewrite" << endl;)
    bool equiv = check(rewrite, nop_cfg, rewrite.get_entry(), nop_cfg.get_entry(), p, empty_path, _true, _false);
    if (!equiv && checker_has_ceg()) {
      sandbox_->insert_input(checker_get_target_ceg());
    }
  }

}

template <typename T>
T find_min(vector<T> v) {
  assert(v.size() > 0);
  T x = v[0];
  for (size_t i = 1; i < v.size(); ++i)
    if (v[i] < x)
      x = v[i];
  return x;
};

vector<Cfg::id_type> intersect(std::vector<Cfg::id_type>& s1, std::vector<Cfg::id_type>& s2) {
  cout << "S1: " << s1 << endl;
  cout << "S2: " << s2 << endl;
  vector<Cfg::id_type> output;
  std::sort(s1.begin(), s1.end());
  std::sort(s2.begin(), s2.end());
  set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                   std::inserter(output, output.begin()));
  cout << "S1 intersect S2: " << output << endl;
  return output;
}

vector<Cfg::id_type> dominator_intersect(Cfg& cfg, std::vector<Cfg::id_type>& blocks) {
  CfgDominators dominators(cfg);

  auto all_blocks = blocks;
  for (auto blk : blocks) {
    auto doms = dominators.get_dominators(blk);

    std::vector<Cfg::id_type> dom_vec(doms.begin(), doms.end());
    cout << "Dominators for " << blk << ": " << dom_vec << endl;
    all_blocks = intersect(all_blocks, dom_vec);
  }

  return all_blocks;
}

void DdecValidator::learn_inductive_paths_at_block(
  vector<CfgPath>& target_inductive_paths,
  vector<CfgPath>& rewrite_inductive_paths,
  Cfg::id_type target_block,
  Cfg::id_type rewrite_block
) {

  bool found_pair = false;

  for (size_t k = 0; k < 4; ++k) {
    size_t bound = (1 << k);
    auto target_paths = CfgPaths::enumerate_paths(target_, bound, target_block, target_block);
    auto rewrite_paths = CfgPaths::enumerate_paths(rewrite_, bound, rewrite_block, rewrite_block);

    for (auto& it : target_paths)
      it.pop_back();
    for (auto& it : rewrite_paths)
      it.pop_back();


    /*
    cout << "Target paths for " << target_block << endl;
    for (auto it : target_paths) {
      cout << it << endl;
    }
    cout << "Rewrite paths for " << rewrite_block << endl;
    for (auto it : rewrite_paths) {
      cout << it << endl;
    }
    */

    for (auto tp : target_paths) {
      for (auto rp : rewrite_paths) {
        if (control_learner_->inductive_pair_feasible(tp, rp)) {
          cout << "Found inductive pair " << tp << " and " << rp << endl;
          target_inductive_paths.push_back(tp);
          rewrite_inductive_paths.push_back(rp);
          found_pair = true;
        }
      }
    }

    if (found_pair)
      return;
  }
}

void DdecValidator::learn_inductive_paths(
  vector<CfgPath>& target_inductive_paths,
  vector<CfgPath>& rewrite_inductive_paths
) {
  cout << "============================================================" << endl;
  cout << "Learning inductive paths" << endl;

  // Learn relations over basic blocks
  CfgSccs target_sccs(target_);
  CfgSccs rewrite_sccs(rewrite_);

  // Figure out possible inductive program paths
  size_t target_num_scc = target_sccs.count();
  size_t rewrite_num_scc = rewrite_sccs.count();
  for (size_t i = 0; i < target_num_scc; ++i) {
    auto target_blocks = target_sccs.get_blocks(i);

    for (size_t j = 0; j < rewrite_num_scc; ++j) {
      auto rewrite_blocks = rewrite_sccs.get_blocks(j);

      for (auto& target_block : target_blocks) {
        for (auto& rewrite_block : rewrite_blocks) {
          learn_inductive_paths_at_block(
            target_inductive_paths,
            rewrite_inductive_paths,
            target_block,
            rewrite_block);
        }
      }
    }
  }
}


bool DdecValidator::discharge_exhaustive(DualAutomata& dual, DualAutomata::State state) {
  Invariant* invariant = dual.get_invariant(state);
  auto edges = dual.next_edges(state);
  vector<pair<CfgPath, CfgPath>> path_pairs;
  for (auto edge : edges) {
    path_pairs.push_back(pair<CfgPath,CfgPath>(edge.te, edge.re));
  }
  bool ok = verify_exhaustive(target_, rewrite_,
                              state.ts, state.rs,
                              path_pairs, *invariant);
  if (ok) {
    cout << "VERIFIED EXHAUSTIVE FOR " << state << endl;
    return true;
  } else {
    cout << "NOT EXHAUSTIVE FOR " << state << endl;
    return false;
  }
}

bool DdecValidator::discharge_exhaustive(DualAutomata& dual) {
  bool okay = true;
  for (auto state : dual.get_reachable_states()) {
    if (state == dual.exit_state())
      continue;

    bool try_again = true;
    bool first = true;
    CpuState ceg;

    while (try_again) {
      try_again = false;
      bool current_okay = discharge_exhaustive(dual, state);
      okay &= current_okay;

      cout << "DischargeExhaustive ---------" << endl;
      cout << "   current_okay " << current_okay << endl;
      cout << "   have ceg     " << checker_has_ceg() << endl;

      // Add a path into the dual that was previously missing
      if (!current_okay && checker_has_ceg() && (checker_get_target_ceg() != ceg || first)) {
        cout << "Attemping to add edge:" << endl;
        ceg = checker_get_target_ceg();

        // TODO: shorten the paths considering cutpoints ugh
        auto tt = checker_get_target_exhaustive_ceg();
        auto rt = checker_get_rewrite_exhaustive_ceg();

        auto tp = DataCollector::project_states(tt);
        auto rp = DataCollector::project_states(rt);
        DualAutomata::Edge edge(state, tp, rp);
        bool added = dual.add_edge(edge);
        if (added) {
          try_again = true;
          cout << "Successfully added edge" << endl;
        } else {
          cout << "Adding edge failed" << endl;
        }
      } else {
        if (!checker_has_ceg()) {
          cout << "Could not get counterexample... aborting" << endl;
        } else if (checker_get_target_ceg() == ceg && !first) {
          cout << "Got repeated counterexample... something is wrong!" << endl;
        } else if (current_okay) {
          cout << "That went well... moving on!" << endl;
        }
      }
      first = false;
    }
  }

  return okay;
}

void DdecValidator::discharge_invariants(DualAutomata& dual) {

  // Now we run a fixedpoint algorithm to get the provable invariants
  set<DualAutomata::State> worklist;

  for (auto reachable : dual.get_reachable_states())
    worklist.insert(reachable);

  // TODO: we can make this faster if the worklist contains *edges* rather than
  //   states.
  while (worklist.size()) {
    // Pick a state
    auto current = worklist.begin();

    cout << "WORKLIST CURRENT " << *current << "; LIST=";
    for (auto it : worklist) {
      cout << it;
    }
    cout << endl;

    bool ok = true;
    for (auto edge : dual.next_edges(*current)) {
      // check this edge
      auto start_inv = dual.get_invariant(edge.from);
      auto end_inv = static_cast<ConjunctionInvariant*>(dual.get_invariant(edge.to));

      cout << "_____________________________" << endl;
      cout << "Edge: " << edge.from << " -> " << edge.to << endl;
      cout << "target: ";
      for (auto it : edge.te) {
        cout << it << " ";
      }
      cout << endl;
      cout << "rewrite: ";
      for (auto it : edge.re) {
        cout << it << " ";
      }
      cout << endl;
      cout << "Assuming: " << *start_inv << endl << endl;

      // check the invariants in the conjunction one at a time
      for (size_t i = 0; i < end_inv->size(); ++i) {
        auto partial_inv = (*end_inv)[i];
        cout << "  Proving " << *partial_inv << endl;
        bool valid = false;
        try {
          valid = check(target_, rewrite_, edge.from.ts, edge.from.rs,
                        edge.te, edge.re, *start_inv, *partial_inv);
        } catch (validator_error e) {
          valid = false;
          cout << "   * encountered " << e.what() << "; assuming false.";
        }
        //bool valid = true;
        cout << "    " << (valid ? "true" : "false") << endl;
        if (!valid) {
          ok = false;
          end_inv->remove(i);
          i--;
        }
      }
    }

    if (!ok) {
      // add all successors of 'current' to the worklist
      // remove 'current' from the worklist
      auto tmp = *current;
      worklist.erase(current);
      auto to_add = dual.next_states(tmp);
      worklist.insert(to_add.begin(), to_add.end());
    } else {
      // remove 'current' from the worklist
      worklist.erase(current);
    }
  }
}


bool DdecValidator::verify_dual(DualAutomata& dual) {
  dual.remove_prefixes();
  dual.print_all();
  InvariantLearner learner;
  Sandbox sb(*sandbox_);
  bool learning_successful = dual.learn_invariants(sb, learner);
  if (!learning_successful) {
    cout << "Learning invariants failed!" << endl;
    return false;
  }
  auto start_state = dual.start_state();
  auto end_state = dual.exit_state();
  dual.set_invariant(start_state, get_initial_invariant());
  dual.set_invariant(end_state, get_final_invariant());

  /** Add NoSignals invariant everywhere */
  auto ns_invariant = new NoSignalsInvariant();
  auto reachable = dual.get_reachable_states();
  for (auto rs : reachable) {
    auto orig_inv = dual.get_invariant(rs);
    orig_inv->add_invariant(ns_invariant);
    dual.set_invariant(rs, orig_inv);
  }

  bool valid = false;
  while (!valid) {
    dual.print_all();
    discharge_invariants(dual);
    dual.print_all();

    /** Check if proof succeeds. */
    auto actual_final = dual.get_invariant(end_state);
    auto expected_final = get_final_invariant();

    bool final_ok = check(target_, rewrite_, end_state.ts, end_state.rs,
                          {}, {}, *actual_final, *expected_final);
    if (!final_ok) {
      cout << "Could not complete final proof step." << endl;
      cout << "Maybe DDEC missed an important invariant?" << endl;
      return false;
    }

    cout << "Verifying exhaustive" << endl;
    auto old_edge_count = dual.count_edges();
    valid = discharge_exhaustive(dual);
    if (!valid) {
      if (old_edge_count == dual.count_edges()) {
        cout << "Couldn't verify exhaustive nor find counterexample (bug?)." << endl;
        return false;
      } else {
        cout << "Couldn't verify exhaustive; going to try again." << endl;
      }
    }
  }

  cout << " ===== PROOF COMPLETE ===== " << endl;
  dual.print_all();
  return true;
};

DualAutomata DdecValidator::learn_inductive_invariants(
  const std::vector<CfgPath>& target_inductive_paths,
  const std::vector<CfgPath>& rewrite_inductive_paths) {

  FlowInvariantLearner fil(*data_collector_, invariant_learner_);
  fil.initialize(target_, rewrite_);

  for (Cfg::id_type i = target_.get_entry(); i != target_.get_exit(); i++) {
    for (Cfg::id_type j = rewrite_.get_entry(); j != rewrite_.get_exit(); j++) {
      cout << "===== INVARIANT AFTER BLOCKS " << i << " / " << j << " =====" << endl;
      auto inv = fil.get_invariant(i,j);
      inv->write_pretty(cout);
      cout << "  == performing transform " << i << "  " << j << " == " << endl;
      std::vector<CfgPath> target_paths;
      std::vector<CfgPath> rewrite_paths;
      for (size_t k = 0; k < target_inductive_paths.size(); ++k) {
        if (target_inductive_paths[k][0] == i && rewrite_inductive_paths[k][0] == j) {
          target_paths.push_back(target_inductive_paths[k]);
          rewrite_paths.push_back(rewrite_inductive_paths[k]);
          cout << "    with paths " << target_inductive_paths[k] << " / " << rewrite_inductive_paths[k] << endl;
        }
      }
      auto inv_trans = fil.transform_invariant(inv, target_paths, rewrite_paths);
      inv_trans->write_pretty(cout);
    }
  }

  DualAutomata dual(&target_, &rewrite_, *data_collector_);

  return dual;
}

bool DdecValidator::verify(const Cfg& init_target, const Cfg& init_rewrite) {

  init_mm();

  auto target = inline_functions(init_target);
  auto rewrite = inline_functions(init_rewrite);
  target_ = target;
  rewrite_ = rewrite;

  DDEC_DEBUG(cout << "INLINED TARGET: " << endl << target.get_code() << endl;)
  DDEC_DEBUG(cout << "INLINED REWRITE: " << endl << rewrite.get_code() << endl;)

  /** Find inductive paths. */
  data_collector_ = new DataCollector(*sandbox_);
  control_learner_ = new ControlLearner(target_, rewrite_, *sandbox_);
  vector<CfgPath> target_inductive_paths;
  vector<CfgPath> rewrite_inductive_paths;
  learn_inductive_paths(target_inductive_paths, rewrite_inductive_paths);

  /** Learn initial inductive invariants. */
  learn_inductive_invariants(target_inductive_paths, rewrite_inductive_paths);

  try {

    sanity_checks(target_, rewrite_);

    DDEC_TC_DEBUG(
      cout << "DDEC sandbox at " << sandbox_ << endl;
    for (size_t i = 0; i < sandbox_->size(); ++i) {
    cout << "DDEC sees this TC: " << endl;
    cout << *sandbox_->get_input(i) << endl;
    }
    )

    return false;

  } catch (validator_error e) {

    has_error_ = true;
    error_ = e.get_message();
    error_file_ = e.get_file();
    error_line_ = e.get_line();
    delete control_learner_;
    reset_mm();
    return false;
  }

  delete control_learner_;
  reset_mm();
  return false;

}




ConjunctionInvariant* DdecValidator::get_initial_invariant() const {
  auto initial_invariant = new ConjunctionInvariant();
  auto sei = new StateEqualityInvariant(target_.def_ins());
  initial_invariant->add_invariant(sei);
  initial_invariant->add_invariant(new MemoryEqualityInvariant());
  initial_invariant->add_invariant(new NoSignalsInvariant());

  /*
    for (auto r : string_params_) {
      // rsi_start = rsi (for example)
      Variable start_var(string_ghost_start(r), false);
      Variable string_reg(r, false);
      string_reg.coefficient = -1;
      auto equiv = new EqualityInvariant({start_var, string_reg}, 0);
      initial_invariant->add_invariant(equiv);

      // rsi_end = 0 (for example)
      Variable end_var(string_ghost_end(r), false);
      auto end_mem = new PointerNullInvariant(end_var, 1);
      initial_invariant->add_invariant(end_mem);
    }
  */

  //initial_invariant->add_invariant(get_fixed_invariant());

  return initial_invariant;
}

ConjunctionInvariant* DdecValidator::get_final_invariant() const {
  auto final_invariant = new ConjunctionInvariant();
  auto sei = new StateEqualityInvariant(target_.live_outs());
  final_invariant->add_invariant(sei);
  final_invariant->add_invariant(new MemoryEqualityInvariant());

  //final_invariant->add_invariant(get_fixed_invariant());

  return final_invariant;
}
