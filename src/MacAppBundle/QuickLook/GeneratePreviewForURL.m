#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

/* -----------------------------------------------------------------------------
 Generate a preview for file

 This function's job is to create preview for designated file
 ----------------------------------------------------------------------------- */


OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
    OSStatus ret = coreFoundationUnknownErr;

    @autoreleasepool {

        // unzip -qq -o -j -d /tmp ~/test.FCStd thumbnails/Thumbnail.png
        //  -qq : be really quiet
        //  -o : overwrite without prompt
        //  -j : don't create archive paths
        //  -d : destination path (create this path)
        // extracts thumbnails/Thumbnail.png to /tmp/Thumbnail.png .
        // We add a UUID and use a system temp directory here.

        // TODO: do we need to release any of these resources?
        NSUUID *uuid = [NSUUID UUID];
        NSString *uuidPath = [uuid UUIDString];
        NSString *unzipDstPath = [NSString stringWithFormat:@"%@/%@/", NSTemporaryDirectory(), uuidPath];
        NSString *unzipDstFile = [NSString stringWithFormat:@"%@%@", unzipDstPath, @"Thumbnail.png"];
        NSURL *zipFile = (__bridge NSURL *)url;
        NSTask *task = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/unzip"
                                                arguments:@[@"-qq", @"-o", @"-j", @"-d", unzipDstPath, [zipFile path], @"thumbnails/Thumbnail.png"]];
        [task waitUntilExit];
//        NSLog(@"%@", unzipDstPath);
//        NSLog(@"%@", unzipDstFile);

        if ( [[NSFileManager defaultManager] fileExistsAtPath:unzipDstFile] )
        {
            // Preview will be drawn in a vectorized context
            CGSize canvasSize = CGSizeMake(512, 512);
            CGContextRef cgContext = QLPreviewRequestCreateContext(preview, canvasSize, true, NULL);
            if(cgContext)
            {
                CGDataProviderRef pngDP = CGDataProviderCreateWithFilename([unzipDstFile fileSystemRepresentation]);
                CGImageRef image = CGImageCreateWithPNGDataProvider(pngDP, NULL, true, kCGRenderingIntentDefault);

                CGContextDrawImage(cgContext,CGRectMake(0, 0, 512, 512), image);

                QLPreviewRequestFlushContext(preview, cgContext);
                ret = noErr;

                CFRelease(cgContext);
                CFRelease(image);
            }
        }

        if ( [[NSFileManager defaultManager] fileExistsAtPath:unzipDstFile] )
            [[NSFileManager defaultManager] removeItemAtPath:unzipDstFile error:nil];

        if ( [[NSFileManager defaultManager] fileExistsAtPath:unzipDstPath] )
            [[NSFileManager defaultManager] removeItemAtPath:unzipDstPath error:nil];
    }

    return ret;
}


void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview)
{
    // implement only if supported
}
