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

#ifndef STOKE_TOOLS_GADGETS_LATENCY_COST_H
#define STOKE_TOOLS_GADGETS_LATENCY_COST_H

#include "src/cost/latency.h"
#include "tools/args/latency.inc"
#include "tools/args/cost.inc"

namespace stoke {

class LatencyCostGadget : public LatencyCost {
public:
  LatencyCostGadget() : LatencyCost() {
  }
};

} // namespace stoke

#endif
