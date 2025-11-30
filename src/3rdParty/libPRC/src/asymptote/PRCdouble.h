#ifndef __PRC_DOUBLE_H
#define __PRC_DOUBLE_H

#include <cstdlib>
#include <cmath>
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef BYTE_ORDER
# undef WORDS_BIG_ENDIAN
# undef WORDS_LITTLE_ENDIAN
# if BYTE_ORDER == BIG_ENDIAN
#  define WORDS_BIG_ENDIAN 1
# endif
# if BYTE_ORDER == LITTLE_ENDIAN
#  define WORDS_LITTLE_ENDIAN 1
# endif
#endif   

// from Adobe's documentation

union ieee754_double
{
 double d;
 /* This is the IEEE 754 double-precision format. */
 struct
 {
#ifdef WORDS_BIGENDIAN
  unsigned int negative:1;
  unsigned int exponent:11;
  /* Together these comprise the mantissa.  */
  unsigned int mantissa0:20;
  unsigned int mantissa1:32;
#else
  /* Together these comprise the mantissa.  */
  unsigned int mantissa1:32;
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int negative:1;
#endif
 } ieee;
};

union ieee754_float
{
 float f;
 /* This is the IEEE 754 float-precision format. */
 struct {
#ifdef WORDS_BIGENDIAN
 unsigned int negative:1;
 unsigned int exponent:8;
 unsigned int mantissa:23;
#else
 unsigned int mantissa:23;
 unsigned int exponent:8;
 unsigned int negative:1;
#endif
 } ieee;
};

enum ValueType {VT_double,VT_exponent};

struct sCodageOfFrequentDoubleOrExponent
{
  short Type;
  short NumberOfBits;
  unsigned Bits;
  union {
    unsigned ul[2];
    double Value;
  } u2uod;
};

#ifdef WORDS_BIGENDIAN
#       define DOUBLEWITHTWODWORD(upper,lower)  upper,lower
#       define UPPERPOWER       (0)
#       define LOWERPOWER       (!UPPERPOWER)

#       define NEXTBYTE(pbd)                            ((pbd)++)
#       define PREVIOUSBYTE(pbd)                ((pbd)--)
#       define MOREBYTE(pbd,pbend)              ((pbd)<=(pbend))
#       define OFFSETBYTE(pbd,offset)   ((pbd)+=offset)
#       define BEFOREBYTE(pbd)                  ((pbd)-1)
#       define DIFFPOINTERS(p1,p2)              ((p1)-(p2))
#       define SEARCHBYTE(pbstart,b,nb) (unsigned char *)memrchr((pbstart),(b),(nb))
#       define BYTEAT(pb,i)     *((pb)-(i))
#else
#       define DOUBLEWITHTWODWORD(upper,lower)  lower,upper
#       define UPPERPOWER       (1)
#       define LOWERPOWER       (!UPPERPOWER)

#       define NEXTBYTE(pbd)                            ((pbd)--)
#       define PREVIOUSBYTE(pbd)                ((pbd)++)
#       define MOREBYTE(pbd,pbend)              ((pbd)>=(pbend))
#       define OFFSETBYTE(pbd,offset)   ((pbd)-=offset)
#       define BEFOREBYTE(pbd)                  ((pbd)+1)
#       define DIFFPOINTERS(p1,p2)              ((unsigned)((p2)-(p1)))
#       define SEARCHBYTE(pbstart,b,nb) (unsigned char *)memchr((pbstart),(b),(nb))
#       define BYTEAT(pb,i)     *((pb)+(i))
#endif

#define MAXLENGTHFORCOMPRESSEDTYPE      ((22+1+1+4+6*(1+8))+7)/8

#define NEGATIVE(d)     (((union ieee754_double *)&(d))->ieee.negative)
#define EXPONENT(d)     (((union ieee754_double *)&(d))->ieee.exponent)
#define MANTISSA0(d)    (((union ieee754_double *)&(d))->ieee.mantissa0)
#define MANTISSA1(d)    (((union ieee754_double *)&(d))->ieee.mantissa1)

typedef unsigned char PRCbyte;
typedef unsigned short PRCword;
typedef unsigned PRCdword;

extern PRCdword stadwZero[2],stadwNegativeZero[2];

#define NUMBEROFELEMENTINACOFDOE   (2077)

#ifdef WORDS_BIGENDIAN
#       define DOUBLEWITHTWODWORDINTREE(upper,lower)    {upper,lower} 
#else
#       define DOUBLEWITHTWODWORDINTREE(upper,lower)    {lower,upper}
#endif
extern sCodageOfFrequentDoubleOrExponent acofdoe[NUMBEROFELEMENTINACOFDOE];

struct sCodageOfFrequentDoubleOrExponent* getcofdoe(unsigned,short);

#define STAT_V
#define STAT_DOUBLE

int stCOFDOECompare(const void*,const void*);

#ifdef WORDS_BIGENDIAN
#ifndef HAVE_MEMRCHR
void *memrchr(const void *,int,size_t);
#endif
#endif

#endif // __PRC_DOUBLE_H
