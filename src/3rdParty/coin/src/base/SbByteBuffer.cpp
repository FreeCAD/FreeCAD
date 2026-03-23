#ifdef ABI_BREAKING_OPTIMIZE
#include <Inventor/SbByteBuffer.h>

SbByteBuffer SbByteBuffer::invalidBuffer_;
#else
#define PIMPL_IMPLEMENTATION
#include <Inventor/SbByteBufferP.icc>

SbByteBuffer SbByteBufferP::invalidBuffer_;
#endif

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(pushUnique)
{

  static const char A [] = "ABC";
  static const char B [] = "XYZ";
  static char C [sizeof(A)+sizeof(B)-1];
  strcpy(C,A);
  strcat(C,B);

  SbByteBuffer a(sizeof(A)-1,A);
  SbByteBuffer b(sizeof(B)-1,B);
  SbByteBuffer c=a;

  c.push(b);

  bool allOk=true;
  for (size_t i=0;i<sizeof(A)-1;++i) {
    if (a[i]!=A[i])
      allOk=false;
  }
  for (size_t i=0;i<sizeof(B)-1;++i) {
    if (b[i]!=B[i])
      allOk=false;
  }
  BOOST_CHECK_MESSAGE(c.size()==sizeof(C)-1,"Concatenation does not have correct size");
  for (size_t i=0;i<sizeof(C)-1;++i) {
    if (c[i]!=C[i])
      allOk=false;
  }

  BOOST_CHECK_MESSAGE(allOk,
                      std::string("Concatenation of ") + A + " and " + B + " is not " + C
                      );
}

BOOST_AUTO_TEST_CASE(pushOnEmpty)
{
  SbByteBuffer a;
  SbByteBuffer b("foo");

  BOOST_CHECK_MESSAGE(a.empty(),
                      std::string("Size of empty buffer is") + ::CoinTest::stringify(a.size())
                      );

  a.push(b);

  BOOST_CHECK_MESSAGE(a.size() == b.size(),
                      "Size of buffers differ"
                      );

  for (size_t i=0; i < b.size(); ++i) {
    if(a[i]!=b[i]) {
      printf("Mjau %lu: %c != %c \n",(unsigned long)i,a[i],b[i]);
    }
  }

  BOOST_CHECK_MESSAGE(a == b,
                      "Byte representations differ"
                      );

}
#endif //COIN_TEST_SUITE
