// Copyright 2014 eric schkufza
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

#ifndef STOKE_TOOLS_GADGETS_TARGET_H
#define STOKE_TOOLS_GADGETS_TARGET_H

#include "src/cfg/cfg.h"
#include "tools/args/target.h"

namespace stoke {

class TargetGadget : public Cfg {
public:
  TargetGadget() : Cfg(target_arg.value().code, def_in_arg, live_out_arg) {
    for (const auto& fxn : aux_fxns_arg.value()) {
      auto code = fxn.code;
      auto lbl = code[0].get_operand<x64asm::Label>(0);
      add_summary(lbl,
                  code.must_read_set(),
                  code.must_write_set(),
                  code.must_undef_set(),
                  code.maybe_read_set(),
                  code.maybe_write_set(),
                  code.maybe_undef_set());
    }
  }
};

} // namespace stoke

#endif