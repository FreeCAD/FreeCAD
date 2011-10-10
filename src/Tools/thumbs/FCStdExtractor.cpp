/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#include "stdafx.h"
#include "resource.h"
#include "ThumbFCStd_i.h"
#include <initguid.h>
#include "ThumbFCStd_i.c"
#include "FCStdExtractor.h"
#include <shlobj.h>

#include <iostream>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipfile.h>
#include <CxImage/xmemfile.h>
#include <CxImage/ximapng.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_FCStdExtractor, CFCStdExtractor)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        _Module.Init(ObjectMap, hInstance, &LIBID_THUMBFCSTDLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CFCStdExtractor

HRESULT CFCStdExtractor::GetLocation(LPWSTR pszPathBuffer,
        DWORD cchMax, DWORD *pdwPriority,
        const SIZE *prgSize, DWORD dwRecClrDepth,
        DWORD *pdwFlags)
{
    m_bmSize = *prgSize;
    if (*pdwFlags & IEIFLAG_ASYNC)	return E_PENDING; 
    return NOERROR;
}

HRESULT CFCStdExtractor::Load(LPCOLESTR wszFile, DWORD dwMode)
{
    USES_CONVERSION;
    _tcscpy(m_szFile, OLE2T((WCHAR*)wszFile)); 
    return S_OK;	
};

bool CFCStdExtractor::CheckZip() const
{
    // open file and check magic number (PK\x03\x04)
    std::ifstream zip(m_szFile, std::ios::in | std::ios::binary);
    unsigned char pk[4] = {0x50, 0x4b, 0x03, 0x04};
    for (int i=0; i<4; i++) {
        unsigned char c;
        if (!zip.get((char&)c))
            return false;
        if (c != pk[i])
            return false;
    }

    return true;
}

// IExtractImage::Extract
HRESULT CFCStdExtractor::Extract(HBITMAP* phBmpThumbnail)
{
    try {
        // first make sure we have a zip file but that might still be invalid
        if (!CheckZip())
            return NOERROR;

        zipios::ZipFile file(m_szFile);
        zipios::ConstEntries files = file.entries();
        // search for a special file name in the project file
        zipios::ConstEntryPointer entry = file.getEntry("thumbnails/Thumbnail.png");
        if (entry && entry->isValid()) {
            // ok, we have found the file. Now, read it in byte for byte
            std::istream *str = file.getInputStream(entry);
            std::vector<unsigned char> content;
            unsigned char c;
            while (str->get((char&)c)) {
                content.push_back(c);
            }

            // pass the memory buffer to CxImage library to create
            // the bitmap handle
            CxMemFile mem(&(content[0]),content.size());
            CxImagePNG png;
            png.Decode(&mem);
            m_hPreview = png.MakeBitmap();
            *phBmpThumbnail = m_hPreview;
        }
    }
    catch(...) {
        // This may happen if the file is corrupted, not a valid zip file
        // or whatever could go wrong
    }

    return NOERROR; 
}

HRESULT CFCStdExtractor::GetDateStamp(FILETIME *pDateStamp)
{
    FILETIME ftCreationTime,ftLastAccessTime,ftLastWriteTime;
    // open the file and get last write time
    HANDLE hFile = CreateFile(m_szFile,GENERIC_READ,FILE_SHARE_READ,NULL,
        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(!hFile) return E_FAIL;
    GetFileTime(hFile,&ftCreationTime,&ftLastAccessTime,&ftLastWriteTime);
    CloseHandle(hFile);
    *pDateStamp = ftLastWriteTime;
    return NOERROR; 
}

