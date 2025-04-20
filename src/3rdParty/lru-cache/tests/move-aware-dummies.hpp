/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include <cstddef>
#include <string>
#include <utility>

template <typename SubClassTemplateParameterJustForNewStaticMembersHehehe>
struct MoveAwareBase {
  static std::size_t move_count;
  static std::size_t non_move_count;
  static std::size_t forwarding_count;
  static std::size_t copy_count;

  static void reset() {
    move_count = 0;
    non_move_count = 0;
    forwarding_count = 0;
    copy_count = 0;
  }

  MoveAwareBase(const MoveAwareBase& other) : s(other.s) {
    copy_count += 1;
  }

  MoveAwareBase(MoveAwareBase&& other) : s(std::move(other.s)) {
    // Just need to implement so it's not deactivated
    // (because we do need the copy constructor)
  }

  MoveAwareBase(std::string&& s_) : s(std::move(s_)) {
    move_count += 1;
  }

  MoveAwareBase(std::string& s_) : s(s_) {
    non_move_count += 1;
  }

  MoveAwareBase(const char* s_) : s(s_) {
    forwarding_count += 1;
  }

  MoveAwareBase(const int& x, const double& y)
  : s(std::to_string(x) + std::to_string(y)) {
    non_move_count += 1;
  }

  MoveAwareBase(int&& x, double&& y)
  : s(std::to_string(x) + std::to_string(y)) {
    move_count += 1;
  }

  virtual ~MoveAwareBase() = default;

  MoveAwareBase& operator=(const MoveAwareBase& other) {
    copy_count += 1;
    s = other.s;
    return *this;
  }

  MoveAwareBase& operator=(MoveAwareBase&& other) {
    s = std::move(other.s);
    return *this;
  }

  bool operator==(const MoveAwareBase& other) const noexcept {
    return this->s == other.s;
  }

  bool operator!=(const MoveAwareBase& other) const noexcept {
    return !(*this == other);
  }

  std::string s;
};

template <typename T>
std::size_t MoveAwareBase<T>::move_count = 0;

template <typename T>
std::size_t MoveAwareBase<T>::non_move_count = 0;

template <typename T>
std::size_t MoveAwareBase<T>::forwarding_count = 0;

template <typename T>
std::size_t MoveAwareBase<T>::copy_count = 0;

struct MoveAwareKey : public MoveAwareBase<MoveAwareKey> {
  using super = MoveAwareBase<MoveAwareKey>;

  // clang-format off
  MoveAwareKey() = default;
  MoveAwareKey(const MoveAwareKey& other) : super(other) {}
  MoveAwareKey(MoveAwareKey&& other) : super(std::move(other)) {}
  MoveAwareKey(std::string&& s_) : super(std::move(s_)) {}
  MoveAwareKey(std::string& s_) : super(s_) {}
  MoveAwareKey(const char* s_) : super(s_) {}
  MoveAwareKey(const int& x, const double& y) : super(x, y) {}
  MoveAwareKey(int&& x, double&& y) : super(std::move(x), std::move(y)) {}
  // clang-format on

  MoveAwareKey& operator=(const MoveAwareKey& other) {
    super::operator=(other);
    return *this;
  }

  MoveAwareKey& operator=(MoveAwareKey&& other) {
    super::operator=(std::move(other));
    return *this;
  }
};

struct MoveAwareValue : public MoveAwareBase<MoveAwareValue> {
  using super = MoveAwareBase<MoveAwareValue>;

  // clang-format off
  MoveAwareValue() = default;
  MoveAwareValue(const MoveAwareValue& other) : super(other) {}
  MoveAwareValue(MoveAwareValue&& other) : super(std::move(other)) {}
  MoveAwareValue(std::string&& s_) : super(std::move(s_)) {}
  MoveAwareValue(std::string& s_) : super(s_) {}
  MoveAwareValue(const char* s_) : super(s_) {}
  MoveAwareValue(const int& x, const double& y) : super(x, y) {}
  MoveAwareValue(int&& x, double&& y) : super(std::move(x), std::move(y)) {}
  // clang-format on

  MoveAwareValue& operator=(const MoveAwareValue& other) {
    super::operator=(other);
    return *this;
  }

  MoveAwareValue& operator=(MoveAwareValue&& other) {
    super::operator=(std::move(other));
    return *this;
  }
};

namespace std {
template <>
struct hash<MoveAwareKey> {
  auto operator()(const MoveAwareKey& key) const {
    return hash<std::string>()(key.s);
  }
};

template <>
struct hash<MoveAwareValue> {
  auto operator()(const MoveAwareValue& value) const {
    return hash<std::string>()(value.s);
  }
};
}  // namespace std
