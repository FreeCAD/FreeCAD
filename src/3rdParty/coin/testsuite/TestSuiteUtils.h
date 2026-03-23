#ifndef COIN_TESTSUITEUTILS_H
#define COIN_TESTSUITEUTILS_H

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

#include <string>
#include <vector>

class SoNode;

namespace SIM { namespace Coin3D { namespace Coin { namespace TestSuite {

typedef bool test_files_CB(SoNode * root, const std::string & filename);

void Init(void);

void PushMessageSuppressFilters(const char * patterns[]);
void PopMessageSuppressFilters(void);

void ResetDebugInfoCount(int count = 0);
int GetDebugInfoCount(void);

void ResetDebugWarningCount(int count = 0);
int GetDebugWarningCount(void);

void ResetDebugErrorCount(int count = 0);
int GetDebugErrorCount(void);

void ResetReadErrorCount(int count = 0);
int GetReadErrorCount(void);

void ResetMemoryErrorCount(int count = 0);
int GetMemoryErrorCount(void);

SoNode * ReadInventorFile(const char * filename);
int WriteInventorFile(const char * filename, SoNode * root);

void test_file(const std::string & filename,
                    test_files_CB * testFunction);
void test_all_files(const std::string & search_directory,
                    std::vector<std::string> & suffixes,
                    test_files_CB * testFunction);
bool testCorrectFile(SoNode * root, const std::string & filename);
bool testIncorrectFile(SoNode * root, const std::string & filename);
bool testOutOfSpecFile(SoNode * root, const std::string & filename);

} } } } // namespace

#endif // !COIN_TESTSUITEUTILS_H
