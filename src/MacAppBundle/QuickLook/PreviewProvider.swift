import AppKit
import Compression
import CoreGraphics
import Foundation
import ImageIO
import QuickLookUI
import os.log

private let logger = Logger(
    subsystem: Bundle(for: PreviewProvider.self).bundleIdentifier
        ?? "org.freecad.quicklook.fallback",
    category: "PreviewProvider"
)

class PreviewProvider: QLPreviewProvider {

    func providePreview(for request: QLFilePreviewRequest) async throws -> QLPreviewReply {
        logger.debug(
            "Providing preview for: \(request.fileURL.path)"
        )

        do {
            let cgImage = try SwiftZIPParser.extractThumbnail(from: request.fileURL)

            let imageSize = CGSize(
                width: CGFloat(cgImage.width),
                height: CGFloat(cgImage.height)
            )

            logger.debug(
                "Preview image size: \(imageSize.width)x\(imageSize.height)"
            )

            // Ensure valid dimensions
            guard imageSize.width > 0 && imageSize.height > 0 else {
                logger.error("Invalid image dimensions: \(imageSize.width)x\(imageSize.height)")
                throw PreviewError.invalidImageDimensions
            }

            let reply = QLPreviewReply(contextSize: imageSize, isBitmap: true) { context, _ in
                logger.debug("Drawing preview image in context")

                // Draw the extracted thumbnail image
                context.draw(cgImage, in: CGRect(origin: .zero, size: imageSize))

                logger.debug("Preview image drawn successfully")
                return
            }

            logger.debug("QLPreviewReply created successfully")
            return reply

        } catch {
            logger.error(
                "Failed to provide preview: \(error.localizedDescription)"
            )
            throw error
        }
    }
}

// MARK: - Preview Error Types

enum PreviewError: Error, LocalizedError {
    case invalidImageDimensions
    case extractionFailed

    var errorDescription: String? {
        switch self {
        case .invalidImageDimensions:
            return "Invalid image dimensions for preview"
        case .extractionFailed:
            return "Failed to extract thumbnail for preview"
        }
    }
}

// MARK: - ZIP File Format Constants

private struct ZIPConstants {
    static let localFileSignature: UInt32 = 0x0403_4b50
    static let centralDirSignature: UInt32 = 0x0201_4b50
    static let endOfCentralDirSignature: UInt32 = 0x0605_4b50

    static let compressionStored: UInt16 = 0
    static let compressionDeflate: UInt16 = 8
}

// MARK: - ZIP Structures

private struct ZIPLocalFileHeader {
    let signature: UInt32
    let version: UInt16
    let flags: UInt16
    let compression: UInt16
    let modTime: UInt16
    let modDate: UInt16
    let crc32: UInt32
    let compressedSize: UInt32
    let uncompressedSize: UInt32
    let filenameLength: UInt16
    let extraFieldLength: UInt16

    static let size = 30
}

private struct ZIPCentralDirHeader {
    let signature: UInt32
    let versionMadeBy: UInt16
    let versionNeeded: UInt16
    let flags: UInt16
    let compression: UInt16
    let modTime: UInt16
    let modDate: UInt16
    let crc32: UInt32
    let compressedSize: UInt32
    let uncompressedSize: UInt32
    let filenameLength: UInt16
    let extraFieldLength: UInt16
    let commentLength: UInt16
    let diskNumber: UInt16
    let internalAttributes: UInt16
    let externalAttributes: UInt32
    let localHeaderOffset: UInt32

    static let size = 46
}

private struct ZIPEndOfCentralDir {
    let signature: UInt32
    let diskNumber: UInt16
    let centralDirDisk: UInt16
    let entriesOnDisk: UInt16
    let totalEntries: UInt16
    let centralDirSize: UInt32
    let centralDirOffset: UInt32
    let commentLength: UInt16

    static let size = 22
}

// MARK: - Error Types

enum ZIPParserError: Error, LocalizedError {
    case fileNotFound
    case invalidZipFile
    case corruptedZipFile
    case thumbnailNotFound
    case compressionUnsupported
    case decompressionFailed

    var errorDescription: String? {
        switch self {
        case .fileNotFound:
            return "FCStd file not found"
        case .invalidZipFile:
            return "Invalid ZIP file format"
        case .corruptedZipFile:
            return "Corrupted ZIP file"
        case .thumbnailNotFound:
            return "No thumbnail found in FCStd file"
        case .compressionUnsupported:
            return "Unsupported compression method"
        case .decompressionFailed:
            return "Failed to decompress file data"
        }
    }
}

// MARK: - Pure Swift ZIP Parser

struct SwiftZIPParser {

    /// Extract thumbnail from FCStd file
    static func extractThumbnail(from fileURL: URL) throws -> CGImage {
        logger.debug("Extracting thumbnail from file: \(fileURL.path)")

        // Handle security scoped resources
        let didStartAccessing = fileURL.startAccessingSecurityScopedResource()
        defer {
            if didStartAccessing {
                fileURL.stopAccessingSecurityScopedResource()
                logger.debug("Stopped accessing security scoped resource")
            }
        }

        // Read file data
        let zipData: Data
        do {
            zipData = try Data(contentsOf: fileURL, options: .mappedIfSafe)
        } catch {
            logger.error(
                "Failed to read file data: \(error.localizedDescription)")
            throw ZIPParserError.fileNotFound
        }

        return try extractThumbnail(from: zipData)
    }

    /// Extract thumbnail from ZIP data
    static func extractThumbnail(from zipData: Data) throws -> CGImage {
        guard zipData.count >= ZIPLocalFileHeader.size else {
            logger.error("ZIP data too small")
            throw ZIPParserError.invalidZipFile
        }

        // Verify ZIP signature
        let signature = zipData.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self) }
        guard signature == ZIPConstants.localFileSignature else {
            logger.error("Invalid ZIP signature: 0x\(String(signature, radix: 16))")
            throw ZIPParserError.invalidZipFile
        }

        // Find end of central directory
        guard let endOfCentralDir = findEndOfCentralDirectory(in: zipData) else {
            logger.error("Could not find end of central directory")
            throw ZIPParserError.corruptedZipFile
        }

        // Look for thumbnail files
        let thumbnailPaths = ["thumbnails/Thumbnail.png", "Thumbnail.png"]

        for thumbnailPath in thumbnailPaths {
            if let thumbnailData = try? extractFile(
                from: zipData,
                endOfCentralDir: endOfCentralDir,
                filename: thumbnailPath
            ) {
                logger.debug("Found thumbnail at path: \(thumbnailPath)")

                if let image = createImage(from: thumbnailData) {
                    logger.debug("Successfully created CGImage from thumbnail data")
                    return image
                }
            }
        }

        logger.info("No thumbnail found in FCStd file")
        throw ZIPParserError.thumbnailNotFound
    }

    /// Validate if file is a valid FCStd (ZIP) file
    static func isValidFCStdFile(at url: URL) -> Bool {
        guard let headerData = try? Data(contentsOf: url, options: .uncached),
            headerData.count >= 4
        else {
            return false
        }

        let signature = headerData.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self) }
        return signature == ZIPConstants.localFileSignature
    }
}

// MARK: - Private ZIP Parsing Methods

extension SwiftZIPParser {

    /// Find the end of central directory record
    fileprivate static func findEndOfCentralDirectory(in data: Data) -> ZIPEndOfCentralDir? {
        let dataCount = data.count
        guard dataCount >= ZIPEndOfCentralDir.size else { return nil }

        let searchStart = dataCount - ZIPEndOfCentralDir.size
        let maxSearch = min(searchStart, 65535 + ZIPEndOfCentralDir.size)  // Max comment length

        for i in 0...maxSearch {
            let pos = searchStart - i
            guard pos + ZIPEndOfCentralDir.size <= dataCount else { continue }

            let signature = data.subdata(in: pos..<(pos + 4))
                .withUnsafeBytes { $0.loadUnaligned(as: UInt32.self) }

            if signature == ZIPConstants.endOfCentralDirSignature {
                return parseEndOfCentralDir(from: data, at: pos)
            }
        }

        return nil
    }

    /// Parse end of central directory structure
    fileprivate static func parseEndOfCentralDir(from data: Data, at offset: Int)
        -> ZIPEndOfCentralDir
    {
        let eocdData = data.subdata(in: offset..<(offset + ZIPEndOfCentralDir.size))

        return ZIPEndOfCentralDir(
            signature: readUInt32(from: eocdData, at: 0),
            diskNumber: readUInt16(from: eocdData, at: 4),
            centralDirDisk: readUInt16(from: eocdData, at: 6),
            entriesOnDisk: readUInt16(from: eocdData, at: 8),
            totalEntries: readUInt16(from: eocdData, at: 10),
            centralDirSize: readUInt32(from: eocdData, at: 12),
            centralDirOffset: readUInt32(from: eocdData, at: 16),
            commentLength: readUInt16(from: eocdData, at: 20)
        )
    }

    /// Extract a specific file from the ZIP data
    fileprivate static func extractFile(
        from zipData: Data,
        endOfCentralDir: ZIPEndOfCentralDir,
        filename: String
    ) throws -> Data {

        let filenameData = filename.data(using: .utf8)!
        let centralDirOffset = Int(endOfCentralDir.centralDirOffset)
        let totalEntries = Int(endOfCentralDir.totalEntries)

        var currentOffset = centralDirOffset

        // Parse central directory entries
        for _ in 0..<totalEntries {
            guard currentOffset + ZIPCentralDirHeader.size <= zipData.count else {
                break
            }

            let centralHeader = parseCentralDirHeader(from: zipData, at: currentOffset)

            guard centralHeader.signature == ZIPConstants.centralDirSignature else {
                break
            }

            // Check if this is our target file
            let filenameOffset = currentOffset + ZIPCentralDirHeader.size
            let filenameLength = Int(centralHeader.filenameLength)

            guard filenameOffset + filenameLength <= zipData.count else {
                throw ZIPParserError.corruptedZipFile
            }

            let entryFilenameData = zipData.subdata(
                in: filenameOffset..<(filenameOffset + filenameLength))

            if entryFilenameData == filenameData {
                // Found our file, extract it
                return try extractFileData(from: zipData, centralHeader: centralHeader)
            }

            // Move to next central directory entry
            currentOffset +=
                ZIPCentralDirHeader.size + Int(centralHeader.filenameLength)
                + Int(centralHeader.extraFieldLength) + Int(centralHeader.commentLength)
        }

        throw ZIPParserError.thumbnailNotFound
    }

    /// Parse central directory header structure
    fileprivate static func parseCentralDirHeader(from data: Data, at offset: Int)
        -> ZIPCentralDirHeader
    {
        let headerData = data.subdata(in: offset..<(offset + ZIPCentralDirHeader.size))

        return ZIPCentralDirHeader(
            signature: readUInt32(from: headerData, at: 0),
            versionMadeBy: readUInt16(from: headerData, at: 4),
            versionNeeded: readUInt16(from: headerData, at: 6),
            flags: readUInt16(from: headerData, at: 8),
            compression: readUInt16(from: headerData, at: 10),
            modTime: readUInt16(from: headerData, at: 12),
            modDate: readUInt16(from: headerData, at: 14),
            crc32: readUInt32(from: headerData, at: 16),
            compressedSize: readUInt32(from: headerData, at: 20),
            uncompressedSize: readUInt32(from: headerData, at: 24),
            filenameLength: readUInt16(from: headerData, at: 28),
            extraFieldLength: readUInt16(from: headerData, at: 30),
            commentLength: readUInt16(from: headerData, at: 32),
            diskNumber: readUInt16(from: headerData, at: 34),
            internalAttributes: readUInt16(from: headerData, at: 36),
            externalAttributes: readUInt32(from: headerData, at: 38),
            localHeaderOffset: readUInt32(from: headerData, at: 42)
        )
    }

    /// Extract file data using local header information
    fileprivate static func extractFileData(from zipData: Data, centralHeader: ZIPCentralDirHeader)
        throws -> Data
    {
        let localHeaderOffset = Int(centralHeader.localHeaderOffset)

        guard localHeaderOffset + ZIPLocalFileHeader.size <= zipData.count else {
            throw ZIPParserError.corruptedZipFile
        }

        let localHeader = parseLocalFileHeader(from: zipData, at: localHeaderOffset)

        guard localHeader.signature == ZIPConstants.localFileSignature else {
            throw ZIPParserError.corruptedZipFile
        }

        // Calculate data offset
        let dataOffset =
            localHeaderOffset + ZIPLocalFileHeader.size + Int(localHeader.filenameLength)
            + Int(localHeader.extraFieldLength)

        guard dataOffset + Int(localHeader.compressedSize) <= zipData.count else {
            throw ZIPParserError.corruptedZipFile
        }

        let compressedData = zipData.subdata(
            in: dataOffset..<(dataOffset + Int(localHeader.compressedSize)))

        // Handle different compression methods
        switch localHeader.compression {
        case ZIPConstants.compressionStored:
            return compressedData

        case ZIPConstants.compressionDeflate:
            return try deflateDecompress(
                data: compressedData,
                expectedSize: Int(localHeader.uncompressedSize)
            )

        default:
            logger.debug("Unsupported compression method: \(localHeader.compression)")
            throw ZIPParserError.compressionUnsupported
        }
    }

    /// Parse local file header structure
    fileprivate static func parseLocalFileHeader(from data: Data, at offset: Int)
        -> ZIPLocalFileHeader
    {
        let headerData = data.subdata(in: offset..<(offset + ZIPLocalFileHeader.size))

        return ZIPLocalFileHeader(
            signature: readUInt32(from: headerData, at: 0),
            version: readUInt16(from: headerData, at: 4),
            flags: readUInt16(from: headerData, at: 6),
            compression: readUInt16(from: headerData, at: 8),
            modTime: readUInt16(from: headerData, at: 10),
            modDate: readUInt16(from: headerData, at: 12),
            crc32: readUInt32(from: headerData, at: 14),
            compressedSize: readUInt32(from: headerData, at: 18),
            uncompressedSize: readUInt32(from: headerData, at: 22),
            filenameLength: readUInt16(from: headerData, at: 26),
            extraFieldLength: readUInt16(from: headerData, at: 28)
        )
    }

    /// Decompress deflate compressed data using Swift Compression framework
    fileprivate static func deflateDecompress(data: Data, expectedSize: Int) throws -> Data {
        guard !data.isEmpty else {
            throw ZIPParserError.decompressionFailed
        }

        // Allocate output buffer
        var outputBuffer = Data(count: expectedSize)

        let actualSize = try data.withUnsafeBytes { inputBytes in
            try outputBuffer.withUnsafeMutableBytes { outputBytes in
                let inputPtr = inputBytes.bindMemory(to: UInt8.self)
                let outputPtr = outputBytes.bindMemory(to: UInt8.self)

                // Use compression_decode_buffer for raw deflate data
                let result = compression_decode_buffer(
                    outputPtr.baseAddress!, expectedSize,
                    inputPtr.baseAddress!, data.count,
                    nil, COMPRESSION_ZLIB
                )

                guard result > 0 else {
                    throw ZIPParserError.decompressionFailed
                }

                return result
            }
        }

        // Adjust size if needed
        if actualSize != expectedSize {
            logger.debug("Decompressed size mismatch: expected \(expectedSize), got \(actualSize)")
            outputBuffer = outputBuffer.prefix(actualSize)
        }

        logger.debug("Successfully decompressed \(data.count) bytes to \(outputBuffer.count) bytes")
        return outputBuffer
    }

    /// Create CGImage from PNG data
    fileprivate static func createImage(from pngData: Data) -> CGImage? {
        guard !pngData.isEmpty else { return nil }

        guard let dataProvider = CGDataProvider(data: pngData as CFData) else {
            return nil
        }

        let image = CGImage(
            pngDataProviderSource: dataProvider,
            decode: nil,
            shouldInterpolate: true,
            intent: .defaultIntent
        )

        if image == nil {
            logger.error("Failed to create CGImage from PNG data")
        }

        return image
    }

    // MARK: - Helper Functions for Safe Memory Access

    /// Safely read UInt32 from Data at offset
    fileprivate static func readUInt32(from data: Data, at offset: Int) -> UInt32 {
        let subdata = data.subdata(in: offset..<(offset + 4))
        return subdata.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self) }
    }

    /// Safely read UInt16 from Data at offset
    fileprivate static func readUInt16(from data: Data, at offset: Int) -> UInt16 {
        let subdata = data.subdata(in: offset..<(offset + 2))
        return subdata.withUnsafeBytes { $0.loadUnaligned(as: UInt16.self) }
    }
}
