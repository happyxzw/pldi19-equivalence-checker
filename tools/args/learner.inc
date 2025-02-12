// Copyright 2013-2015 Stanford University
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

#ifndef STOKE_TOOLS_ARGS_LEARNER_INC
#define STOKE_TOOLS_ARGS_LEARNER_INC

#include "src/ext/cpputil/include/command_line/command_line.h"

namespace stoke {

cpputil::Heading& learner_heading =
  cpputil::Heading::create("Invariant Learner Options:");

cpputil::FlagArg& only_linear_arg =
  cpputil::FlagArg::create("only_linear")
  .description("Only learn linear invariants");

cpputil::FlagArg& no_learn_memory_arg =
  cpputil::FlagArg::create("only_non_memory")
  .description("Only learn invariants without memory");

cpputil::FlagArg& vector_invariants_arg =
  cpputil::FlagArg::create("vector_invariants")
  .description("Learn invariants over sub-registers");

cpputil::FlagArg& shadow_regs_arg =
  cpputil::FlagArg::create("shadow_registers")
  .description("Learn invariants over shadow registers (basic block counts)");

} // namespace stoke

#endif
