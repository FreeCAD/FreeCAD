// SPDX-FileNotice: Part of the FreeCAD project.

import AppKit
import QuickLookThumbnailing
import os.log

private let logger = Logger(
    subsystem: Bundle(for: ThumbnailProvider.self).bundleIdentifier
        ?? "org.freecad.quicklook.thumbnail.fallback",
    category: "ThumbnailProvider"
)

class ThumbnailProvider: QLThumbnailProvider {

    override func provideThumbnail(
        for request: QLFileThumbnailRequest,
        _ handler: @escaping (QLThumbnailReply?, Error?) -> Void
    ) {

        logger.debug(
            "Providing thumbnail for: \(request.fileURL.path)"
        )

        guard
            let cgImage = try? SwiftZIPParser.extractThumbnail(
                from: request.fileURL, maxSize: request.maximumSize)
        else {
            logger.warning("No valid thumbnail found; returning empty reply.")
            handler(nil, nil)
            return
        }

        let reply = QLThumbnailReply(
            contextSize: request.maximumSize,
            currentContextDrawing: { () -> Bool in
                let image = NSImage(cgImage: cgImage, size: request.maximumSize)
                image.draw(in: CGRect(origin: .zero, size: request.maximumSize))
                return true
            }
        )

        handler(reply, nil)
    }
}
