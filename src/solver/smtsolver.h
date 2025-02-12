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


#ifndef _STOKE_SRC_SOLVER_SMTSOLVER_H
#define _STOKE_SRC_SOLVER_SMTSOLVER_H

#include <map>
#include <vector>
#include <atomic>

#include "src/solver/solver.h"
#include "src/ext/cpputil/include/container/bit_vector.h"

namespace stoke {

class SymBool;

class SMTSolver {

public:

  /* Resets the state common to SMT solvers */
  SMTSolver() {
    timeout_ = 0;
    error_ = "";
  }

  virtual ~SMTSolver() {}

  virtual SMTSolver* clone() const = 0;

  /** Set the maximum time to spend solving */
  virtual SMTSolver& set_timeout(uint64_t ms) {
    timeout_ = ms;
    return *this;
  }
  /** Get the timeout last set. */
  virtual uint64_t get_timeout() const {
    return timeout_;
  }

  /** Check if a query is satisfiable given constraints */
  virtual bool is_sat(const std::vector<SymBool>& constraints) = 0;

  /** Check if a satisfying assignment is available. */
  virtual bool has_model() const = 0;
  /** Get the satisfying assignment for a bit-vector from the model.
      For now, one must specify the expected number of octwords. */
  virtual cpputil::BitVector get_model_bv(const std::string& var, uint16_t octs) = 0;
  /** Get the satisfying assignment for a bit from the model. */
  virtual bool get_model_bool(const std::string& var) = 0;
  /** Get the satisfying assignment for an array */
  virtual std::pair<std::map<uint64_t, cpputil::BitVector>,uint8_t> get_model_array(const std::string& var, uint16_t key_bits, uint16_t value_bits) = 0;

  /** Check if the last query trivvered an error. */
  virtual bool has_error() {
    return error_.size() > 0;
  }
  /** Report the last encountered error. */
  virtual std::string get_error() {
    return error_;
  }

  virtual void interrupt() {
    stop_now_.store(true);
  }

  virtual Solver get_enum() {
    return Solver::NONE;
  }

protected:

  std::atomic<bool> stop_now_;

  /** Used to set the timeout before invoking solver */
  uint64_t timeout_;
  /** Current error message */
  std::string error_;

};

} //namespace stoke

#endif
