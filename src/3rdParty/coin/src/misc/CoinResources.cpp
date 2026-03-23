/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include <Inventor/misc/CoinResources.h>

/*!
  \namespace CoinResources
  \brief Static utility functions for managing data resources at the basic buffer level.

  This class is just a static scope for some resource managing
  functions.  With this, Coin can register built-in default resources
  that can later be retrieved through the same resource locator,
  either from disk (if present) and as a fallback the built-in
  (compiled in) buffer

  The resource locators take the form of a URL with coin as its schema and empty host eg.
  "coin://path/to/resource.ext".

  The "coin:" prefix is for Coin to differentiate a resource locator from
  a filename (for multipurpose function usage).

  The path/to/resource.ext is for most platforms the path under the
  environment variable $COINDIR where the file should be present.  For
  Mac OS X, the path is under the Inventor framework bundle Resources/
  directory, if Coin was installed as a framework that is.

  The file on disk can be an updated version, compared to the
  compiled-in buffer, which is why the externalized files are prioritized
  over the built-in buffers.

  A resource does not need to have a corresponding external file.  This is
  configured in the flags parameter when the resource is set.  You can in
  other words also register built-in-only resources.

  \ingroup coin_internal
*/

#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbByteBuffer.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#if defined(COIN_MACOS_10) && defined(COIN_MACOSX_FRAMEWORK)
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFURL.h>
#endif // COIN_MACOS_10 && COIN_MACOSX_FRAMEWORK

#include "tidbitsp.h"
#include "coindefs.h"

#if COIN_WORKAROUND(COIN_MSVC, <= COIN_MSVC_6_0_VERSION)
// symbol length truncation
#pragma warning(disable:4786)
#endif // VC6.0

#include <map>

#ifndef MAXPATHLEN
#define MAXPATHLEN 2048
#endif // !MAXPATHLEN

//Anonymous namespace
namespace CoinResources { namespace {
    class ResourceHandle {
    public:
      ResourceHandle(void)
        : resloc(NULL), canbefile(FALSE), filenotfound(FALSE)
      { }

      char * resloc;
      SbBool canbefile;
      SbBool filenotfound;

      SbByteBuffer loadedbuf;
      SbByteBuffer internalbuf;
    };

    typedef std::map<const char *, ResourceHandle *> ResourceMap;

    ResourceMap * resourcemap;

    ResourceHandle * getResourceHandle(const char * resloc);
    ResourceHandle * createResourceHandle(const char * resloc);
    void cleanup(void);

    void
    cleanup(void)
    {
      ResourceMap::iterator it = resourcemap->begin();
      while (it != resourcemap->end()) {
        delete it->second;
        ++it;
      }
      delete resourcemap;
      resourcemap = NULL;
    }

    ResourceHandle *
    getResourceHandle(const char * resloc)
    {
      assert(resloc);
      SbName reslochash(resloc);
      ResourceMap::iterator it =
        resourcemap->find(reslochash.getString());
      if (it == resourcemap->end()) return NULL;
      return it->second;
    }

    ResourceHandle *
    createResourceHandle(const char * resloc)
    {
      assert(resloc);
      SbName reslochash(resloc);
      ResourceHandle * handle = new ResourceHandle;
      handle->resloc = const_cast<char *>(reslochash.getString());
      std::pair<const char *, ResourceHandle *> mapentry(reslochash.getString(), handle);
      resourcemap->insert(mapentry);
      return handle;
    }
    static const char PREFIX [] = "coin:";

}}

void
CoinResources::init(void)
{
  CoinResources::resourcemap = new CoinResources::ResourceMap;
  cc_coin_atexit_static_internal((coin_atexit_f*) CoinResources::cleanup);
}


/*!
  Returns a resource if one exists. If the Coin installation permits,
  the resource will be loaded from file, but if the file cannot be
  located or loaded, built-in versions will be returned instead.

  \return TRUE on success, and FALSE if there is no such resource.
*/
SbByteBuffer
CoinResources::get(const char * resloc)
{
  if (strncmp(resloc, PREFIX, sizeof(PREFIX)-1) != 0) {
    return SbByteBuffer::invalidBuffer();
  }

  ResourceHandle * handle = CoinResources::getResourceHandle(resloc);
  if (!handle) {
    return SbByteBuffer::invalidBuffer();
  }

  if (handle->loadedbuf.empty() && handle->canbefile && !handle->filenotfound) {
    // try loading file from COINDIR/...
    do { // to 'break' out of this try-file-loading sequence
      SbString filename;
#if defined(COIN_MACOS_10) && defined(COIN_MACOSX_FRAMEWORK)
      // CFBundleIdentifier in Info.plist
      CFStringRef identifier =
        CFStringCreateWithCString(kCFAllocatorDefault,
                                  COIN_MAC_FRAMEWORK_IDENTIFIER_CSTRING,
                                  kCFStringEncodingASCII);
      CFBundleRef coinbundle = CFBundleGetBundleWithIdentifier(identifier);
      CFRelease(identifier);
      if (!coinbundle) {
        handle->filenotfound = TRUE;
        break;
      }

      // search app-bundle as well? probably not
      // CFBundleRef mainbundle = CFBundleGetMainBundle();

      CFURLRef url = CFBundleCopyResourcesDirectoryURL(coinbundle);
      UInt8 buf[MAXPATHLEN];

      if (!CFURLGetFileSystemRepresentation(url, true, buf, MAXPATHLEN-1)) {
        handle->filenotfound = TRUE;
        CFRelease(url);
        break;
      }
      filename.sprintf("%s/%s", buf, resloc + 5);
      CFRelease(url);
#else // !COIN_MACOSX_FRAMEWORK
      static const char * coindirenv = coin_getenv("COINDIR");
      if (coindirenv == NULL) {
        handle->filenotfound = TRUE;
        break;
      }
      filename.sprintf("%s/%s/%s", coindirenv, COIN_DATADIR, resloc + 5);
#endif // !COIN_MACOSX_FRAMEWORK
      if (COIN_DEBUG && 0) {
        SoDebugError::postInfo("CoinResources::get", "trying to load '%s'.",
                               filename.getString());
      }
      FILE * fp = fopen(filename.getString(), "rb");
      if (!fp) {
        handle->filenotfound = TRUE;
        break;
      }

      fseek(fp, 0, SEEK_END);
      long size = ftell(fp);
      if (size < 0) {
        fclose(fp);
        handle->filenotfound = TRUE;
        break;
      }

      fseek(fp, 0, SEEK_SET);

      SbByteBuffer buffer(size);

      size_t num = fread(buffer.data(), size, 1, fp);
      fclose(fp);
      fp = NULL;

      if (num == 1) {
        // FIXME: at this point we can check if this is the first
        // load, and if so hook up freeLoadedExternals() to atexit()
        // to clean up those buffers automatically.  Or we can maybe
        // hook up something that clears out everything instead.
        handle->loadedbuf = buffer;

        if (COIN_DEBUG && 0) {
          SoDebugError::postInfo("CoinResources::get", "load '%s' ok.",
                                 filename.getString());
        }
      } else {
        handle->filenotfound = TRUE;
        break;
      }
    } while ( FALSE );
  }

  assert(handle);
  if (! handle->loadedbuf.empty()) {
    return handle->loadedbuf;
  }

  assert(handle);
  return handle->internalbuf;
}

/*!
  This function registers a new resource.  The resource locator (\a resloc)
  should take the form "coin:" followed by a relative file path that should
  lead to the file representation of the resource from where the COINDIR
  environment variable points.  The relative path should use / for directory
  separation, and not \ if on Microsoft Windows.

  If you put COIN_RESOURCE_NOT_A_FILE in the \a flags argument, then the
  automatic file searching will not be performed.

  \returns TRUE if the resource was set, and FALSE if something went wrong.
  FALSE would most likely be returned because the resource already exists.
*/
SbBool
CoinResources::set(const char * resloc, const SbByteBuffer & buffer, ResourceFlags flags)
{
  if (strncmp(resloc, PREFIX, sizeof(PREFIX)-1) != 0) {
    return FALSE;
  }

  ResourceHandle * handle = CoinResources::getResourceHandle(resloc);
  if (handle) { // already set
    SoDebugError::post("CoinResources::set", "Resource already set.");
    return FALSE;
  }
  handle = CoinResources::createResourceHandle(resloc);
  assert(handle);
  handle->internalbuf = buffer;
  if (flags & COIN_RESOURCE_NOT_A_FILE) {
    handle->canbefile = FALSE;
  } else {
    handle->canbefile = TRUE;
  }
  return TRUE;
}

/*!
  This function deallocates all the buffers that have been loaded
  from disk.  Internal buffers will not be freed, as they are expected
  to be compiled into the Coin library data section.
*/
void
CoinResources::freeLoadedExternals(void)
{
  CoinResources::ResourceMap::iterator it =
    CoinResources::resourcemap->begin();
  while (it != CoinResources::resourcemap->end()) {
    ResourceHandle * handle = it->second;
    if (!handle->loadedbuf.empty()) {
      //FIXME: This may be entirely unnecessary with the SbByteBuffer class BFG 20090212
      handle->loadedbuf = SbByteBuffer();
    }
    ++it;
  }
}
