// ============================================================================
// gzstream, C++ iostream classes wrapping the zlib compression library.
// Copyright (C) 2001  Deepak Bandyopadhyay, Lutz Kettner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ============================================================================
//
// File          : gzstream.h
// Revision      : Revision: 1.5 
// Revision_date : Date: 2002/04/26 23:30:15 
// Author(s)     : Deepak Bandyopadhyay, Lutz Kettner
// 
// Standard streambuf implementation following Nicolai Josuttis, "The 
// Standard C++ Library".
// ============================================================================

#ifndef GZSTREAM_H
#define GZSTREAM_H 1

// standard C++ with new header file names and std:: namespace
#include <iostream>
#include <fstream>
#include <istream>
#include <ios>
#include <zlib.h>

#ifdef _MSC_VER
using std::ostream;
using std::istream;
#endif


namespace Base {


#define BUFFERSIZE 47+256
//const int bufferSize = 47+256;    // size of data buff


// ----------------------------------------------------------------------------
// Internal classes to implement gzstream. See below for user classes.
// ----------------------------------------------------------------------------

class BaseExport gzstreambuf : public std::streambuf {
private:
    static const int bufferSize;    // size of data buff
    // totals 512 bytes under g++ for igzstream at the end.

    gzFile           file;               // file handle for compressed file
    char             buffer[BUFFERSIZE]; // data buffer
    char             opened;             // open/close state of stream
    int              mode;               // I/O mode

    int flush_buffer();
public:
    gzstreambuf() : file(nullptr), opened(0), mode(0) {
        setp( buffer, buffer + (bufferSize-1));
        setg( buffer + 4,     // beginning of putback area
              buffer + 4,     // read position
              buffer + 4);    // end position      
        // ASSERT: both input & output capabilities will not be used together
    }
    int is_open() { return opened; }
    gzstreambuf* open( const char* name, int open_mode, int comp);
    gzstreambuf* close();
    ~gzstreambuf() { close(); }
    
    virtual int     overflow( int c = EOF);
    virtual int     underflow();
    virtual int     sync();
};

class BaseExport gzstreambase : virtual public std::ios {
protected:
    gzstreambuf buf;
public:
    gzstreambase() { init(&buf); }
    gzstreambase( const char* name, int open_mode, int comp);
    ~gzstreambase();
    void open( const char* name, int open_mode, int comp);
    void close();
    gzstreambuf* rdbuf() { return &buf; }
};

// ----------------------------------------------------------------------------
// User classes. Use igzstream and ogzstream analogously to ifstream and
// ofstream respectively. They read and write files based on the gz* 
// function interface of the zlib. Files are compatible with gzip compression.
// ----------------------------------------------------------------------------

class BaseExport igzstream : public gzstreambase, public std::istream {
public:
    igzstream()
#ifdef _MSC_VER
      : istream( &buf) {} 
#else
      : std::istream( &buf) {} 
#endif
    igzstream( const char* name, int open_mode = std::ios_base::in, int comp = 1)
#ifdef _MSC_VER
      : gzstreambase( name, open_mode, comp ), istream( &buf) {}  
#else
      : gzstreambase( name, open_mode, comp), std::istream( &buf) {}  
#endif
    gzstreambuf* rdbuf() { return gzstreambase::rdbuf(); }
    void open( const char* name, int open_mode = std::ios_base::in, int comp = 1) {
        gzstreambase::open( name, open_mode, comp);
    }
};

class BaseExport ogzstream : public gzstreambase, public std::ostream {
public:
    ogzstream()
#ifdef _MSC_VER
      : ostream( &buf) {}
#else
      : std::ostream( &buf) {}
#endif
    ogzstream( const char* name, int mode = std::ios_base::out, int comp = 1)
#ifdef _MSC_VER
      : gzstreambase( name, mode, comp), ostream( &buf) {}  
#else
      : gzstreambase( name, mode, comp), std::ostream( &buf) {}  
#endif
    gzstreambuf* rdbuf() { return gzstreambase::rdbuf(); }
    void open( const char* name, int open_mode = std::ios_base::out, int comp = 1) {
        gzstreambase::open( name, open_mode, comp);
    }
};

} // namespace BAse

#endif // GZSTREAM_H
// ============================================================================
// EOF //

