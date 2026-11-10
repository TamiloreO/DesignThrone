import AVFoundation
import CoreGraphics
import CoreTransferable
import Foundation
import PhotosUI
import UniformTypeIdentifiers

/// Footage that has been copied into the app's own storage.
struct MediaAsset: Identifiable {
    let id: UInt64
    let url: URL
    let duration: Double
    let naturalSize: CGSize
    let preferredTransform: CGAffineTransform

    /// Size after rotation, which is the size the viewer actually sees.
    var displaySize: CGSize {
        let rect = CGRect(origin: .zero, size: naturalSize).applying(preferredTransform)
        return CGSize(width: abs(rect.width), height: abs(rect.height))
    }
}

/// The value PhotosPicker hands back: a file that exists only for the duration
/// of the import.
struct PickedMovie: Transferable {
    let url: URL

    static var transferRepresentation: some TransferRepresentation {
        FileRepresentation(contentType: .movie) { movie in
            SentTransferredFile(movie.url)
        } importing: { received in
            let ext = received.file.pathExtension.isEmpty ? "mov" : received.file.pathExtension
            let destination = FileManager.default.temporaryDirectory
                .appendingPathComponent(UUID().uuidString)
                .appendingPathExtension(ext)
            if FileManager.default.fileExists(atPath: destination.path) {
                try FileManager.default.removeItem(at: destination)
            }
            try FileManager.default.copyItem(at: received.file, to: destination)
            return PickedMovie(url: destination)
        }
    }
}

/// Imports footage into the app's storage.
///
/// The picker hands over a temporary file, and the original in the photo library
/// may be an iCloud placeholder or may be deleted later. A project has to keep
/// working either way, so imports are copied rather than referenced.
enum MediaStore {
    static var storageDirectory: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent("Media", isDirectory: true)
    }

    static func importMovie(at url: URL, id: UInt64) async throws -> MediaAsset {
        try FileManager.default.createDirectory(at: storageDirectory, withIntermediateDirectories: true)

        let ext = url.pathExtension.isEmpty ? "mov" : url.pathExtension
        let destination = storageDirectory
            .appendingPathComponent(UUID().uuidString)
            .appendingPathExtension(ext)
        try FileManager.default.copyItem(at: url, to: destination)
        try? FileManager.default.removeItem(at: url)

        let asset = AVURLAsset(url: destination)
        let duration = try await asset.load(.duration)
        let seconds = duration.seconds

        var naturalSize = CGSize(width: 1920, height: 1080)
        var transform = CGAffineTransform.identity
        if let track = try await asset.loadTracks(withMediaType: .video).first {
            naturalSize = try await track.load(.naturalSize)
            transform = try await track.load(.preferredTransform)
        }

        return MediaAsset(
            id: id,
            url: destination,
            duration: seconds.isFinite && seconds > 0 ? seconds : 0,
            naturalSize: naturalSize,
            preferredTransform: transform
        )
    }
}
