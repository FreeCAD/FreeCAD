// IExtractImage.h
// content taken by shobjidl.h
// Philip Sakellaropoulos 2002

#ifndef __IExtractImage_FWD_DEFINED__
#define __IExtractImage_FWD_DEFINED__

//BB2E617C-0920-11d1-9A0B-00C04FC2D6C1
DEFINE_GUID(IID_IExtractImage, 
0xBB2E617C, 0x0920, 0x11d1, 0x9A, 0x0B, 0x00, 0xC0, 0x4F, 0xC2, 0xD6, 0xC1);

#define IEIFLAG_ASYNC       0x0001      // ask the extractor if it supports ASYNC extract (free threaded)
#define IEIFLAG_CACHE       0x0002      // returned from the extractor if it does NOT cache the thumbnail
#define IEIFLAG_ASPECT      0x0004      // passed to the extractor to beg it to render to the aspect ratio of the supplied rect
#define IEIFLAG_OFFLINE     0x0008      // if the extractor shouldn't hit the net to get any content neede for the rendering
#define IEIFLAG_GLEAM       0x0010      // does the image have a gleam ? this will be returned if it does
#define IEIFLAG_SCREEN      0x0020      // render as if for the screen  (this is exlusive with IEIFLAG_ASPECT )
#define IEIFLAG_ORIGSIZE    0x0040      // render to the approx size passed, but crop if neccessary
#define IEIFLAG_NOSTAMP     0x0080      // returned from the extractor if it does NOT want an icon stamp on the thumbnail
#define IEIFLAG_NOBORDER    0x0100      // returned from the extractor if it does NOT want an a border around the thumbnail
#define IEIFLAG_QUALITY     0x0200      // passed to the Extract method to indicate that a slower, higher quality image is desired, re-compute the thumbnail
#define IEIFLAG_REFRESH     0x0400      // returned from the extractor if it would like to have Refresh Thumbnail available

//#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BB2E617C-0920-11d1-9A0B-00C04FC2D6C1")
    IExtractImage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetLocation( 
            /* [size_is][out] */ LPWSTR pszPathBuffer,
            /* [in] */ DWORD cch,
            /* [unique][out][in] */ DWORD *pdwPriority,
            /* [in] */ const SIZE *prgSize,
            /* [in] */ DWORD dwRecClrDepth,
            /* [in] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Extract( 
            /* [out] */ HBITMAP *phBmpThumbnail) = 0;
        
    };

	DEFINE_GUID(IID_IExtractImage2, 
	0x953BB1EE, 0x93B4, 0x11d1, 0x98, 0xA3, 0x00, 0xC0, 0x4F, 0xB6, 0x87, 0xDA);

    MIDL_INTERFACE("953BB1EE-93B4-11d1-98A3-00C04FB687DA")
    IExtractImage2 : public IExtractImage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDateStamp( 
            /* [out] */ FILETIME *pDateStamp) = 0;
        
    };

#endif