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


#ifndef FC_STD_EXTRACTOR_H
#define FC_STD_EXTRACTOR_H

#include "resource.h"               // main symbols
#include <shlguid.h>
#include <AtlCom.h>
#include <shlobj.h>
#include "IExtractImage.h"

/////////////////////////////////////////////////////////////////////////////
// CFCStdExtractor
class ATL_NO_VTABLE CFCStdExtractor : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFCStdExtractor, &CLSID_FCStdExtractor>,
    public IPersistFile,
    public IExtractImage2
{
public:
    CFCStdExtractor()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_THUMBSCB)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFCStdExtractor)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IExtractImage)
    COM_INTERFACE_ENTRY(IExtractImage2)
END_COM_MAP()

// IExtractImage
public:

    STDMETHOD(GetLocation)(LPWSTR pszPathBuffer,
                           DWORD cchMax,
                           DWORD *pdwPriority,
                           const SIZE *prgSize,
                           DWORD dwRecClrDepth,
                           DWORD *pdwFlags);
    STDMETHOD(Extract)(HBITMAP*);
    // IExtractImage2
    STDMETHOD(GetDateStamp)(FILETIME *pDateStamp);

    // IPersistFile
    STDMETHOD(Load)(LPCOLESTR wszFile, DWORD dwMode);

    STDMETHOD(GetClassID)(LPCLSID clsid)
    { MessageBox(0,"GetClassID",0,0);
    return E_NOTIMPL;	}

    STDMETHOD(IsDirty)(VOID)
    { MessageBox(0,"IsDirty",0,0);
    return E_NOTIMPL; }

    STDMETHOD(Save)(LPCOLESTR, BOOL)
    { MessageBox(0,"Save",0,0);
    return E_NOTIMPL; }

    STDMETHOD(SaveCompleted)(LPCOLESTR)
    { MessageBox(0,"SaveCompleted",0,0);
    return E_NOTIMPL; }

    STDMETHOD(GetCurFile)(LPOLESTR FAR*)
    { MessageBox(0,"GetCurFile",0,0);
    return E_NOTIMPL; }

private:
    bool CheckZip() const;
    SIZE m_bmSize;
    HBITMAP m_hPreview;
    TCHAR m_szFile[1000];
};

#endif //FC_STD_EXTRACTOR_H
