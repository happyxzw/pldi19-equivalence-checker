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


#ifndef _STOKE_SRC_SOLVER_CVC4SOLVER_H
#define _STOKE_SRC_SOLVER_CVC4SOLVER_H

#include <map>

#include "cvc4/cvc4.h"
#include "src/solver/smtsolver.h"
#include "src/symstate/bitvector.h"
#include "src/symstate/memo_visitor.h"

namespace stoke {

class Cvc4Solver : public SMTSolver {

public:
  Cvc4Solver() : SMTSolver(), smt_(NULL), em_(), uninterpreted_(false) {
    smt_ = new CVC4::SmtEngine(&em_);
    init();
  }

  Cvc4Solver(const Cvc4Solver& s) : SMTSolver(), smt_(NULL), em_(), uninterpreted_(false) {
    init();
    set_timeout(s.get_timeout());
  }

  Cvc4Solver& operator=(const Cvc4Solver& s) {
    set_timeout(s.get_timeout());
    return* this;
  }

  ~Cvc4Solver() {
    delete smt_;
  }

  SMTSolver& set_timeout(uint64_t ms) {
    timeout_ = ms;
    smt_->setTimeLimit(timeout_, true);
    return *this;
  }

  /** Check if a query is satisfiable given constraints */
  bool is_sat(const std::vector<SymBool>& constraints);

  /** Check if a satisfying assignment is available. */
  bool has_model() const {
    return !uninterpreted_;
  }
  /** Get the satisfying assignment for a bit-vector from the model. */
  cpputil::BitVector get_model_bv(const std::string& var, uint16_t octs);
  /** Get the satisfying assignment for a bit from the model. */
  bool get_model_bool(const std::string& var);

  std::pair<std::map<uint64_t, cpputil::BitVector>, uint8_t> get_model_array(const std::string& var, uint16_t key_bits, uint16_t value_bits);

  void reset() {
    variables_ = std::map<std::string, CVC4::Expr>();
    uninterpreted_ = false;
  }

  virtual void interrupt() {
    stop_now_.store(true);
    try {
      smt_->interrupt();
      //std::cout << "CVC4 interrupted" << std::endl;
    } catch (std::exception e) {

    }
  }

  SMTSolver* clone() const {
    return new Cvc4Solver(*this);
  }

  virtual Solver get_enum() {
    return Solver::CVC4;
  }

private:

  Cvc4Solver& init();

  CVC4::SmtEngine* smt_;
  CVC4::ExprManager em_;
  bool uninterpreted_;
  /** Tracks variable expressions already created for re-use */
  std::map<std::string, CVC4::Expr> variables_;


  /** This class converts symbolic bit-vectors into Z3's format. */
  /** NOTE: turned off memoization due to bound variables, which changes things;
    a single AST node may need multiple different translations. */
  class ExprConverter : public SymVisitor<CVC4::Expr, CVC4::Expr, CVC4::Expr> {

  public:
    ExprConverter(Cvc4Solver* parent) :
      em_(parent->em_),
      variables_(parent->variables_),
      bound_variables_(),
      uninterpreted_(&(parent->uninterpreted_)) {}

    CVC4::Expr operator()(const SymBool& b) {
      return SymVisitor<CVC4::Expr, CVC4::Expr, CVC4::Expr>::operator()(b.ptr);
    }
    CVC4::Expr operator()(const SymBitVector& bb) {
      return SymVisitor<CVC4::Expr, CVC4::Expr, CVC4::Expr>::operator()(bb.ptr);
    }
    CVC4::Expr operator()(const SymArray& a) {
      return SymVisitor<CVC4::Expr, CVC4::Expr, CVC4::Expr>::operator()(a.ptr);
    }

    /** Visit bit-vector binop */
    CVC4::Expr visit_binop(const SymBitVectorBinop * const bv);
    /** Visit bool binop */
    CVC4::Expr visit_binop(const SymBoolBinop * const bv);
    /** Visit bit-vector unop */
    CVC4::Expr visit_unop(const SymBitVectorUnop * const bv);
    /** Visit bit-vector compare */
    CVC4::Expr visit_compare(const SymBoolCompare * const bv);

    /** Visit a bit-vector array lookup */
    CVC4::Expr visit(const SymBitVectorArrayLookup * const bv);
    /** Visit a bit-vector constant */
    CVC4::Expr visit(const SymBitVectorConstant * const bv);
    /** Visit a bit-vector extract */
    CVC4::Expr visit(const SymBitVectorExtract * const bv);
    /** Visit a bit-vector function */
    CVC4::Expr visit(const SymBitVectorFunction * const bv);
    /** Visit a bit-vector if-then-else */
    CVC4::Expr visit(const SymBitVectorIte * const bv);
    /** Visit a bit-vector sign-extend */
    CVC4::Expr visit(const SymBitVectorSignExtend * const bv);
    /** Visit a bit-vector variable */
    CVC4::Expr visit(const SymBitVectorVar * const bv);

    /** Visit a boolean ARRAY_EQ */
    CVC4::Expr visit(const SymBoolArrayEq * const b);
    /** Visit a boolean FALSE */
    CVC4::Expr visit(const SymBoolFalse * const b);
    /** Visit a boolean FOR_ALL */
    CVC4::Expr visit(const SymBoolForAll * const b);
    /** Visit a boolean NOT */
    CVC4::Expr visit(const SymBoolNot * const b);
    /** Visit a boolean TRUE */
    CVC4::Expr visit(const SymBoolTrue * const b);
    /** Visit a boolean VAR */
    CVC4::Expr visit(const SymBoolVar * const b);

    /** Visit an array STORE */
    CVC4::Expr visit(const SymArrayStore * const a);
    /** Visit an array VAR */
    CVC4::Expr visit(const SymArrayVar * const a);


    /** See if there's an error */
    bool has_error() {
      return error_.size() > 0;
    }
    /** Get the error */
    std::string error() {
      return error_;
    }

  private:

    /** Reference to parent expression manager */
    CVC4::ExprManager& em_;
    /** Reference to variable table */
    std::map<std::string, CVC4::Expr>& variables_;
    std::map<std::string, CVC4::Expr> bound_variables_;
    bool* uninterpreted_;

    /** Keep error messages */
    std::string error_;

    /** Logic to instantiate a new function variable */
    CVC4::Expr build_function(const SymBitVectorFunction * const);
    /** Helper function for rotate */
    CVC4::Expr generic_rot(bool left, const SymBitVectorAbstract * const, CVC4::Expr, CVC4::Expr);
  };


};

} //namespace stoke

#endif
