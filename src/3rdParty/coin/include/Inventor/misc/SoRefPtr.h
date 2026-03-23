#ifndef COIN_SOREFPTR_H
#define COIN_SOREFPTR_H

#include <utility>

template <typename T>
class SoRefPtr {
public:
  SoRefPtr(void) noexcept : ptr(NULL) { }

  explicit SoRefPtr(T * p) : ptr(p)
  {
    if (this->ptr) this->ptr->ref();
  }

  SoRefPtr(const SoRefPtr & other) : ptr(other.ptr)
  {
    if (this->ptr) this->ptr->ref();
  }

  SoRefPtr(SoRefPtr && other) noexcept : ptr(other.ptr)
  {
    other.ptr = NULL;
  }

  ~SoRefPtr(void)
  {
    if (this->ptr) this->ptr->unref();
  }

  SoRefPtr & operator=(SoRefPtr other) noexcept
  {
    this->swap(other);
    return *this;
  }

  void reset(T * p = NULL)
  {
    SoRefPtr tmp(p);
    this->swap(tmp);
  }

  T * get(void) const noexcept { return this->ptr; }
  T & operator*(void) const { return *this->ptr; }
  T * operator->(void) const noexcept { return this->ptr; }
  explicit operator bool(void) const noexcept { return this->ptr != NULL; }

  void swap(SoRefPtr & other) noexcept
  {
    using std::swap;
    swap(this->ptr, other.ptr);
  }

private:
  T * ptr;
};

#endif // !COIN_SOREFPTR_H

