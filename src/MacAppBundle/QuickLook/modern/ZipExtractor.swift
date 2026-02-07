// SPDX-FileNotice: Part of the FreeCAD project.

//
//  ZipExtractor.swift
//  FreeCAD QuickLook Swift Implementation
//
//  Pure Swift ZIP parser for extracting thumbnails from FreeCAD (.FCStd) files
//  This removes external dependencies while maintaining modern Swift APIs
//
//  Created for integration with FreeCAD upstream
//

import Compression
import CoreGraphics
import Foundation
import ImageIO
import os.log

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
    private static let logger = Logger(
        subsystem: Bundle.main.bundleIdentifier ?? "org.freecad.quicklook",
        category: "SwiftZIPParser"
    )

    /// Extract thumbnail from FCStd file
    static func extractThumbnail(from fileURL: URL, maxSize: CGSize? = nil) throws -> CGImage {
        logger.debug("Extracting thumbnail from file: \(fileURL.path)")
        logger.info("=== SwiftZIPParser.extractThumbnail called ===")
        logger.info("File URL: \(fileURL.path)")
        logger.info("File exists: \(FileManager.default.fileExists(atPath: fileURL.path))")
        do {
            let isReachable = try fileURL.checkResourceIsReachable()
            logger.info("File is readable: \(isReachable)")
        } catch {
            logger.info("File is readable: false (error: \(error.localizedDescription))")
        }

        // Handle security scoped resources
        let didStartAccessing = fileURL.startAccessingSecurityScopedResource()
        logger.info("Started accessing security scoped resource: \(didStartAccessing)")
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
            logger.info("Successfully read \(zipData.count) bytes from file")
        } catch {
            logger.error(
                "Failed to read file data: \(error.localizedDescription)")
            logger.error("Failed to read file data: \(error.localizedDescription)")
            throw ZIPParserError.fileNotFound
        }

        return try extractThumbnail(from: zipData, maxSize: maxSize)
    }

    /// Extract thumbnail from ZIP data
    static func extractThumbnail(from zipData: Data, maxSize: CGSize? = nil) throws -> CGImage {
        logger.info("=== Processing ZIP data ===")
        logger.info("ZIP data size: \(zipData.count) bytes")

        guard zipData.count >= ZIPLocalFileHeader.size else {
            logger.error("ZIP data too small")
            logger.error("ZIP data too small: \(zipData.count) < \(ZIPLocalFileHeader.size)")
            throw ZIPParserError.invalidZipFile
        }

        // Verify ZIP signature
        let signature = zipData.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self) }
        logger.info("ZIP signature: 0x\(String(signature, radix: 16))")
        guard signature == ZIPConstants.localFileSignature else {
            logger.error("Invalid ZIP signature: 0x\(String(signature, radix: 16))")
            logger.error(
                "Invalid ZIP signature: 0x\(String(signature, radix: 16)), expected: 0x\(String(ZIPConstants.localFileSignature, radix: 16))"
            )
            throw ZIPParserError.invalidZipFile
        }

        // Find end of central directory
        logger.info("Searching for end of central directory...")
        guard let endOfCentralDir = findEndOfCentralDirectory(in: zipData) else {
            logger.error("Could not find end of central directory")
            throw ZIPParserError.corruptedZipFile
        }
        logger.info("Found end of central directory with \(endOfCentralDir.totalEntries) entries")

        // Look for thumbnail files
        let thumbnailPaths = ["thumbnails/Thumbnail.png", "Thumbnail.png"]
        logger.info("Searching for thumbnail files: \(thumbnailPaths)")

        for thumbnailPath in thumbnailPaths {
            logger.info("Trying path: \(thumbnailPath)")
            if let thumbnailData = try? extractFile(
                from: zipData,
                endOfCentralDir: endOfCentralDir,
                filename: thumbnailPath
            ) {
                logger.debug("Found thumbnail at path: \(thumbnailPath)")
                logger.info(
                    "Found thumbnail at path: \(thumbnailPath), size: \(thumbnailData.count) bytes")

                if let image = createImage(from: thumbnailData, maxSize: maxSize) {
                    logger.debug("Successfully created CGImage from thumbnail data")
                    logger.info("Successfully created CGImage from thumbnail data")
                    return image
                } else {
                    logger.warning(
                        "Failed to create CGImage from thumbnail data at path: \(thumbnailPath)")
                }
            } else {
                logger.info("No thumbnail found at path: \(thumbnailPath)")
            }
        }

        logger.info("No thumbnail found in FCStd file")
        logger.warning("No valid thumbnail found in FCStd file")
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

// MARK: - Private Extensions

extension SwiftZIPParser {

    /// Find the end of central directory record
    private static func findEndOfCentralDirectory(in data: Data) -> ZIPEndOfCentralDir? {
        // Search backwards from the end of the file
        let minOffset = max(0, data.count - 65557)  // Max comment length + EOCD size
        for offset in stride(from: data.count - ZIPEndOfCentralDir.size, through: minOffset, by: -1)
        {
            let signature = readUInt32(from: data, at: offset)
            if signature == ZIPConstants.endOfCentralDirSignature {
                return parseEndOfCentralDir(from: data, at: offset)
            }
        }
        return nil
    }

    /// Parse end of central directory record
    private static func parseEndOfCentralDir(from data: Data, at offset: Int) -> ZIPEndOfCentralDir
    {
        return ZIPEndOfCentralDir(
            signature: readUInt32(from: data, at: offset),
            diskNumber: readUInt16(from: data, at: offset + 4),
            centralDirDisk: readUInt16(from: data, at: offset + 6),
            entriesOnDisk: readUInt16(from: data, at: offset + 8),
            totalEntries: readUInt16(from: data, at: offset + 10),
            centralDirSize: readUInt32(from: data, at: offset + 12),
            centralDirOffset: readUInt32(from: data, at: offset + 16),
            commentLength: readUInt16(from: data, at: offset + 20)
        )
    }

    /// Extract a specific file from the ZIP archive
    private static func extractFile(
        from zipData: Data,
        endOfCentralDir: ZIPEndOfCentralDir,
        filename: String
    ) throws -> Data {
        logger.debug("Searching for file: \(filename)")

        // Search through central directory
        var offset = Int(endOfCentralDir.centralDirOffset)
        for _ in 0..<endOfCentralDir.totalEntries {
            let centralHeader = parseCentralDirHeader(from: zipData, at: offset)

            // Extract filename
            let filenameStart = offset + ZIPCentralDirHeader.size
            let filenameData = zipData.subdata(
                in: filenameStart..<(filenameStart + Int(centralHeader.filenameLength)))
            if let foundFilename = String(data: filenameData, encoding: .utf8),
                foundFilename == filename
            {
                logger.debug("Found matching file: \(foundFilename)")
                return try extractFileData(from: zipData, centralHeader: centralHeader)
            }

            // Move to next entry
            offset +=
                ZIPCentralDirHeader.size + Int(centralHeader.filenameLength)
                + Int(centralHeader.extraFieldLength) + Int(centralHeader.commentLength)
        }

        throw ZIPParserError.thumbnailNotFound
    }

    /// Parse central directory file header
    private static func parseCentralDirHeader(from data: Data, at offset: Int)
        -> ZIPCentralDirHeader
    {
        return ZIPCentralDirHeader(
            signature: readUInt32(from: data, at: offset),
            versionMadeBy: readUInt16(from: data, at: offset + 4),
            versionNeeded: readUInt16(from: data, at: offset + 6),
            flags: readUInt16(from: data, at: offset + 8),
            compression: readUInt16(from: data, at: offset + 10),
            modTime: readUInt16(from: data, at: offset + 12),
            modDate: readUInt16(from: data, at: offset + 14),
            crc32: readUInt32(from: data, at: offset + 16),
            compressedSize: readUInt32(from: data, at: offset + 20),
            uncompressedSize: readUInt32(from: data, at: offset + 24),
            filenameLength: readUInt16(from: data, at: offset + 28),
            extraFieldLength: readUInt16(from: data, at: offset + 30),
            commentLength: readUInt16(from: data, at: offset + 32),
            diskNumber: readUInt16(from: data, at: offset + 34),
            internalAttributes: readUInt16(from: data, at: offset + 36),
            externalAttributes: readUInt32(from: data, at: offset + 38),
            localHeaderOffset: readUInt32(from: data, at: offset + 42)
        )
    }

    /// Extract file data using central directory header
    private static func extractFileData(from zipData: Data, centralHeader: ZIPCentralDirHeader)
        throws -> Data
    {
        let localHeaderOffset = Int(centralHeader.localHeaderOffset)
        let localHeader = parseLocalFileHeader(from: zipData, at: localHeaderOffset)

        guard localHeader.signature == ZIPConstants.localFileSignature else {
            throw ZIPParserError.corruptedZipFile
        }

        let dataStart =
            localHeaderOffset + ZIPLocalFileHeader.size + Int(localHeader.filenameLength)
            + Int(localHeader.extraFieldLength)
        let dataEnd = dataStart + Int(localHeader.compressedSize)

        guard dataEnd <= zipData.count else {
            throw ZIPParserError.corruptedZipFile
        }

        let compressedData = zipData.subdata(in: dataStart..<dataEnd)

        switch localHeader.compression {
        case ZIPConstants.compressionStored:
            return compressedData
        case ZIPConstants.compressionDeflate:
            return try deflateDecompress(
                data: compressedData, expectedSize: Int(localHeader.uncompressedSize))
        default:
            throw ZIPParserError.compressionUnsupported
        }
    }

    /// Parse local file header
    private static func parseLocalFileHeader(from data: Data, at offset: Int) -> ZIPLocalFileHeader
    {
        return ZIPLocalFileHeader(
            signature: readUInt32(from: data, at: offset),
            version: readUInt16(from: data, at: offset + 4),
            flags: readUInt16(from: data, at: offset + 6),
            compression: readUInt16(from: data, at: offset + 8),
            modTime: readUInt16(from: data, at: offset + 10),
            modDate: readUInt16(from: data, at: offset + 12),
            crc32: readUInt32(from: data, at: offset + 14),
            compressedSize: readUInt32(from: data, at: offset + 18),
            uncompressedSize: readUInt32(from: data, at: offset + 22),
            filenameLength: readUInt16(from: data, at: offset + 26),
            extraFieldLength: readUInt16(from: data, at: offset + 28)
        )
    }

    /// Decompress deflate-compressed data
    private static func deflateDecompress(data: Data, expectedSize: Int) throws -> Data {
        guard expectedSize > 0 else {
            throw ZIPParserError.decompressionFailed
        }

        return try data.withUnsafeBytes { bytes in
            let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: expectedSize)
            defer { buffer.deallocate() }

            let actualSize = compression_decode_buffer(
                buffer, expectedSize,
                bytes.bindMemory(to: UInt8.self).baseAddress!, data.count,
                nil, COMPRESSION_ZLIB
            )

            guard actualSize == expectedSize else {
                throw ZIPParserError.decompressionFailed
            }

            return Data(bytes: buffer, count: expectedSize)
        }
    }

    /// Create CGImage from PNG data with optional size constraints
    private static func createImage(from pngData: Data, maxSize: CGSize? = nil) -> CGImage? {
        guard let dataProvider = CGDataProvider(data: pngData as CFData),
            let image = CGImage(
                pngDataProviderSource: dataProvider, decode: nil, shouldInterpolate: true,
                intent: .defaultIntent)
        else {
            return nil
        }

        // If no max size specified, return original
        guard let maxSize = maxSize else {
            return image
        }

        // Calculate scaled size
        let originalSize = CGSize(width: CGFloat(image.width), height: CGFloat(image.height))
        let scaledSize = scaleToFit(originalSize: originalSize, maxSize: maxSize)

        // If no scaling needed, return original
        if scaledSize == originalSize {
            return image
        }

        // Scale the image
        return scaleImage(image, to: scaledSize)
    }

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

    /// Calculate size that fits within maxSize while maintaining aspect ratio
    fileprivate static func scaleToFit(originalSize: CGSize, maxSize: CGSize) -> CGSize {
        let widthRatio = maxSize.width / originalSize.width
        let heightRatio = maxSize.height / originalSize.height
        let scaleFactor = min(widthRatio, heightRatio)

        return CGSize(
            width: originalSize.width * scaleFactor,
            height: originalSize.height * scaleFactor
        )
    }

    /// Scale CGImage to specified size
    fileprivate static func scaleImage(_ image: CGImage, to size: CGSize) -> CGImage? {
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue)

        guard
            let context = CGContext(
                data: nil,
                width: Int(size.width),
                height: Int(size.height),
                bitsPerComponent: 8,
                bytesPerRow: 0,
                space: colorSpace,
                bitmapInfo: bitmapInfo.rawValue
            )
        else {
            logger.error("Failed to create CGContext for image scaling")
            return nil
        }

        context.interpolationQuality = .high
        context.draw(image, in: CGRect(origin: .zero, size: size))

        return context.makeImage()
    }
}
