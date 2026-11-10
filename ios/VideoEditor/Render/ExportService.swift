import AVFoundation
import Foundation

enum ExportError: Error, LocalizedError {
    case noSession
    case failed(String)

    var errorDescription: String? {
        switch self {
        case .noSession:
            return "This project cannot be exported on this device."
        case .failed(let message):
            return message
        }
    }
}

/// Exports a composition to a file.
///
/// Known gap: the codec is whatever AVFoundation chooses for the preset, which
/// can be HEVC on newer hardware. Pinning H.264 needs the custom exporter that
/// ships with the render pipeline. The container is MP4 either way.
@MainActor
enum ExportService {
    static func export(_ result: CompositionResult, named name: String) async throws -> URL {
        let destination = FileManager.default.temporaryDirectory
            .appendingPathComponent(name)
            .appendingPathExtension("mp4")
        if FileManager.default.fileExists(atPath: destination.path) {
            try FileManager.default.removeItem(at: destination)
        }

        guard let session = AVAssetExportSession(
            asset: result.composition,
            presetName: AVAssetExportPresetHighestQuality
        ) else {
            throw ExportError.noSession
        }

        session.videoComposition = result.videoComposition
        session.shouldOptimizeForNetworkUse = true

        do {
            try await session.export(to: destination, as: .mp4)
        } catch {
            throw ExportError.failed(error.localizedDescription)
        }
        return destination
    }
}
