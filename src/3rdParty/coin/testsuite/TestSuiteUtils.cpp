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

#include <cassert>
#include <cstdio>
#include <cstring>

#if defined (_POSIX_C_SOURCE) || defined (_POSIX_SOURCE) || defined(__APPLE__) || defined(__FreeBSD__)
#define USE_POSIX
#elif defined(_WIN32)
#define USE_WIN32
#else //_WIN32
#error Unknown system
#endif //POSIX

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoMemoryError.h>
#include <Inventor/errors/SoReadError.h>

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/nodes/SoSeparator.h>

#include <TestSuiteUtils.h>

#include "CoinTest.h"



using namespace SIM::Coin3D::Coin;

namespace {

int debuginfocount = 0;
int debugwarningcount = 0;
int debugerrorcount = 0;
int readerrorcount = 0;
int memoryerrorcount = 0;

struct filterset {
  char ** filters;
  filterset * next;
} * messagefilters = NULL;

int
should_filter(const SbString & msg)
{
  filterset * filters = messagefilters;
  while (filters != NULL) {
    for (int i = 0; filters->filters[i] != NULL; ++i) {
      if (msg.find(filters->filters[i]) >= 0) {
        return 1;
      }
    }
    filters = filters->next;
  }
  return 0;
}

void
debugerrormsg_handler(const SoError * error, void * data)
{
  assert(error);
  const SoDebugError * dbgerror = static_cast<const SoDebugError *>(error);
  switch (dbgerror->getSeverity()) {
  case SoDebugError::INFO:
    ++debuginfocount;
    break;
  case SoDebugError::WARNING:
    ++debugwarningcount;
    break;
  case SoDebugError::ERROR:
    ++debugerrorcount;
    break;
  default:
    assert(0 && "schizofrene SoDebugError error");
  }
  const SbString & msg = error->getDebugString();
  if (!should_filter(msg)) fprintf(stderr, "%s\n", msg.getString());
}

void
readerrormsg_handler(const SoError * error, void * data)
{
  ++readerrorcount;
  const SbString & msg = error->getDebugString();
  if (!should_filter(msg)) fprintf(stderr, "%s\n", msg.getString());
}

void
memoryerrormsg_handler(const SoError * error, void * data)
{
  ++memoryerrorcount;
  const SbString & msg = error->getDebugString();
  if (!should_filter(msg)) fprintf(stderr, "%s\n", msg.getString());
}

} // anonymous namespace

void
TestSuite::Init(void)
{
  SoDebugError::setHandlerCallback(debugerrormsg_handler, NULL);
  SoReadError::setHandlerCallback(readerrormsg_handler, NULL);
  SoMemoryError::setHandlerCallback(memoryerrormsg_handler, NULL);
}

void
TestSuite::PushMessageSuppressFilters(const char * patterns[])
{
  filterset * filters = new filterset;
  filters->next = messagefilters;
  messagefilters = filters;

  int count = 0;
  for (; patterns[count] != NULL; ++count) { }
  filters->filters = new char * [ count + 1 ];
  for (int i = 0; i < count; ++i ) {
    filters->filters[i] = new char [ strlen(patterns[i]) + 1 ];
    strcpy(filters->filters[i], patterns[i]);
  }
  filters->filters[count] = NULL;
}

void
TestSuite::PopMessageSuppressFilters(void)
{
  assert(messagefilters);
  filterset * filters = messagefilters;
  messagefilters = filters->next;

  for (int i = 0; filters->filters[i] != NULL; ++i) {
    delete [] filters->filters[i];
  }
  delete [] filters->filters;
  delete filters;
}

void
TestSuite::ResetDebugInfoCount(int count)
{
  debuginfocount = count;
}

int
TestSuite::GetDebugInfoCount(void)
{
  return debuginfocount;
}

void
TestSuite::ResetDebugWarningCount(int count)
{
  debugwarningcount = count;
}

int
TestSuite::GetDebugWarningCount(void)
{
  return debugwarningcount;
}

void
TestSuite::ResetDebugErrorCount(int count)
{
  debugerrorcount = count;
}

int
TestSuite::GetDebugErrorCount(void)
{
  return debugerrorcount;
}

void
TestSuite::ResetReadErrorCount(int count)
{
  readerrorcount = count;
}

int
TestSuite::GetReadErrorCount(void)
{
  return readerrorcount;
}

void
TestSuite::ResetMemoryErrorCount(int count)
{
  memoryerrorcount = count;
}

int
TestSuite::GetMemoryErrorCount(void)
{
  return memoryerrorcount;
}

SoNode *
TestSuite::ReadInventorFile(const char * filename)
{
  assert(filename);
  SoNode * root = NULL;
  {
    SoInput in;
    if (!in.openFile(filename)) {
      return NULL;
    }
    root = SoDB::readAll(&in);
  }
  return root;
}

int
TestSuite::WriteInventorFile(const char * filename, SoNode * root)
{
  assert(filename);
  assert(root);
  SoOutput out;
  if (!out.openFile(filename)) {
    return FALSE;
  }
  SoWriteAction wa(&out);
  root->ref();
  wa.apply(root);
  root->unrefNoDelete();
  return TRUE;
}

/*
  This strange place to do includes is actually necessary to avoid interference between SoDebugError::ERROR and the windows.h header.
*/
#ifdef USE_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif //USE_POSIX
#ifdef USE_WIN32
#include <windows.h>
#endif //_WIN32

namespace {
  static const char DIRECTORY_SEPARATOR [] = "/";

  std::string coin_getcwd()
  {
    char buf[1024];
#ifdef USE_POSIX
    if (!getcwd(buf,sizeof(buf)))
      return "";
#endif //USE_POSIX
#ifdef USE_WIN32
    GetCurrentDirectory(sizeof(buf),buf);
#endif //USE_WIN32
    return buf;
  }

  int
  coin_chdir(const std::string & path)
  {
#ifdef USE_POSIX
    return chdir(path.c_str());
#endif //POSIX
#ifdef USE_WIN32
    return SetCurrentDirectory(path.c_str());
#endif //_WIN32
  }

  bool
  compare_suffix(const std::string & input ,const std::string & suffix) {
    size_t suffixLength = suffix.size();

	size_t n = input.size();
    if (n<suffixLength)
      return false;

    std::string toCompare = input.substr(n-suffixLength,suffixLength);
    return toCompare == suffix;
  }
  bool exists(const std::string & path) {
#ifdef USE_POSIX
    struct stat st;
    if (stat(path.c_str(),&st))
#endif //USE_POSIX
#ifdef USE_WIN32
    DWORD       fileAttr;

    fileAttr = GetFileAttributes(path.c_str());
    if (INVALID_FILE_ATTRIBUTES == fileAttr)
#endif //USE_WIN32
      return false;
    return true;
  }

  bool is_directory(const std::string & path) {
#ifdef USE_POSIX
    struct stat st;
    if (stat(path.c_str(),&st))
      return false;
    return S_ISDIR(st.st_mode);
#endif //USE_POSIX
#ifdef USE_WIN32
    DWORD       fileAttr;

    fileAttr = GetFileAttributes(path.c_str());
    return (fileAttr & FILE_ATTRIBUTE_DIRECTORY);
#endif //USE_WIN32
  }

  bool
  find_file( const std::string & dir_path,         // in this directory,
             const std::string & suffix, // search for this suffix,
             std::vector<std::string> & paths )            // placing path here if found
  {
    bool file_found = false;
    if ( !exists( dir_path ) ) return false;
#ifdef USE_POSIX
    DIR * dh;
    if ((dh = opendir(dir_path.c_str()))) {
      struct dirent * itr;
      while ((itr = readdir(dh))) {
        std::string filename = itr->d_name;
#endif //USE_POSIX
#ifdef USE_WIN32
  WIN32_FIND_DATA f;
  HANDLE h = FindFirstFile((dir_path+"/*").c_str(), &f);
  if(h != INVALID_HANDLE_VALUE)
  {
    do
    {
      std::string filename = f.cFileName;
#endif //USE_WIN32
      if ( (filename == ".") || (filename == "..") )
        continue;
      std::string full_path = dir_path + DIRECTORY_SEPARATOR + filename;
      if (is_directory(full_path)) {
        if ( find_file( full_path, suffix, paths ) )
          file_found = true;
      }
      else if (compare_suffix(full_path,suffix)) {
        paths.push_back(full_path);
        file_found = true;
      }
#ifdef USE_POSIX
            }
      closedir(dh);
#endif //USE_POSIX
#ifdef USE_WIN32
    } while(FindNextFile(h, &f));
#endif //USE_WIN32
  }

  return file_found;
}
}

void
TestSuite::test_file(const std::string & filename,
                          test_files_CB * testFunction)
{
  std::string basepath = coin_getcwd();
  size_t n = filename.find_last_of(DIRECTORY_SEPARATOR);
  std::string dir = filename.substr(0,n);
  std::string file = filename.substr(n+1,filename.size()-n-1);
  coin_chdir(dir);
  SoNode * fileroot = TestSuite::ReadInventorFile(file.c_str());
  testFunction(fileroot, filename);
  if (fileroot != NULL) {
    fileroot->ref();
    fileroot->unref();
  }
  coin_chdir(basepath);
}

void
TestSuite::test_all_files(const std::string & search_directory,
                          std::vector<std::string> & suffixes,
                          test_files_CB * testFunction)
{
  std::string basepath = coin_getcwd();

  std::vector<std::string> paths;
  for (
       std::vector<std::string>::const_iterator it = suffixes.begin();
       it != suffixes.end();
       ++it)
    {
      find_file(search_directory, *it, paths);
    }

  for (
       std::vector<std::string>::const_iterator it = paths.begin();
       it != paths.end();
       ++it)
    {
      test_file(*it, testFunction);
    }
}

bool
TestSuite::testCorrectFile(SoNode * root, const std::string & filename) {
    BOOST_CHECK_MESSAGE((root != NULL) && (GetReadErrorCount() == 0), (std::string("failed to read file ") + filename).c_str());
    ResetReadErrorCount();
    return root != NULL;
}

bool
TestSuite::testIncorrectFile(SoNode * root, const std::string & filename) {
    BOOST_CHECK_MESSAGE((root == NULL) || (GetReadErrorCount() > 0), (std::string("Managed to read an incorrect file ") + filename).c_str());
    ResetReadErrorCount();
    return root != NULL;
}

bool
TestSuite::testOutOfSpecFile(SoNode * root, const std::string & filename) {
    BOOST_CHECK_MESSAGE(root != NULL, (std::string("This out of spec file could be read in an earlier version ") + filename).c_str());
    return root != NULL;
}
