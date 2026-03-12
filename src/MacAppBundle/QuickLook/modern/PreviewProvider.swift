// SPDX-FileNotice: Part of the FreeCAD project.

import Cocoa
import Quartz
import UniformTypeIdentifiers
import os.log

private let logger = Logger(
    subsystem: Bundle(for: PreviewProvider.self).bundleIdentifier
        ?? "org.freecad.quicklook.fallback",
    category: "PreviewProvider"
)

class PreviewProvider: QLPreviewProvider, QLPreviewingController {

    func providePreview(for request: QLFilePreviewRequest) async throws
        -> QLPreviewReply
    {
        logger.debug(
            "--- PreviewProvider: providePreview CALLED for \(request.fileURL.lastPathComponent) ---"
        )

        let fileURL = request.fileURL
        logger.info("Received file URL: \(fileURL.path)")

        guard let image = try? SwiftZIPParser.extractThumbnail(from: fileURL) else {
            let errorMessage =
                "Failed to extract thumbnail from FreeCAD file: \(fileURL.lastPathComponent)"
            logger.error("\(errorMessage)")
            throw NSError(
                domain: Bundle(for: PreviewProvider.self).bundleIdentifier
                    ?? "org.freecad.quicklook.fallback",
                code: 1001,
                userInfo: [NSLocalizedDescriptionKey: errorMessage]
            )
        }

        logger.info(
            "Thumbnail extracted successfully. Image size: \(image.width)x\(image.height)"
        )

        let imageSize = CGSize(
            width: CGFloat(image.width),
            height: CGFloat(image.height)
        )
        logger.debug(
            "Preview contextSize will be: \(imageSize.width)x\(imageSize.height)"
        )

        // Ensure imageSize is valid and positive
        if imageSize.width <= 0 || imageSize.height <= 0 {
            let errorMessage =
                "Cannot create preview with zero or negative dimensions: \(imageSize) for file: \(fileURL.lastPathComponent)"
            logger.error("\(errorMessage)")
            throw NSError(
                domain: Bundle(for: PreviewProvider.self).bundleIdentifier
                    ?? "org.freecad.quicklook.fallback",
                code: 1003,
                userInfo: [NSLocalizedDescriptionKey: errorMessage]
            )
        }

        let reply = QLPreviewReply(contextSize: imageSize, isBitmap: true) {
            context,
            _ in
            logger.info("Drawing block started. Drawing extracted thumbnail.")

            // Draw the extracted thumbnail image
            context.draw(image, in: CGRect(origin: .zero, size: imageSize))
            logger.debug("Thumbnail image drawn in context.")

            logger.info("Drawing block finished.")
            return
        }

        logger.notice("QLPreviewReply created. Returning reply.")
        return reply
    }
}
