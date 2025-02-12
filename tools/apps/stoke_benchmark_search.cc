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

#include <chrono>
#include <iostream>

#include "src/ext/cpputil/include/command_line/command_line.h"
#include "src/ext/cpputil/include/io/console.h"
#include "src/ext/cpputil/include/signal/debug_handler.h"

#include "tools/args/benchmark.inc"
#include "tools/gadgets/functions.h"
#include "tools/gadgets/seed.h"
#include "tools/gadgets/target.h"
#include "tools/gadgets/transform_pools.h"
#include "tools/gadgets/weighted_transform.h"

using namespace cpputil;
using namespace std;
using namespace std::chrono;
using namespace stoke;

int main(int argc, char** argv) {
  CommandLineConfig::strict_with_convenience(argc, argv);
  DebugHandler::install_sigsegv();
  DebugHandler::install_sigill();

  SeedGadget seed;
  FunctionsGadget aux_fxns;
  TargetGadget target(aux_fxns, false);
  TransformPoolsGadget transform_pools(target, aux_fxns, seed);
  WeightedTransformGadget transform(transform_pools, seed);

  Console::msg() << "Transforms::modify()..." << endl;

  const auto start = steady_clock::now();
  for (size_t i = 0; i < benchmark_itr_arg; ++i) {
    transform(target);
  }
  const auto dur = duration_cast<duration<double>>(steady_clock::now() - start);
  const auto mps = benchmark_itr_arg / dur.count();

  Console::msg() << fixed;
  Console::msg() << "Runtime:    " << dur.count() << " seconds" << endl;
  Console::msg() << "Throughput: " << mps << " / second" << endl;

  return 0;
}
