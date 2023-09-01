/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QBuffer>
# include <QByteArray>
# include <QIODevice>
# include <cstring>
#ifdef __GNUC__
# include <cstdint>
#endif
#endif

#include "Stream.h"
#include "Swap.h"
#include <CXX/Objects.hxx>


using namespace Base;

Stream::Stream() = default;

Stream::~Stream() = default;

Stream::ByteOrder Stream::byteOrder() const
{
    return _swap ? BigEndian : LittleEndian;
}

void Stream::setByteOrder(ByteOrder bo)
{
    _swap = (bo == BigEndian);
}

OutputStream::OutputStream(std::ostream &rout) : _out(rout)
{
}

OutputStream::~OutputStream() = default;

OutputStream& OutputStream::operator << (bool b)
{
    _out.write((const char*)&b, sizeof(bool));
    return *this;
}

OutputStream& OutputStream::operator << (int8_t ch)
{
    _out.write((const char*)&ch, sizeof(int8_t));
    return *this;
}

OutputStream& OutputStream::operator << (uint8_t uch)
{
    _out.write((const char*)&uch, sizeof(uint8_t));
    return *this;
}

OutputStream& OutputStream::operator << (int16_t s)
{
    if (isSwapped()) SwapEndian<int16_t>(s);
    _out.write((const char*)&s, sizeof(int16_t));
    return *this;
}

OutputStream& OutputStream::operator << (uint16_t us)
{
    if (isSwapped()) SwapEndian<uint16_t>(us);
    _out.write((const char*)&us, sizeof(uint16_t));
    return *this;
}

OutputStream& OutputStream::operator << (int32_t i)
{
    if (isSwapped()) SwapEndian<int32_t>(i);
    _out.write((const char*)&i, sizeof(int32_t));
    return *this;
}

OutputStream& OutputStream::operator << (uint32_t ui)
{
    if (isSwapped()) SwapEndian<uint32_t>(ui);
    _out.write((const char*)&ui, sizeof(uint32_t));
    return *this;
}

OutputStream& OutputStream::operator << (int64_t l)
{
    if (isSwapped()) SwapEndian<int64_t>(l);
    _out.write((const char*)&l, sizeof(int64_t));
    return *this;
}

OutputStream& OutputStream::operator << (uint64_t ul)
{
    if (isSwapped()) SwapEndian<uint64_t>(ul);
    _out.write((const char*)&ul, sizeof(uint64_t));
    return *this;
}

OutputStream& OutputStream::operator << (float f)
{
    if (isSwapped()) SwapEndian<float>(f);
    _out.write((const char*)&f, sizeof(float));
    return *this;
}

OutputStream& OutputStream::operator << (double d)
{
    if (isSwapped()) SwapEndian<double>(d);
    _out.write((const char*)&d, sizeof(double));
    return *this;
}

InputStream::InputStream(std::istream &rin) : _in(rin)
{
}

InputStream::~InputStream() = default;

InputStream& InputStream::operator >> (bool& b)
{
    _in.read((char*)&b, sizeof(bool));
    return *this;
}

InputStream& InputStream::operator >> (int8_t& ch)
{
    _in.read((char*)&ch, sizeof(int8_t));
    return *this;
}

InputStream& InputStream::operator >> (uint8_t& uch)
{
    _in.read((char*)&uch, sizeof(uint8_t));
    return *this;
}

InputStream& InputStream::operator >> (int16_t& s)
{
    _in.read((char*)&s, sizeof(int16_t));
    if (isSwapped()) SwapEndian<int16_t>(s);
    return *this;
}

InputStream& InputStream::operator >> (uint16_t& us)
{
    _in.read((char*)&us, sizeof(uint16_t));
    if (isSwapped()) SwapEndian<uint16_t>(us);
    return *this;
}

InputStream& InputStream::operator >> (int32_t& i)
{
    _in.read((char*)&i, sizeof(int32_t));
    if (isSwapped()) SwapEndian<int32_t>(i);
    return *this;
}

InputStream& InputStream::operator >> (uint32_t& ui)
{
    _in.read((char*)&ui, sizeof(uint32_t));
    if (isSwapped()) SwapEndian<uint32_t>(ui);
    return *this;
}

InputStream& InputStream::operator >> (int64_t& l)
{
    _in.read((char*)&l, sizeof(int64_t));
    if (isSwapped()) SwapEndian<int64_t>(l);
    return *this;
}

InputStream& InputStream::operator >> (uint64_t& ul)
{
    _in.read((char*)&ul, sizeof(uint64_t));
    if (isSwapped()) SwapEndian<uint64_t>(ul);
    return *this;
}

InputStream& InputStream::operator >> (float& f)
{
    _in.read((char*)&f, sizeof(float));
    if (isSwapped()) SwapEndian<float>(f);
    return *this;
}

InputStream& InputStream::operator >> (double& d)
{
    _in.read((char*)&d, sizeof(double));
    if (isSwapped()) SwapEndian<double>(d);
    return *this;
}

// ----------------------------------------------------------------------

ByteArrayOStreambuf::ByteArrayOStreambuf(QByteArray& ba) : _buffer(new QBuffer(&ba))
{
    _buffer->open(QIODevice::WriteOnly);
}

ByteArrayOStreambuf::~ByteArrayOStreambuf()
{
    _buffer->close();
    delete _buffer;
}

std::streambuf::int_type
ByteArrayOStreambuf::overflow(std::streambuf::int_type c)
{
    if (c != EOF) {
        char z = static_cast<char>(c);
        if (_buffer->write (&z, 1) != 1) {
            return EOF;
        }
    }
    return c;
}

std::streamsize ByteArrayOStreambuf::xsputn (const char* s, std::streamsize num)
{
    return _buffer->write(s,num);
}

std::streambuf::pos_type
ByteArrayOStreambuf::seekoff(std::streambuf::off_type off,
                             std::ios_base::seekdir way,
                             std::ios_base::openmode /*mode*/)
{
    off_type endpos = 0;
    off_type curpos = _buffer->pos();
    switch (way) {
        case std::ios_base::beg:
            endpos = off;
            break;
        case std::ios_base::cur:
            endpos = curpos + off;
            break;
        case std::ios_base::end:
            endpos = _buffer->size();
            break;
        default:
            return {off_type(-1)};
    }

    if (endpos != curpos) {
        if (!_buffer->seek(endpos))
            endpos = -1;
    }

    return {endpos};
}

std::streambuf::pos_type
ByteArrayOStreambuf::seekpos(std::streambuf::pos_type pos,
                             std::ios_base::openmode /*mode*/)
{
    return seekoff(pos, std::ios_base::beg);
}

// ----------------------------------------------------------------------

ByteArrayIStreambuf::ByteArrayIStreambuf(const QByteArray& data)
    : _buffer(data)
    , _beg(0)
    , _end(data.size())
    , _cur(0)
{
}

ByteArrayIStreambuf::~ByteArrayIStreambuf() = default;

ByteArrayIStreambuf::int_type ByteArrayIStreambuf::underflow()
{
    if (_cur == _end)
        return traits_type::eof();

    return static_cast<ByteArrayIStreambuf::int_type>(_buffer[_cur]) & 0x000000ff;
}

ByteArrayIStreambuf::int_type ByteArrayIStreambuf::uflow()
{
    if (_cur == _end)
        return traits_type::eof();

    return static_cast<ByteArrayIStreambuf::int_type>(_buffer[_cur++]) & 0x000000ff;
}

ByteArrayIStreambuf::int_type ByteArrayIStreambuf::pbackfail(int_type ch)
{
    if (_cur == _beg || (ch != traits_type::eof() && ch != _buffer[_cur-1]))
        return traits_type::eof();

    return static_cast<ByteArrayIStreambuf::int_type>(_buffer[--_cur]) & 0x000000ff;
}

std::streamsize ByteArrayIStreambuf::showmanyc()
{
    return _end - _cur;
}

std::streambuf::pos_type
ByteArrayIStreambuf::seekoff(std::streambuf::off_type off,
                             std::ios_base::seekdir way,
                             std::ios_base::openmode /*mode*/ )
{
    int p_pos=-1;
    if (way == std::ios_base::beg)
        p_pos = _beg;
    else if (way == std::ios_base::end)
        p_pos = _end;
    else if (way == std::ios_base::cur)
        p_pos = _cur;

    if (p_pos > _end)
        return traits_type::eof();

    if (((p_pos + off) > _end) || ((p_pos + off) < _beg))
        return traits_type::eof();

    _cur = p_pos+ off;

    return ((p_pos+off) - _beg);
}

std::streambuf::pos_type
ByteArrayIStreambuf::seekpos(std::streambuf::pos_type pos,
                             std::ios_base::openmode /*mode*/)
{
    return seekoff(pos, std::ios_base::beg);
}

// ----------------------------------------------------------------------

IODeviceOStreambuf::IODeviceOStreambuf(QIODevice* dev) : device(dev)
{
}

IODeviceOStreambuf::~IODeviceOStreambuf() = default;

std::streambuf::int_type
IODeviceOStreambuf::overflow(std::streambuf::int_type c)
{
    if (c != EOF) {
        char z = static_cast<char>(c);
        if (device->write (&z, 1) != 1) {
            return EOF;
        }
    }
    return c;
}

std::streamsize IODeviceOStreambuf::xsputn (const char* s, std::streamsize num)
{
    return device->write(s,num);
}

std::streambuf::pos_type
IODeviceOStreambuf::seekoff(std::streambuf::off_type off,
                            std::ios_base::seekdir way,
                            std::ios_base::openmode /*mode*/)
{
    off_type endpos = 0;
    off_type curpos = device->pos();
    switch (way) {
        case std::ios_base::beg:
            endpos = off;
            break;
        case std::ios_base::cur:
            endpos = curpos + off;
            break;
        case std::ios_base::end:
            endpos = device->size();
            break;
        default:
            return {off_type(-1)};
    }

    if (endpos != curpos) {
        if (!device->seek(endpos))
            endpos = -1;
    }

    return {endpos};
}

std::streambuf::pos_type
IODeviceOStreambuf::seekpos(std::streambuf::pos_type pos,
                            std::ios_base::openmode /*mode*/)
{
    return seekoff(pos, std::ios_base::beg);
}

// ----------------------------------------------------------------------

IODeviceIStreambuf::IODeviceIStreambuf(QIODevice* dev) : device(dev)
{
    setg (buffer+pbSize,     // beginning of putback area
          buffer+pbSize,     // read position
          buffer+pbSize);    // end position
}

IODeviceIStreambuf::~IODeviceIStreambuf() = default;

std::streambuf::int_type
IODeviceIStreambuf::underflow()
{
#ifndef _MSC_VER
using std::memcpy;
#endif

    // is read position before end of buffer?
    if (gptr() < egptr()) {
        return *gptr();
    }

    /* process size of putback area
     * - use number of characters read
     * - but at most size of putback area
     */
    int numPutback{};
    numPutback = gptr() - eback();
    if (numPutback > pbSize) {
        numPutback = pbSize;
    }

    /* copy up to pbSize characters previously read into
     * the putback area
     */
    memcpy (buffer+(pbSize-numPutback), gptr()-numPutback,
            numPutback);

    // read at most bufSize new characters
    int num{};
    num = device->read(buffer+pbSize, bufSize);
    if (num <= 0) {
        // ERROR or EOF
        return EOF;
    }

    // reset buffer pointers
    setg (buffer+(pbSize-numPutback),   // beginning of putback area
          buffer+pbSize,                // read position
          buffer+pbSize+num);           // end of buffer

    // return next character
    return *gptr();
}

std::streambuf::pos_type
IODeviceIStreambuf::seekoff(std::streambuf::off_type off,
                            std::ios_base::seekdir way,
                            std::ios_base::openmode /*mode*/)
{
    off_type endpos = 0;
    off_type curpos = device->pos();
    switch (way) {
        case std::ios_base::beg:
            endpos = off;
            break;
        case std::ios_base::cur:
            endpos = curpos + off;
            break;
        case std::ios_base::end:
            endpos = device->size();
            break;
        default:
            return {off_type(-1)};
    }

    if (endpos != curpos) {
        if (!device->seek(endpos))
            endpos = -1;
    }

    return {endpos};
}

std::streambuf::pos_type
IODeviceIStreambuf::seekpos(std::streambuf::pos_type pos,
                            std::ios_base::openmode /*mode*/)
{
    return seekoff(pos, std::ios_base::beg);
}

// ---------------------------------------------------------

#define PYSTREAM_BUFFERED

// http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
// http://www.icce.rug.nl/documents/cplusplus/cplusplus24.html
PyStreambuf::PyStreambuf(PyObject* o, std::size_t buf_size, std::size_t put_back)
    : inp(o)
    , put_back(std::max(put_back, std::size_t(1)))
    , buffer(std::max(buf_size, put_back) + put_back)
{
    Py_INCREF(inp);
    char *end = &buffer.front() + buffer.size();
    setg(end, end, end);
#ifdef PYSTREAM_BUFFERED
    char *base = &buffer.front();
    setp(base, base + buffer.size());
#endif
}

PyStreambuf::~PyStreambuf()
{
    PyStreambuf::sync();
    Py_DECREF(inp);
}

PyStreambuf::int_type PyStreambuf::underflow()
{
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    char *base = &buffer.front();
    char *start = base;

    if (eback() == base) { // true when this isn't the first fill
        std::memmove(base, egptr() - put_back, put_back);
        start += put_back;
    }

    std::size_t n{};
    Py::Tuple arg(1);
    long len = static_cast<long>(buffer.size() - (start - base));
    arg.setItem(0, Py::Long(len));
    Py::Callable meth(Py::Object(inp).getAttr("read"));

    try {
        std::string c;
        Py::Object res(meth.apply(arg));
        if (res.isBytes()) {
            c = static_cast<std::string>(Py::Bytes(res));
        }
        else if (res.isString()) {
            c = static_cast<std::string>(Py::String(res));
        }
        else {
            // wrong type
            return traits_type::eof();
        }

        n = c.size();
        if (n == 0) {
            return traits_type::eof();
        }

        std::memcpy(start, &(c[0]), c.size());
    }
    catch (Py::Exception& e) {
        e.clear();
        return traits_type::eof();
    }

    setg(base, start, start + n);
    return traits_type::to_int_type(*gptr());
}

PyStreambuf::int_type
PyStreambuf::overflow(PyStreambuf::int_type ch)
{
#ifdef PYSTREAM_BUFFERED
    sync();
    if (ch != traits_type::eof()) {
        *pptr() = static_cast<char>(ch);
        pbump(1);
        return ch;
    }

    return traits_type::eof();
#else
    if (ch != EOF) {
        char z = ch;
        if (!writeStr(&z, 1))
            return traits_type::eof();
    }

    return ch;
#endif
}

int PyStreambuf::sync()
{
#ifdef PYSTREAM_BUFFERED
    if (pptr() > pbase()) {
        flushBuffer();
    }
    return 0;
#else
    return std::streambuf::sync();
#endif
}

bool PyStreambuf::flushBuffer()
{
    std::ptrdiff_t n = pptr() - pbase();
    pbump(-n);
    return writeStr(pbase(), n);
}

bool PyStreambuf::writeStr(const char* str, std::streamsize num)
{
    try {
        Py::Tuple arg(1);
        Py::Callable meth(Py::Object(inp).getAttr("write"));

        if (type == StringIO) {
            arg.setItem(0, Py::String(str, num));
            meth.apply(arg);
            return true;
        }
        else if (type == BytesIO) {
            arg.setItem(0, Py::Bytes(str, num));
            meth.apply(arg);
            return true;
        }
        else {
            // try out what works
            try {
                arg.setItem(0, Py::String(str, num));
                meth.apply(arg);
                type = StringIO;
                return true;
            }
            catch (Py::Exception& e) {
                if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                    e.clear();
                    arg.setItem(0, Py::Bytes(str, num));
                    meth.apply(arg);
                    type = BytesIO;
                    return true;
                }
                else {
                    throw; // re-throw
                }
            }
        }
    }
    catch(Py::Exception& e) {
        e.clear();
    }

    return false;
}

std::streamsize PyStreambuf::xsputn (const char* s, std::streamsize num)
{
#ifdef PYSTREAM_BUFFERED
    return std::streambuf::xsputn(s, num);
#else
    if (!writeStr(s, num))
        return 0;
    return num;
#endif
}

PyStreambuf::pos_type
PyStreambuf::seekoff(PyStreambuf::off_type offset, PyStreambuf::seekdir dir, PyStreambuf::openmode /*mode*/)
{
    int whence = 0;
    switch (dir) {
    case std::ios_base::beg:
        whence = 0;
        break;
    case std::ios_base::cur:
        whence = 1;
        break;
    case std::ios_base::end:
        whence = 2;
        break;
    default:
        return {off_type(-1)};
    }

    try {
        Py::Tuple arg(2);
        arg.setItem(0, Py::Long(static_cast<long>(offset)));
        arg.setItem(1, Py::Long(whence));
        Py::Callable seek(Py::Object(inp).getAttr("seek"));
        seek.apply(arg);

        // get current position
        Py::Tuple arg2;
        Py::Callable tell(Py::Object(inp).getAttr("tell"));
        Py::Long pos(tell.apply(arg2));
        long cur_pos = static_cast<long>(pos);
        return static_cast<pos_type>(cur_pos);
    }
    catch(Py::Exception& e) {
        e.clear();
        return {off_type(-1)};
    }
}

PyStreambuf::pos_type
PyStreambuf::seekpos(PyStreambuf::pos_type offset, PyStreambuf::openmode mode)
{
    return seekoff(offset, std::ios::beg, mode);
}

// ---------------------------------------------------------

Streambuf::Streambuf(const std::string& data)
{
    _beg = data.begin();
    _end = data.end();
    _cur = _beg; //NOLINT
}

Streambuf::~Streambuf() = default;

Streambuf::int_type Streambuf::underflow()
{
    if (_cur == _end)
        return traits_type::eof();

    return static_cast<Streambuf::int_type>(*_cur) & 0x000000ff;
}

Streambuf::int_type Streambuf::uflow()
{
    if (_cur == _end)
        return traits_type::eof();

    return static_cast<Streambuf::int_type>(*_cur++) & 0x000000ff;
}

Streambuf::int_type Streambuf::pbackfail( int_type ch )
{
    /* coverity[negative_returns] _cur is an iterator */
    if (_cur == _beg || (ch != traits_type::eof() && ch != _cur[-1]))
        return traits_type::eof();

    return static_cast<Streambuf::int_type>(*--_cur) & 0x000000ff;
}

std::streamsize Streambuf::showmanyc()
{
    return _end - _cur;
}

std::streambuf::pos_type
Streambuf::seekoff(std::streambuf::off_type off,
                   std::ios_base::seekdir way,
                   std::ios_base::openmode /*mode*/ )
{
    std::string::const_iterator p_pos;
    if (way == std::ios_base::beg)
        p_pos = _beg;
    else if (way == std::ios_base::end)
        p_pos = _end;
    else if (way == std::ios_base::cur)
        p_pos = _cur;

    if (p_pos > _end)
        return traits_type::eof();

    if (((p_pos + off) > _end) || ((p_pos + off) < _beg))
        return traits_type::eof();

    _cur = p_pos+ off;

    return ((p_pos+off) - _beg);
}

std::streambuf::pos_type
Streambuf::seekpos(std::streambuf::pos_type pos,
                   std::ios_base::openmode /*mode*/)
{
    return seekoff(pos, std::ios_base::beg);
}
