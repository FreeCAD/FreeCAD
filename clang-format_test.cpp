/***************************************************************************
 *
 * test file for clang-format.
 *
 * created 22 Nov 2022
 *
 * This file is provided to enable evaluation of clang-format settings.
 * From a format point of view it is deliberately rubbish!
 * Please add any formatting situation of interest
*
   *
 * DO NOT FORMAT BEFORE MERGE !!
 *
          ***************************************************************************/

#ifndef _PreComp_
#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#include <sys/types.h>
#elif defined(__MINGW32__)
#define WINVER 0x502// needed for SetDllDirectory
#include <Windows.h>
#endif
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

#include "Annotation.h"

FC_LOG_LEVEL_INIT("App", true, true);

using namespace App;
using Base::FileInfo;
namespace sp = std::placeholders;
namespace test
{
//========================comment==================================================
// Application
//========================= ===================== very long comment =========== ========  ================= ============m

int var1;
int var2();
int var2a();
int var3{};
int var3a {};// comment
int var4=4;
int var4a = 4;
ptrType* var5 = nullptr;
ptrType* var5a {};
// interupting comment
ptrType* var5b{};
ptrType * var5c {};
ptrType *var5d {};
const int var6 {};

struct HelloStruct { // comment
    int aa = 1;
    std::string astring {"hello"};
};
class HelloWorld // comment
{
public:
    HelloWorld() {};
    char* ternary() { return a ? getA() : getB(); }
    int switchMeth()
    {
        switch (b) {
            case 1:
                someStuff();
                break;
            case 2:
                break;
            default:;
        }
    }
    void method() noexcept {
        std::string str{};
        std::vector<std::string>> sss {};
        for(const auto & vec : vecs){
            sss.emplace_back(vec.first());
        }
    };




private:
    int a;
    int b {3};
    int aa, bb, cc;
    std::vector<std::string> vecs {"aaa", "bbb"};
};
auto hello = new HelloWorld();
void aFunction(
    int theFirstParameter, int theSecondParameter, int theThirdParameterIsABitLonger,int yetAnotherParameterSoTheArgListIsLong)
{   if (var1) { doa(); } else if (var2) { dob(); } else { doNothing(); }
    bool fred=var1||var2&&var3&&!var3;
    if (var1) sameLineNoBrackets();
    if (var2) {sameLineBrackets();}
    if (var2) {
        NewLineBrackets();
    }
    if (var2)
    {
        NewLineBrackets();
    }
    if (!var2)
        booleanNot();
 }



}//namespace test.
// Is there an ending EOL on its own line??
