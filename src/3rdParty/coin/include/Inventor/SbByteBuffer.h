/*
  Defining Private variables in exactly one location, to keep them in
  sync, remember to undefine this, so this is not visible outside this
  header, unless included from an .icc file
*/
/*
  NOTE: This define is done outside the Header guard, and undefined in
  the end. This is to make it possible to include this from an .icc
  file, even if the header has been included from before.
*/
#define SBBYTEBUFFER_PRIVATE_VARIABLES \
  size_t size_; \
  std::shared_ptr<char> buffer; \
  SbBool invalid; \
  static SbByteBuffer invalidBuffer_;

#ifndef COIN_SBBYTEBUFFER_H
#define COIN_SBBYTEBUFFER_H

#include <cstring>
#include <memory>
#include <Inventor/SbBasic.h>

#ifndef ABI_BREAKING_OPTIMIZE
class SbByteBufferP;
#endif //ABI_BREAKING_OPTIMIZE



//Consider making a general Buffer class for non bytes;
//Implements as a minimum the Buffer concept as defined by
//http://www.boost.org/doc/libs/1_37_0/libs/graph/doc/Buffer.html
class COIN_DLL_API SbByteBuffer {
 public:
  SbByteBuffer(const char * buffer);
  SbByteBuffer(const SbByteBuffer & buffer);
  SbByteBuffer(size_t size = 0, const char * buffer = NULL);
  SbByteBuffer(size_t size, const unsigned char * buffer);
  ~SbByteBuffer();

  SbBool isValid() const;
  size_t size() const;
  SbBool empty() const;

  const char & operator[](size_t idx) const;
  SbByteBuffer & operator=(const SbByteBuffer & in);
  SbBool operator==(const SbByteBuffer & that) const;
  SbByteBuffer & operator+=(const SbByteBuffer & buf) {
    this->push(buf);
    return *this;
  }

  void push(const SbByteBuffer & buf);

  const char * constData() const;
  char * data();

  static SbByteBuffer & invalidBuffer();
  void makeUnique();

 private:
#ifndef ABI_BREAKING_OPTIMIZE
  SbByteBufferP * pimpl;
#else
  SBBYTEBUFFER_PRIVATE_VARIABLES
#endif //ABI_BREAKING_OPTIMIZE
};

#ifdef ABI_BREAKING_OPTIMIZE
#include "SbByteBufferP.icc"
#endif //ABI_BREAKING_OPTIMIZE

#endif // !COIN_SBBYTEBUFFER_H

//The SBBYTEBUFFER_PRIVATE_VARIABLES must survive an inclusion from the .icc file
#ifndef COIN_ICC_INCLUDE
#undef SBBYTEBUFFER_PRIVATE_VARIABLES
#endif //COIN_ICC_INCLUDE
