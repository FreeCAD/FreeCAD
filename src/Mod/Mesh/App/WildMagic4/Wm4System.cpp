// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.0 (2006/06/28)

#include "Wm4FoundationPCH.h"
#include "Wm4System.h"
using namespace Wm4;

// support for Load
#include <sys/stat.h>

// support for GetTime
#if !defined(_WIN32)
#include <sys/time.h>
static timeval gs_kInitial;
static bool gs_bInitializedTime = false;
#else
#include <sys/timeb.h>
static long gs_lInitialSec = 0;
static long gs_lInitialUSec = 0;
static bool gs_bInitializedTime = false;
#endif

// support for locating the application directory
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

char System::ms_acPath[SYSTEM_MAX_PATH];
char System::ms_acEnvVar[SYSTEM_MAX_ENVVAR];
std::vector<std::string>* System::ms_pkDirectories = nullptr;
char System::WM4_PATH[SYSTEM_MAX_ENVVAR];

//----------------------------------------------------------------------------
void System::SwapBytes (int iSize, void* pvValue)
{
    // size must be even
    assert(iSize >= 2 && (iSize & 1) == 0);

    char* acBytes = (char*) pvValue;
    for (int i0 = 0, i1 = iSize-1; i0 < iSize/2; i0++, i1--)
    {
        char cSave = acBytes[i0];
        acBytes[i0] = acBytes[i1];
        acBytes[i1] = cSave;
    }
}
//----------------------------------------------------------------------------
void System::SwapBytes (int iSize, int iQuantity, void* pvValue)
{
    // size must be even
    assert(iSize >= 2 && (iSize & 1) == 0);

    char* acBytes = (char*) pvValue;
    for (int i = 0; i < iQuantity; i++, acBytes += iSize)
    {
        for (int i0 = 0, i1 = iSize-1; i0 < iSize/2; i0++, i1--)
        {
            char cSave = acBytes[i0];
            acBytes[i0] = acBytes[i1];
            acBytes[i1] = cSave;
        }
    }
}
//----------------------------------------------------------------------------
bool System::IsBigEndian ()
{
    int iInt = 1;
    char* pcChar = (char*)&iInt;
    return !(*pcChar);
}
//----------------------------------------------------------------------------
void System::EndianCopy (int iSize, const void* pvSrc, void* pvDst)
{
    size_t uiSize = (size_t)iSize;
    Memcpy(pvDst,uiSize,pvSrc,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(iSize,pvDst);
#endif
}
//----------------------------------------------------------------------------
void System::EndianCopy (int iSize, int iQuantity, const void* pvSrc,
    void* pvDst)
{
    size_t uiSize = (size_t)(iSize*iQuantity);
    Memcpy(pvDst,uiSize,pvSrc,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(iSize,iQuantity,pvDst);
#endif
}
//----------------------------------------------------------------------------
double System::GetTime ()
{
#if !defined(_WIN32)
    if (!gs_bInitializedTime)
    {
        gs_bInitializedTime = true;
        gettimeofday(&gs_kInitial, nullptr);
    }

    struct timeval kCurrent;
    gettimeofday(&kCurrent,nullptr);
    
    struct timeval kDelta;
    timersub(&kCurrent,&gs_kInitial,&kDelta);

    return 0.001*(double)(1000*kDelta.tv_sec + kDelta.tv_usec/1000);
#else
    struct timeb kTB;

    if (!gs_bInitializedTime)
    {
        gs_bInitializedTime = true;
        ftime(&kTB);
        gs_lInitialSec = (long)kTB.time;
        gs_lInitialUSec = 1000*kTB.millitm;
    }

    ftime(&kTB);
    long lCurrentSec = (long)kTB.time;
    long lCurrentUSec = 1000*kTB.millitm;
    long lDeltaSec = lCurrentSec - gs_lInitialSec;
    long lDeltaUSec = lCurrentUSec -gs_lInitialUSec;
    if (lDeltaUSec < 0)
    {
        lDeltaUSec += 1000000;
        lDeltaSec--;
    }

    return 0.001*(double)(1000*lDeltaSec + lDeltaUSec/1000);
#endif
}
//----------------------------------------------------------------------------
bool System::Load (const char* acFilename, char*& racBuffer, int& riSize)
{
    struct stat kStat;
    if (stat(acFilename,&kStat) != 0)
    {
        // file does not exist
        racBuffer = nullptr;
        riSize = 0;
        return false;
    }

    FILE* pkFile = System::Fopen(acFilename,"rb");
    assert(pkFile);
    if (!pkFile)
    {
        racBuffer = nullptr;
        riSize = 0;
        return false;
    }

    riSize = kStat.st_size;
    racBuffer = WM4_NEW char[riSize];
    int iRead = (int)fread(racBuffer,sizeof(char),riSize,pkFile);
    if (System::Fclose(pkFile) != 0 || iRead != riSize)
    {
        assert(false);
        WM4_DELETE[] racBuffer;
        racBuffer = nullptr;
        riSize = 0;
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
bool System::Save (const char* acFilename, const char* acBuffer, int iSize)
{
    if (!acBuffer || iSize <= 0)
    {
        // The input buffer must exist.  It is not possible to verify that
        // the buffer has the specified number of bytes.
        assert(false);
        return false;
    }

    FILE* pkFile = System::Fopen(acFilename,"wb");
    if (!pkFile)
    {
        return false;
    }

    int iWrite = (int)fwrite(acBuffer,sizeof(char),iSize,pkFile);
    if (System::Fclose(pkFile) != 0 || iWrite != iSize)
    {
        assert( false );
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
bool System::Append (const char* acFilename, char* acBuffer, int iSize)
{
    if (!acBuffer || iSize <= 0)
    {
        // The input buffer must exist.  It is not possible to verify that
        // the buffer has the specified number of bytes.
        assert(false);
        return false;
    }

    FILE* pkFile = System::Fopen(acFilename,"ab");
    if (!pkFile)
    {
        return false;
    }

    int iWrite = (int)fwrite(acBuffer,sizeof(char),iSize,pkFile);
    if (System::Fclose(pkFile) != 0 || iWrite != iSize)
    {
        assert( false );
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
int System::Read1 (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    size_t uiSize = (size_t)iQuantity;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
    return iQuantity;
}
//----------------------------------------------------------------------------
int System::Write1 (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    size_t uiSize = (size_t)iQuantity;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
    return iQuantity;
}
//----------------------------------------------------------------------------
int System::Read1 (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,1,iQuantity,pkFile); (void)r;
    return iQuantity;
}
//----------------------------------------------------------------------------
int System::Write1 (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    fwrite(pvData,1,iQuantity,pkFile);
    return iQuantity;
}
//----------------------------------------------------------------------------
int System::Read2le (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 2*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read4le (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 4*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read8le (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 8*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write2le (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 2*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write4le (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 4*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write8le (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 8*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifdef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read2le (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,2,iQuantity,pkFile); (void)r;
#ifdef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,pvData);
#endif
    return 2*iQuantity;
}
//----------------------------------------------------------------------------
int System::Read4le (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,4,iQuantity,pkFile); (void)r;
#ifdef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,pvData);
#endif
    return 4*iQuantity;
}
//----------------------------------------------------------------------------
int System::Read8le (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,8,iQuantity,pkFile); (void)r;
#ifdef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,pvData);
#endif
    return 8*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write2le (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifdef WM4_BIG_ENDIAN
    const short* psData = (const short*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        short sTemp = *psData++;
        SwapBytes(2,&sTemp);
        fwrite(&sTemp,2,1,pkFile);
    }
#else
    fwrite(pvData,2,iQuantity,pkFile);
#endif
    return 2*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write4le (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifdef WM4_BIG_ENDIAN
    const int* piData = (const int*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        int iTemp = *piData++;
        SwapBytes(4,&iTemp);
        fwrite(&iTemp,4,1,pkFile);
    }
#else
    fwrite(pvData,4,iQuantity,pkFile);
#endif
    return 4*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write8le (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifdef WM4_BIG_ENDIAN
    const double* pdData = (const double*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        double dTemp = *pdData++;
        SwapBytes(8,&dTemp);
        fwrite(&dTemp,8,1,pkFile);
    }
#else
    fwrite(pvData,8,iQuantity,pkFile);
#endif
    return 8*iQuantity;
}
//----------------------------------------------------------------------------
int System::Read2be (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 2*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read4be (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 4*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read8be (const char* acBuffer, int iQuantity, void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 8*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(pvData,uiSize,acBuffer,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,pvData);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write2be (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 2*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write4be (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 4*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Write8be (char* acBuffer, int iQuantity, const void* pvData)
{
    assert(acBuffer && iQuantity > 0 && pvData);
    int iNumBytes = 8*iQuantity;
    size_t uiSize = (size_t)iNumBytes;
    Memcpy(acBuffer,uiSize,pvData,uiSize);
#ifndef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,acBuffer);
#endif
    return iNumBytes;
}
//----------------------------------------------------------------------------
int System::Read2be (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,2,iQuantity,pkFile); (void)r;
#ifndef WM4_BIG_ENDIAN
    SwapBytes(2,iQuantity,pvData);
#endif
    return 2*iQuantity;
}
//----------------------------------------------------------------------------
int System::Read4be (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,4,iQuantity,pkFile); (void)r;
#ifndef WM4_BIG_ENDIAN
    SwapBytes(4,iQuantity,pvData);
#endif
    return 4*iQuantity;
}
//----------------------------------------------------------------------------
int System::Read8be (FILE* pkFile, int iQuantity, void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
    size_t r = fread(pvData,8,iQuantity,pkFile); (void)r;
#ifndef WM4_BIG_ENDIAN
    SwapBytes(8,iQuantity,pvData);
#endif
    return 8*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write2be (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifndef WM4_BIG_ENDIAN
    const short* psData = (const short*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        short sTemp = *psData++;
        SwapBytes(2,&sTemp);
        fwrite(&sTemp,2,1,pkFile);
    }
#else
    fwrite(pvData,2,iQuantity,pkFile);
#endif
    return 2*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write4be (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifndef WM4_BIG_ENDIAN
    const int* piData = (const int*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        int iTemp = *piData++;
        SwapBytes(4,&iTemp);
        fwrite(&iTemp,4,1,pkFile);
    }
#else
    fwrite(pvData,4,iQuantity,pkFile);
#endif
    return 4*iQuantity;
}
//----------------------------------------------------------------------------
int System::Write8be (FILE* pkFile, int iQuantity, const void* pvData)
{
    assert(pkFile && iQuantity > 0 && pvData);
#ifndef WM4_BIG_ENDIAN
    const double* pdData = (const double*)pvData;
    for (int i = 0; i < iQuantity; i++)
    {
        double dTemp = *pdData++;
        SwapBytes(8,&dTemp);
        fwrite(&dTemp,8,1,pkFile);
    }
#else
    fwrite(pvData,8,iQuantity,pkFile);
#endif
    return 8*iQuantity;
}
//----------------------------------------------------------------------------
const char* System::GetPath (const char* acDirectory, const char* acFilename)
{
    // #0000656: WildMagic4 doesn't build on 64-bit Mac OS
#if defined(__APPLE__) && !defined(__aarch64__) && !defined(__x86_64__)
    // An application-relative path is needed for the applications to be able
    // to find the input data sets.  Unfortunately, there is no exact way to
    // predict which directory the application is run from, since this depends
    // on whether it was launched by the Finder, Xcode, gdb, the console, etc.
    // To work around this, the following code switches to the application
    // directory.  Beware of your applications themselves changing directory,
    // which could cause the input data sets not to be found.

    // WARNING.  The engine supports XCode 2.x, but not XCode 1.x.  The
    // directory structure assumed here is based on the default Xcode 2.x
    // preferences:
    //     "Put build products in project directory"
    //     "Put intermediate build files with build products"
    // If you change the Xcode preferences, you will need to modify this code
    // to correctly locate the application directory.

    OSStatus eError;
    FSRef kExeRef;    // ${ApplicationPath}/build/WildMagic/executableFile
    FSRef kWMRef;     // ${ApplicationPath}/build/WildMagic
    FSRef kBuildRef;  // ${ApplicationPath}/build
    FSRef kAppRef;    // ${ApplicationPath}
    ProcessSerialNumber kPSN;

    MacGetCurrentProcess(&kPSN);
    eError = GetProcessBundleLocation(&kPSN,&kExeRef);
    eError = FSGetCatalogInfo(&kExeRef,0,0,0,0,&kWMRef);
    eError = FSGetCatalogInfo(&kWMRef,0,0,0,0,&kBuildRef);
    eError = FSGetCatalogInfo(&kBuildRef,0,0,0,0,&kAppRef);
    eError = FSRefMakePath(&kAppRef,(UInt8*)ms_acPath,SYSTEM_MAX_PATH);
    eError = chdir(ms_acPath);

#if 0
    // If you really must use Xcode version 1.x, then the path searching
    // code must be the following.
    OSStatus eError;
    FSRef kExeRef;    // ${ApplicationPath}/build/executableFile
    FSRef kBuildRef;  // ${ApplicationPath}/build
    FSRef kAppRef;    // ${ApplicationPath}
    ProcessSerialNumber kPSN;

    MacGetCurrentProcess(&kPSN);
    eError = GetProcessBundleLocation(&kPSN,&kExeRef);
    eError = FSGetCatalogInfo(&kExeRef,0,0,0,0,&kBuildRef);
    eError = FSGetCatalogInfo(&kBuildRef,0,0,0,0,&kAppRef);
    eError = FSRefMakePath(&kAppRef,(UInt8*)ms_acPath,SYSTEM_MAX_PATH);
    eError = chdir(ms_acPath);
#endif
#endif

    size_t uiDLength = strlen(acDirectory);
    size_t uiFLength = strlen(acFilename);
    if (uiDLength + uiFLength + 1 <= SYSTEM_MAX_PATH)
    {
        System::Strcpy(ms_acPath,SYSTEM_MAX_PATH,acDirectory);
        System::Strcat(ms_acPath,SYSTEM_MAX_PATH,acFilename);
        return ms_acPath;
    }
    return nullptr;
}
//----------------------------------------------------------------------------
void System::Initialize ()
{
    assert(!ms_pkDirectories);
    ms_pkDirectories = WM4_NEW std::vector<std::string>;

    const char* acWm4Path = GetEnv("WM4_PATH");
    if (acWm4Path)
    {
        Strcpy(WM4_PATH,SYSTEM_MAX_ENVVAR,acWm4Path);
    }
    else
    {
        WM4_PATH[0] = 0;
    }
}
//----------------------------------------------------------------------------
void System::Terminate ()
{
    WM4_DELETE ms_pkDirectories;
    ms_pkDirectories = nullptr;
}
//----------------------------------------------------------------------------
int System::GetDirectoryQuantity ()
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    return (int)ms_pkDirectories->size();
}
//----------------------------------------------------------------------------
const char* System::GetDirectory (int i)
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    if (0 <= i && i < (int)ms_pkDirectories->size())
    {
        return (*ms_pkDirectories)[i].c_str();
    }
    return nullptr;
}
//----------------------------------------------------------------------------
bool System::InsertDirectory (const char* acDirectory)
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    std::string kDirectory = std::string(acDirectory) + std::string("/");
    for (int i = 0; i < (int)ms_pkDirectories->size(); i++)
    {
        if (kDirectory == (*ms_pkDirectories)[i])
        {
            return false;
        }
    }
    ms_pkDirectories->push_back(kDirectory);
    return true;
}
//----------------------------------------------------------------------------
bool System::RemoveDirectory (const char* acDirectory)
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    std::string kDirectory = std::string(acDirectory) + std::string("/");
    std::vector<std::string>::iterator pkIter = ms_pkDirectories->begin();
    for (/**/; pkIter != ms_pkDirectories->end(); pkIter++)
    {
        if (kDirectory == *pkIter)
        {
            ms_pkDirectories->erase(pkIter);
            return true;
        }
    }
    return false;
}
//----------------------------------------------------------------------------
void System::RemoveAllDirectories ()
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    ms_pkDirectories->clear();
}
//----------------------------------------------------------------------------
const char* System::GetPath (const char* acFilename, int eMode)
{
    if (!ms_pkDirectories)
    {
        Initialize();
    }

    for (int i = 0; i < (int)ms_pkDirectories->size(); i++)
    {
        const char* acDecorated = System::GetPath(
            (*ms_pkDirectories)[i].c_str(),acFilename);
        if (!acDecorated)
        {
            return nullptr;
        }

        FILE* pkFile;
        if (eMode == SM_READ)
        {
            pkFile = System::Fopen(acDecorated,"r");
        }
        else if (eMode == SM_WRITE)
        {
            pkFile = System::Fopen(acDecorated,"w");
        }
        else // eMode == SM_READ_WRITE
        {
            pkFile = System::Fopen(acDecorated,"r+");
        }

        if (pkFile)
        {
            System::Fclose(pkFile);
            return acDecorated;
        }
    }
    return nullptr;
}
//----------------------------------------------------------------------------
unsigned int System::MakeRGB (unsigned char ucR, unsigned char ucG,
    unsigned char ucB)
{
#ifdef WM4_BIG_ENDIAN
    return (0xFF | (ucB << 8) | (ucG << 16) | (ucR << 24));
#else
    return (ucR | (ucG << 8) | (ucB << 16) | (0xFF << 24));
#endif
}
//----------------------------------------------------------------------------
unsigned int System::MakeRGBA (unsigned char ucR, unsigned char ucG,
    unsigned char ucB, unsigned char ucA)
{
#ifdef WM4_BIG_ENDIAN
    return (ucA | (ucB << 8) | (ucG << 16) | (ucR << 24));
#else
    return (ucR | (ucG << 8) | (ucB << 16) | (ucA << 24));
#endif
}
//----------------------------------------------------------------------------
FILE* System::Fopen (const char* acFilename, const char* acMode)
{
#ifdef WM4_USING_VC80
    FILE* pkFile;
    errno_t uiError = fopen_s(&pkFile,acFilename,acMode);
    if (uiError == 0)
    {
        return pkFile;
    }
    else
    {
        return 0;
    }
#else
    return fopen(acFilename,acMode);
#endif
}
//----------------------------------------------------------------------------
int System::Fprintf (FILE* pkFile, const char* acFormat, ...)
{
    if (!pkFile || !acFormat)
    {
        return -1;
    }

    va_list acArgs;
    va_start(acArgs,acFormat);

#ifdef WM4_USING_VC80
    int iNumWritten = vfprintf_s(pkFile,acFormat,acArgs);
#else
    int iNumWritten = vfprintf(pkFile,acFormat,acArgs);
#endif

    va_end(acArgs);
    return iNumWritten;
}
//----------------------------------------------------------------------------
int System::Fclose (FILE* pkFile)
{
    return fclose(pkFile);
}
//----------------------------------------------------------------------------
const char* System::GetEnv (const char* acEnvVarName)
{
#ifdef WM4_USING_VC80
   size_t uiRequiredSize;
   errno_t uiError = getenv_s(&uiRequiredSize,0,0,acEnvVarName);
   if (uiError > 0)
   {
       return 0;
   }
   getenv_s(&uiRequiredSize,ms_acEnvVar,SYSTEM_MAX_ENVVAR,acEnvVarName);
#else
    char* acEnvVar = getenv(acEnvVarName);
    if (!acEnvVar)
    {
        return nullptr;
    }
    System::Strcpy(ms_acEnvVar,SYSTEM_MAX_ENVVAR,getenv(acEnvVarName));
#endif
    return ms_acEnvVar;
}
//----------------------------------------------------------------------------
void* System::Memcpy (void* pvDst, size_t uiDstSize, const void* pvSrc,
    size_t uiSrcSize)
{
#ifdef WM4_USING_VC80
    errno_t uiError = memcpy_s(pvDst,uiDstSize,pvSrc,uiSrcSize);
    if (uiError == 0)
    {
        return pvDst;
    }
    else
    {
        return 0;
    }
#else
    if (!pvDst || uiDstSize == 0 || !pvSrc || uiSrcSize == 0)
    {
        // Be consistent with the behavior of memcpy_s.
        return nullptr;
    }

    if (uiSrcSize > uiDstSize)
    {
        // The source memory is too large to copy to the destination.  To
        // be consistent with memcpy_s, return null as an indication that the
        // copy failed.
        return nullptr;
    }
    memcpy(pvDst,pvSrc,uiSrcSize);
    return pvDst;
#endif
}
//----------------------------------------------------------------------------
int System::Sprintf (char* acDst, size_t uiDstSize, const char* acFormat, ...)
{
    if (!acDst || uiDstSize == 0 || !acFormat)
    {
        return -1;
    }

    va_list acArgs;
    va_start(acArgs,acFormat);

#ifdef WM4_USING_VC80
    int iNumWritten = vsprintf_s(acDst,uiDstSize,acFormat,acArgs);
#else
    int iNumWritten = vsprintf(acDst,acFormat,acArgs);
#endif

    va_end(acArgs);
    return iNumWritten;
}
//----------------------------------------------------------------------------
char* System::Strcpy (char* acDst, size_t uiDstSize, const char* acSrc)
{
#ifdef WM4_USING_VC80
    errno_t uiError = strcpy_s(acDst,uiDstSize,acSrc);
    if (uiError == 0)
    {
        return acDst;
    }
    else
    {
        return 0;
    }
#else
    if (!acDst || uiDstSize == 0 || !acSrc)
    {
        // Be consistent with the behavior of strcpy_s.
        return nullptr;
    }

    size_t uiSrcLen = strlen(acSrc);
    if (uiSrcLen + 1 > uiDstSize)
    {
        // The source string is too large to copy to the destination.  To
        // be consistent with strcpy_s, return null as an indication that the
        // copy failed.
        return nullptr;
    }
    strncpy(acDst,acSrc,uiSrcLen);
    acDst[uiSrcLen] = 0;
    return acDst;
#endif
}
//----------------------------------------------------------------------------
char* System::Strcat (char* acDst, size_t uiDstSize, const char* acSrc)
{
#ifdef WM4_USING_VC80
    errno_t uiError = strcat_s(acDst,uiDstSize,acSrc);
    if (uiError == 0)
    {
        return acDst;
    }
    else
    {
        return 0;
    }
#else
    if (!acDst || uiDstSize == 0 || !acSrc)
    {
        // Be consistent with the behavior of strcat_s.
        return nullptr;
    }

    size_t uiSrcLen = strlen(acSrc);
    size_t uiDstLen = strlen(acDst);
    size_t uiSumLen = uiSrcLen + uiDstLen;
    if (uiSumLen + 1 > uiDstSize)
    {
        // The source string is too large to append to the destination.  To
        // be consistent with strcat_s, return null as an indication that
        // the concatenation failed.
        return nullptr;
    }
    strncat(acDst,acSrc,uiSrcLen);
    acDst[uiSumLen] = 0;
    return acDst;
#endif
}
//----------------------------------------------------------------------------
char* System::Strncpy (char* acDst, size_t uiDstSize, const char* acSrc,
    size_t uiSrcSize)
{
#ifdef WM4_USING_VC80
    errno_t uiError = strncpy_s(acDst,uiDstSize,acSrc,uiSrcSize);
    if (uiError == 0)
    {
        return acDst;
    }
    else
    {
        return 0;
    }
#else
    if (!acDst || uiDstSize == 0 || !acSrc || uiSrcSize == 0)
    {
        // Be consistent with the behavior of strncpy_s.
        return nullptr;
    }

    if (uiSrcSize + 1 > uiDstSize)
    {
        // The source string is too large to copy to the destination.  To
        // be consistent with strncpy_s, return null as an indication that
        // the copy failed.
        return nullptr;
    }
    strncpy(acDst,acSrc,uiSrcSize);
    return acDst;
#endif
}
//----------------------------------------------------------------------------
char* System::Strtok (char* acToken, const char* acDelimiters,
    char*& racNextToken)
{
#ifdef WM4_USING_VC80
    return strtok_s(acToken,acDelimiters,&racNextToken);
#else
    (void)racNextToken;  // avoid warning in release build
    return strtok(acToken,acDelimiters);
#endif
}
//----------------------------------------------------------------------------
