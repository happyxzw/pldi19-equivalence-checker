/*********************                                                        */
/*! \file sygus_print_callback.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds, Haniel Barbosa
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief sygus print callback functions
 **/

#include <cvc4/cvc4_public.h>

#ifndef __CVC4__PRINTER__SYGUS_PRINT_CALLBACK_H
#define __CVC4__PRINTER__SYGUS_PRINT_CALLBACK_H

#include <vector>

#include <cvc4/expr/datatype.h>
#include <cvc4/expr/expr.h>

namespace CVC4 {
namespace printer {

/** sygus expression constructor printer
 *
 * This class is used for printing sygus datatype constructor terms whose top
 * symbol is an expression, such as a custom defined lambda. For example, for
 * sygus grammar:
 *    A -> (+ x A B) | x | y
 *    B -> 0 | 1
 * The first constructor, call it C_f for A takes two arguments (A, B) and has
 * sygus operator
 *   (lambda ((z Int) (w Int)) (+ x z w))
 * For this operator, we set a print callback that prints, e.g. the term
 *   C_f( t1, t2 )
 * is printed as:
 *   "(+ x [out1] [out2])"
 * where out1 is the result of p->toStreamSygus(out,t1) and
 *       out2 is the result of p->toStreamSygus(out,t2).
 */
class CVC4_PUBLIC SygusExprPrintCallback : public SygusPrintCallback
{
 public:
  SygusExprPrintCallback(Expr body, std::vector<Expr>& args);
  ~SygusExprPrintCallback() {}
  /** print sygus term e on output out using printer p */
  virtual void toStreamSygus(const Printer* p,
                             std::ostream& out,
                             Expr e) const override;

 protected:
  /** body of the sygus term */
  Expr d_body;
  /** arguments */
  std::vector<Expr> d_args;
  /** body argument */
  int d_body_argument;
  /** do string replace
   *
   * Replaces all occurrences of oldStr with newStr in str.
   */
  void doStrReplace(std::string& str,
                    const std::string& oldStr,
                    const std::string& newStr) const;
};

/** sygus let expression constructor printer
 *
 * This class is used for printing sygus
 * datatype constructor terms whose top symbol
 * is a let expression.
 * For example, for grammar:
 *   A -> (let ((x B)) (+ A 1)) | x | (+ A A) | 0
 *   B -> 0 | 1
 * the first constructor for A takes as arguments
 * (B,A) and has sygus operator
 *   (lambda ((y Int) (z Int)) (+ z 1))
 * CVC4's support for let expressions in grammars
 * is highly limited, since notice that the
 * argument y : B is unused.
 *
 * For the above constructor, we have that
 *   d_let_body is (+ z 1),
 *   d_sygus_let_args is { y, z },
 *   d_sygus_num_let_input_args is 1, since
 *     y is an original input argument of the
 *     let expression, but z is not.
 */
class CVC4_PUBLIC SygusLetExprPrintCallback : public SygusExprPrintCallback
{
 public:
  SygusLetExprPrintCallback(Expr let_body,
                            std::vector<Expr>& let_args,
                            unsigned ninput_args);
  ~SygusLetExprPrintCallback() {}
  /** print sygus term e on output out using printer p */
  virtual void toStreamSygus(const Printer* p,
                             std::ostream& out,
                             Expr e) const override;

 private:
  /** number of arguments that are interpreted as input arguments */
  unsigned d_num_let_input_args;
};

/** sygus named constructor printer
 *
 * This callback is used for explicitly naming
 * the builtin term that a sygus datatype constructor
 * should be printed as. This is used for printing
 * terms in sygus grammars that involve defined
 * functions. For example, for grammar :
 *   A -> f( A ) | 0
 * where f is defined as:
 *   (define-fun ((x Int)) Int (+ x 1))
 * this class can be used as a callback for printing
 * the first sygus datatype constructor f, where we use
 * analog operator (lambda (x) (+ x 1)).
 */
class CVC4_PUBLIC SygusNamedPrintCallback : public SygusPrintCallback
{
 public:
  SygusNamedPrintCallback(std::string name);
  ~SygusNamedPrintCallback() {}
  /** print sygus term e on output out using printer p */
  virtual void toStreamSygus(const Printer* p,
                             std::ostream& out,
                             Expr e) const override;

 private:
  /** the defined function name */
  std::string d_name;
};

/** sygus empty printer
 *
 * This callback is used for printing constructors whose operators are
 * implicit, such as identity functions. For example, for grammar :
 *   A -> B
 *   B -> x | 0 | 1
 * The first constructor of A, call it cons, has sygus operator (lambda (x) x).
 * Call toStreamSygus on cons( t ) should call toStreamSygus on t directly.
 */
class CVC4_PUBLIC SygusEmptyPrintCallback : public SygusPrintCallback
{
 public:
  SygusEmptyPrintCallback() {}
  ~SygusEmptyPrintCallback() {}
  /** print sygus term e on output out using printer p */
  virtual void toStreamSygus(const Printer* p,
                             std::ostream& out,
                             Expr e) const override;
  /* Retrieves empty callback pointer */
  static inline std::shared_ptr<SygusEmptyPrintCallback> getEmptyPC()
  {
    if (!d_empty_pc)
    {
      d_empty_pc = std::make_shared<SygusEmptyPrintCallback>();
    }
    return d_empty_pc;
  }

 private:
  /* empty callback object */
  static std::shared_ptr<SygusEmptyPrintCallback> d_empty_pc;
};

} /* CVC4::printer namespace */
} /* CVC4 namespace */

#endif /* __CVC4__PRINTER__SYGUS_PRINT_CALLBACK_H */
