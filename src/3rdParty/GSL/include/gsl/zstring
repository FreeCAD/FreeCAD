///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef GSL_ZSTRING_H
#define GSL_ZSTRING_H

#include "./span_ext" // for dynamic_extent

#include <cstddef>   // for size_t, nullptr_t

namespace gsl
{
//
// czstring and wzstring
//
// These are "tag" typedefs for C-style strings (i.e. null-terminated character arrays)
// that allow static analysis to help find bugs.
//
// There are no additional features/semantics that we can find a way to add inside the
// type system for these types that will not either incur significant runtime costs or
// (sometimes needlessly) break existing programs when introduced.
//

template <typename CharT, std::size_t Extent = dynamic_extent>
using basic_zstring = CharT*;

using czstring = basic_zstring<const char, dynamic_extent>;

using cwzstring = basic_zstring<const wchar_t, dynamic_extent>;

using cu16zstring = basic_zstring<const char16_t, dynamic_extent>;

using cu32zstring = basic_zstring<const char32_t, dynamic_extent>;

using zstring = basic_zstring<char, dynamic_extent>;

using wzstring = basic_zstring<wchar_t, dynamic_extent>;

using u16zstring = basic_zstring<char16_t, dynamic_extent>;

using u32zstring = basic_zstring<char32_t, dynamic_extent>;

} // namespace gsl

#endif // GSL_ZSTRING_H
