// Copyright 2013-2019 Stanford University
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


#ifndef STOKE_SRC_SYMSTATE_MEMORY_FLAT_H
#define STOKE_SRC_SYMSTATE_MEMORY_FLAT_H

#include <map>

#include "src/symstate/bitvector.h"
#include "src/symstate/memory.h"
#include "src/symstate/memory/stack.h"

namespace stoke {

/** Models memory as a giant array */
class FlatMemory : public SymMemory {

public:

  FlatMemory(bool separate_stack, bool no_constraints = false) : SymMemory(separate_stack) {
    variable_ = SymArray::tmp_var(64, 8);
    start_variable_ = variable_;
    heap_ = variable_;
    variable_up_to_date_ = true;
    no_constraints_ = no_constraints;

  }

  FlatMemory(FlatMemory& other) : SymMemory(other.separate_stack_) {
    variable_ = other.variable_;
    start_variable_ = other.start_variable_;
    heap_ = other.heap_;
    stack_ = other.stack_;
    variable_up_to_date_ = other.variable_up_to_date_;
    no_constraints_ = other.no_constraints_;
  }

  /** Updates the memory with a write.
   *  Returns condition for segmentation fault */
  SymBool write(SymBitVector address, SymBitVector value, uint16_t size, DereferenceInfo info);

  /** Reads from the memory.  Returns value and segv condition. */
  std::pair<SymBitVector,SymBool> read(SymBitVector address, uint16_t size, DereferenceInfo info);

  /** Create a formula expressing these memory cells with another set. */
  SymBool equality_constraint(FlatMemory& other);

  std::vector<SymBool> get_constraints() {
    std::vector<SymBool> output = constraints_;
    auto stack_constraints = stack_.get_constraints();
    output.insert(output.begin(), stack_constraints.begin(), stack_constraints.end());
    return output;
  }

  /** Get a variable representing the memory at this state. */
  SymArray get_variable() {
    if (!variable_up_to_date_) {
      variable_ = SymArray::tmp_var(64, 8);
      variable_up_to_date_ = true;
      constraints_.push_back(variable_ == heap_);
    }

    return variable_;
  }

  SymArray get_start_variable() {
    return start_variable_;
  }

  std::vector<SymArray> get_stack_start_variables() const {
    return stack_.get_start_variables();
  }

  std::vector<SymArray> get_stack_end_variables() const {
    return stack_.get_end_variables();
  }

  /** Get list of accesses accessed (via read or write).  This is needed for
   * marking relevant cells valid in the counterexample. */
  std::map<const SymBitVectorAbstract*, uint64_t> get_access_list() {
    return access_list_;
  }

  /** The heap state */
  SymArray heap_;
  /** The stack state */
  StackMemory stack_;
  /** Extra constraints needed to make everything work. */
  std::vector<SymBool> constraints_;

private:

  /** A variable that represents the heap state */
  bool variable_up_to_date_;
  SymArray variable_;
  SymArray start_variable_;

  /** map of (symbolic address, size) pairs accessed. */
  std::map<const SymBitVectorAbstract*, uint64_t> access_list_;

  bool no_constraints_;

};

};

#endif
