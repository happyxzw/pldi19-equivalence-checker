/*********************                                                        */
/*! \file floatingpoint.h.in
 ** \verbatim
 ** Top contributors (to current version):
 **   Martin Brain, Tim King, Andres Noetzli
 ** Copyright (c) 2013  University of Oxford
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Utility functions for working with floating point theories. ]]
 **
 ** [[ This file contains the data structures used by the constant and
 **    parametric types of the floating point theory. ]]
 **/
#include <cvc4/cvc4_public.h>

#ifndef __CVC4__FLOATINGPOINT_H
#define __CVC4__FLOATINGPOINT_H

#include <cvc4/util/bitvector.h>
#include <cvc4/util/rational.h>

#include <fenv.h>

#if 0
#include <symfpu/core/unpackedFloat.h>
#endif /* 0 */

namespace CVC4 {
  // Inline these!
  inline bool CVC4_PUBLIC validExponentSize (unsigned e) { return e >= 2; }
  inline bool CVC4_PUBLIC validSignificandSize (unsigned s) { return s >= 2; }

  /**
   * Floating point sorts are parameterised by two non-zero constants
   * giving the width (in bits) of the exponent and significand
   * (including the hidden bit).
   */
  class CVC4_PUBLIC FloatingPointSize {
    /*
      Class invariants:
      * VALIDEXPONENTSIZE(e)
      * VALIDSIGNIFCANDSIZE(s)
     */

  private :
    unsigned e;
    unsigned s;

  public :
    FloatingPointSize (unsigned _e, unsigned _s);
    FloatingPointSize (const FloatingPointSize &old);

    inline unsigned exponent (void) const {
      return this->e;
    }

    inline unsigned significand (void) const {
      return this->s;
    }

    bool operator ==(const FloatingPointSize& fps) const {
      return (e == fps.e) && (s == fps.s);
    }

    // Implement the interface that symfpu uses for floating-point formats.
    inline unsigned exponentWidth(void) const { return this->exponent(); }
    inline unsigned significandWidth(void) const { return this->significand(); }
    inline unsigned packedWidth(void) const
    {
      return this->exponentWidth() + this->significandWidth();
    }
    inline unsigned packedExponentWidth(void) const
    {
      return this->exponentWidth();
    }
    inline unsigned packedSignificandWidth(void) const
    {
      return this->significandWidth() - 1;
    }

  }; /* class FloatingPointSize */

  struct CVC4_PUBLIC FloatingPointSizeHashFunction {
    static inline size_t ROLL(size_t X, size_t N) {
      return (((X) << (N)) | ((X) >> (8*sizeof((X)) - (N)) ));
    }

    inline size_t operator() (const FloatingPointSize& fpt) const {
      return size_t(ROLL(fpt.exponent(), 4*sizeof(unsigned)) |
		    fpt.significand());
    }
  }; /* struct FloatingPointSizeHashFunction */


  /**
   * A concrete instance of the rounding mode sort
   */
  enum CVC4_PUBLIC RoundingMode {
    roundNearestTiesToEven = FE_TONEAREST,
    roundTowardPositive = FE_UPWARD,
    roundTowardNegative = FE_DOWNWARD,
    roundTowardZero = FE_TOWARDZERO,
    // Initializes this to the diagonalization of the 4 other values.
    roundNearestTiesToAway = (((~FE_TONEAREST) & 0x1) | ((~FE_UPWARD) & 0x2) |
                              ((~FE_DOWNWARD) & 0x4) | ((~FE_TOWARDZERO) & 0x8))
  }; /* enum RoundingMode */

  struct CVC4_PUBLIC RoundingModeHashFunction {
    inline size_t operator() (const RoundingMode& rm) const {
      return size_t(rm);
    }
  }; /* struct RoundingModeHashFunction */

  /**
   * This is a symfpu literal "back-end".  It allows the library to be used as
   * an arbitrary precision floating-point implementation.  This is effectively
   * the glue between symfpu's notion of "signed bit-vector" and CVC4's
   * BitVector.
   */
  namespace symfpuLiteral {
  // Forward declaration of wrapper so that traits can be defined early and so
  // used in the implementation of wrappedBitVector.
  template <bool T>
  class wrappedBitVector;

  // This is the template parameter for symfpu's templates.
  class traits
  {
   public:
    // The six key types that symfpu uses.
    typedef unsigned bwt;
    typedef bool prop;
    typedef ::CVC4::RoundingMode rm;
    typedef ::CVC4::FloatingPointSize fpt;
    typedef wrappedBitVector<true> sbv;
    typedef wrappedBitVector<false> ubv;

    // Give concrete instances of each rounding mode, mainly for comparisons.
    static rm RNE(void);
    static rm RNA(void);
    static rm RTP(void);
    static rm RTN(void);
    static rm RTZ(void);

    // The sympfu properties.
    static void precondition(const prop &p);
    static void postcondition(const prop &p);
    static void invariant(const prop &p);
  };

  // Use the same type names as symfpu.
  typedef traits::bwt bwt;
  typedef traits::prop prop;
  typedef traits::rm rm;
  typedef traits::fpt fpt;
  typedef traits::ubv ubv;
  typedef traits::sbv sbv;

  // Type function for mapping between types
  template <bool T>
  struct signedToLiteralType;

  template <>
  struct signedToLiteralType<true>
  {
    typedef int literalType;
  };
  template <>
  struct signedToLiteralType<false>
  {
    typedef unsigned int literalType;
  };

  // This is an additional interface for CVC4::BitVector.
  // The template parameter distinguishes signed and unsigned bit-vectors, a
  // distinction symfpu uses.
  template <bool isSigned>
  class wrappedBitVector : public BitVector
  {
   protected:
    typedef typename signedToLiteralType<isSigned>::literalType literalType;

    friend wrappedBitVector<!isSigned>;  // To allow conversion between the
                                         // types
#if 0
    friend ::symfpu::ite<prop, wrappedBitVector<isSigned> >;  // For ITE
#endif /* 0 */

   public:
    wrappedBitVector(const bwt w, const unsigned v) : BitVector(w, v) {}
    wrappedBitVector(const prop &p) : BitVector(1, p ? 1U : 0U) {}
    wrappedBitVector(const wrappedBitVector<isSigned> &old) : BitVector(old) {}
    wrappedBitVector(const BitVector &old) : BitVector(old) {}
    bwt getWidth(void) const { return getSize(); }
    /*** Constant creation and test ***/

    static wrappedBitVector<isSigned> one(const bwt &w);
    static wrappedBitVector<isSigned> zero(const bwt &w);
    static wrappedBitVector<isSigned> allOnes(const bwt &w);

    prop isAllOnes() const;
    prop isAllZeros() const;

    static wrappedBitVector<isSigned> maxValue(const bwt &w);
    static wrappedBitVector<isSigned> minValue(const bwt &w);

    /*** Operators ***/
    wrappedBitVector<isSigned> operator<<(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator>>(
        const wrappedBitVector<isSigned> &op) const;

    /* Inherited but ...
     * *sigh* if we use the inherited version then it will return a
     * CVC4::BitVector which can be converted back to a
     * wrappedBitVector<isSigned> but isn't done automatically when working
     * out types for templates instantiation.  ITE is a particular
     * problem as expressions and constants no longer derive the
     * same type.  There aren't any good solutions in C++, we could
     * use CRTP but Liana wouldn't appreciate that, so the following
     * pointless wrapping functions are needed.
     */
    wrappedBitVector<isSigned> operator|(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator&(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator+(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator-(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator*(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator/(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator%(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> operator-(void) const;
    wrappedBitVector<isSigned> operator~(void)const;

    wrappedBitVector<isSigned> increment() const;
    wrappedBitVector<isSigned> decrement() const;
    wrappedBitVector<isSigned> signExtendRightShift(
        const wrappedBitVector<isSigned> &op) const;

    /*** Modular opertaions ***/
    // No overflow checking so these are the same as other operations
    wrappedBitVector<isSigned> modularLeftShift(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> modularRightShift(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> modularIncrement() const;
    wrappedBitVector<isSigned> modularDecrement() const;
    wrappedBitVector<isSigned> modularAdd(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> modularNegate() const;

    /*** Comparisons ***/
    // Inherited ... ish ...
    prop operator==(const wrappedBitVector<isSigned> &op) const;
    prop operator<=(const wrappedBitVector<isSigned> &op) const;
    prop operator>=(const wrappedBitVector<isSigned> &op) const;
    prop operator<(const wrappedBitVector<isSigned> &op) const;
    prop operator>(const wrappedBitVector<isSigned> &op) const;

    /*** Type conversion ***/
    wrappedBitVector<true> toSigned(void) const;
    wrappedBitVector<false> toUnsigned(void) const;

    /*** Bit hacks ***/
    wrappedBitVector<isSigned> extend(bwt extension) const;
    wrappedBitVector<isSigned> contract(bwt reduction) const;
    wrappedBitVector<isSigned> resize(bwt newSize) const;
    wrappedBitVector<isSigned> matchWidth(
        const wrappedBitVector<isSigned> &op) const;
    wrappedBitVector<isSigned> append(
        const wrappedBitVector<isSigned> &op) const;

    // Inclusive of end points, thus if the same, extracts just one bit
    wrappedBitVector<isSigned> extract(bwt upper, bwt lower) const;
  };
  }

  /**
   * A concrete floating point number
   */
#if 0
  typedef ::symfpu::unpackedFloat<symfpuLiteral::traits> FloatingPointLiteral;
#else
  class CVC4_PUBLIC FloatingPointLiteral {
  public :
    // This intentional left unfinished as the choice of literal
    // representation is solver specific.
    void unfinished (void) const;

    FloatingPointLiteral(unsigned, unsigned, double) { unfinished(); }
    FloatingPointLiteral(unsigned, unsigned, const std::string &) { unfinished(); }
    FloatingPointLiteral(const FloatingPointLiteral &) { unfinished(); }
    bool operator == (const FloatingPointLiteral &op) const {
      unfinished();
      return false;
    }

    size_t hash (void) const {
      unfinished();
      return 23;
    }
  };
#endif /* 0 */

  class CVC4_PUBLIC FloatingPoint {
  protected :
    FloatingPointLiteral fpl;

  public :
    FloatingPointSize t;

    FloatingPoint (unsigned e, unsigned s, const BitVector &bv);
    FloatingPoint (const FloatingPointSize &oldt, const FloatingPointLiteral &oldfpl) : fpl(oldfpl), t(oldt) {}
    FloatingPoint (const FloatingPoint &fp) : fpl(fp.fpl), t(fp.t) {}
    FloatingPoint (const FloatingPointSize &ct, const RoundingMode &rm, const BitVector &bv, bool signedBV);
    FloatingPoint (const FloatingPointSize &ct, const RoundingMode &rm, const Rational &r);


    static FloatingPoint makeNaN (const FloatingPointSize &t);
    static FloatingPoint makeInf (const FloatingPointSize &t, bool sign);
    static FloatingPoint makeZero (const FloatingPointSize &t, bool sign);

    const FloatingPointLiteral & getLiteral (void) const {
      return this->fpl;
    }

    // Gives the corresponding IEEE-754 transfer format
    BitVector pack (void) const;


    FloatingPoint absolute (void) const;
    FloatingPoint negate (void) const;
    FloatingPoint plus (const RoundingMode &rm, const FloatingPoint &arg) const;
    FloatingPoint sub (const RoundingMode &rm, const FloatingPoint &arg) const;
    FloatingPoint mult (const RoundingMode &rm, const FloatingPoint &arg) const;
    FloatingPoint div (const RoundingMode &rm, const FloatingPoint &arg) const;
    FloatingPoint fma (const RoundingMode &rm, const FloatingPoint &arg1, const FloatingPoint &arg2) const;
    FloatingPoint sqrt (const RoundingMode &rm) const;
    FloatingPoint rti (const RoundingMode &rm) const;
    FloatingPoint rem (const FloatingPoint &arg) const;

    // zeroCase is whether the left or right is returned in the case of min/max(-0,+0) or (+0,-0)
    FloatingPoint maxTotal (const FloatingPoint &arg, bool zeroCaseLeft) const;
    FloatingPoint minTotal (const FloatingPoint &arg, bool zeroCaseLeft) const;

    // These detect when the answer is defined
    typedef std::pair<FloatingPoint, bool> PartialFloatingPoint;

    PartialFloatingPoint max (const FloatingPoint &arg) const;
    PartialFloatingPoint min (const FloatingPoint &arg) const;


    bool operator ==(const FloatingPoint& fp) const;
    bool operator <= (const FloatingPoint &arg) const;
    bool operator < (const FloatingPoint &arg) const;

    bool isNormal (void) const;
    bool isSubnormal (void) const;
    bool isZero (void) const;
    bool isInfinite (void) const;
    bool isNaN (void) const;
    bool isNegative (void) const;
    bool isPositive (void) const;

    FloatingPoint convert (const FloatingPointSize &target, const RoundingMode &rm) const;

    // These require a value to return in the undefined case
    BitVector convertToBVTotal (BitVectorSize width, const RoundingMode &rm,
				bool signedBV, BitVector undefinedCase) const;
    Rational convertToRationalTotal (Rational undefinedCase) const;

    // These detect when the answer is defined
    typedef std::pair<BitVector, bool> PartialBitVector;
    typedef std::pair<Rational, bool> PartialRational;

    PartialBitVector convertToBV (BitVectorSize width, const RoundingMode &rm,
				bool signedBV) const;
    PartialRational convertToRational (void) const;

  }; /* class FloatingPoint */


  struct CVC4_PUBLIC FloatingPointHashFunction {
    size_t operator() (const FloatingPoint& fp) const {
      FloatingPointSizeHashFunction fpshf;
      BitVectorHashFunction bvhf;

      return fpshf(fp.t) ^ bvhf(fp.pack());
    }
  }; /* struct FloatingPointHashFunction */

  /**
   * The parameter type for the conversions to floating point.
   */
  class CVC4_PUBLIC FloatingPointConvertSort {
  public :
    FloatingPointSize t;

    FloatingPointConvertSort (unsigned _e, unsigned _s)
      : t(_e,_s) {}
    FloatingPointConvertSort (const FloatingPointSize &fps)
      : t(fps) {}

    bool operator ==(const FloatingPointConvertSort& fpcs) const {
      return t == fpcs.t;
    }

  };

  /**
   * As different conversions are different parameterised kinds, there
   * is a need for different (C++) types for each one.
   */

  class CVC4_PUBLIC FloatingPointToFPIEEEBitVector : public FloatingPointConvertSort {
  public :
    FloatingPointToFPIEEEBitVector (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPIEEEBitVector (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };

  class CVC4_PUBLIC FloatingPointToFPFloatingPoint : public FloatingPointConvertSort {
  public :
    FloatingPointToFPFloatingPoint (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPFloatingPoint (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };

  class CVC4_PUBLIC FloatingPointToFPReal : public FloatingPointConvertSort {
  public :
    FloatingPointToFPReal (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPReal (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };

  class CVC4_PUBLIC FloatingPointToFPSignedBitVector : public FloatingPointConvertSort {
  public :
    FloatingPointToFPSignedBitVector (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPSignedBitVector (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };

  class CVC4_PUBLIC FloatingPointToFPUnsignedBitVector : public FloatingPointConvertSort {
  public :
    FloatingPointToFPUnsignedBitVector (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPUnsignedBitVector (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };

  class CVC4_PUBLIC FloatingPointToFPGeneric : public FloatingPointConvertSort {
  public :
    FloatingPointToFPGeneric (unsigned _e, unsigned _s) : FloatingPointConvertSort(_e,_s) {}
    FloatingPointToFPGeneric (const FloatingPointConvertSort &old) : FloatingPointConvertSort(old) {}
  };



  template <unsigned key>
  struct CVC4_PUBLIC FloatingPointConvertSortHashFunction {
    inline size_t operator() (const FloatingPointConvertSort& fpcs) const {
      FloatingPointSizeHashFunction f;
      return f(fpcs.t) ^ (0x00005300 | (key << 24));
    }
  }; /* struct FloatingPointConvertSortHashFunction */








  /**
   * The parameter type for the conversion to bit vector.
   */
  class CVC4_PUBLIC FloatingPointToBV {
  public :
    BitVectorSize bvs;

    FloatingPointToBV (unsigned s)
      : bvs(s) {}
    FloatingPointToBV (const FloatingPointToBV &old) : bvs(old.bvs) {}
    FloatingPointToBV (const BitVectorSize &old) : bvs(old) {}
    operator unsigned () const { return bvs; }
  };

  class CVC4_PUBLIC FloatingPointToUBV : public FloatingPointToBV {
  public : FloatingPointToUBV (unsigned _s) : FloatingPointToBV(_s) {}
    FloatingPointToUBV (const FloatingPointToBV &old) : FloatingPointToBV(old) {}
  };
  class CVC4_PUBLIC FloatingPointToSBV : public FloatingPointToBV {
  public : FloatingPointToSBV (unsigned _s) : FloatingPointToBV(_s) {}
    FloatingPointToSBV (const FloatingPointToBV &old) : FloatingPointToBV(old) {}
  };
  class CVC4_PUBLIC FloatingPointToUBVTotal : public FloatingPointToBV {
  public : FloatingPointToUBVTotal (unsigned _s) : FloatingPointToBV(_s) {}
    FloatingPointToUBVTotal (const FloatingPointToBV &old) : FloatingPointToBV(old) {}
  };
  class CVC4_PUBLIC FloatingPointToSBVTotal : public FloatingPointToBV {
  public : FloatingPointToSBVTotal (unsigned _s) : FloatingPointToBV(_s) {}
    FloatingPointToSBVTotal (const FloatingPointToBV &old) : FloatingPointToBV(old) {}
  };


  template <unsigned key>
  struct CVC4_PUBLIC FloatingPointToBVHashFunction {
    inline size_t operator() (const FloatingPointToBV& fptbv) const {
      UnsignedHashFunction< ::CVC4::BitVectorSize > f;
      return 	(key ^ 0x46504256) ^ f(fptbv.bvs);
    }
  }; /* struct FloatingPointToBVHashFunction */


  // It is not possible to pack a number without a size to pack to,
  // thus it is difficult to implement this in a sensible way.  Use
  // FloatingPoint instead...
  /*
  inline std::ostream& operator <<(std::ostream& os, const FloatingPointLiteral& fp) CVC4_PUBLIC;
  inline std::ostream& operator <<(std::ostream& os, const FloatingPointLiteral& fp) {
    return os << "FloatingPointLiteral";
  }
  */

  inline std::ostream& operator <<(std::ostream& os, const FloatingPoint& fp) CVC4_PUBLIC;
  inline std::ostream& operator <<(std::ostream& os, const FloatingPoint& fp) {
    BitVector bv(fp.pack());

    unsigned largestSignificandBit = fp.t.significand() - 2; // -1 for -inclusive, -1 for hidden
    unsigned largestExponentBit = (fp.t.exponent() - 1) + (largestSignificandBit + 1);

    return os
      << "(fp #b" << bv.extract(largestExponentBit + 1, largestExponentBit + 1)
      << " #b" << bv.extract(largestExponentBit, largestSignificandBit + 1)
      << " #b" << bv.extract(largestSignificandBit, 0)
      << ")";
  }

  inline std::ostream& operator <<(std::ostream& os, const FloatingPointSize& fps) CVC4_PUBLIC;
  inline std::ostream& operator <<(std::ostream& os, const FloatingPointSize& fps) {
    return os << "(_ FloatingPoint " << fps.exponent() << " " << fps.significand() << ")";
  }

  inline std::ostream& operator <<(std::ostream& os, const FloatingPointConvertSort& fpcs) CVC4_PUBLIC;
  inline std::ostream& operator <<(std::ostream& os, const FloatingPointConvertSort& fpcs) {
    return os << "(_ to_fp " << fpcs.t.exponent() << " " << fpcs.t.significand() << ")";
  }

}/* CVC4 namespace */

#endif /* __CVC4__FLOATINGPOINT_H */
