import AVFoundation
import CoreGraphics

struct CompositionResult {
    let composition: AVMutableComposition
    let videoComposition: AVMutableVideoComposition
}

enum CompositionError: Error, LocalizedError {
    case noVideoTrack

    var errorDescription: String? {
        switch self {
        case .noVideoTrack:
            return "The device could not create a video track."
        }
    }
}

/// Builds an AVFoundation composition from the engine's clip list.
///
/// This is deliberately temporary plumbing. It renders through AVFoundation's
/// compositor, which is enough to preview and export a straightforward timeline,
/// but it cannot do effects, masks, or speed ramps. Those arrive with the Metal
/// render pipeline in Phase 1, which will replace this file rather than extend it.
@MainActor
enum CompositionBuilder {
    static let renderSize = CGSize(width: 1920, height: 1080)
    static let timescale: CMTimeScale = 600
    static let frameRate: Int32 = 30

    static func build(clips: [TimelineClip], assets: [UInt64: MediaAsset]) async throws -> CompositionResult? {
        guard !clips.isEmpty else { return nil }

        let composition = AVMutableComposition()
        guard let videoTrack = composition.addMutableTrack(
            withMediaType: .video,
            preferredTrackID: kCMPersistentTrackID_Invalid
        ) else {
            throw CompositionError.noVideoTrack
        }
        let audioTrack = composition.addMutableTrack(
            withMediaType: .audio,
            preferredTrackID: kCMPersistentTrackID_Invalid
        )

        for clip in clips {
            guard let media = assets[clip.media], clip.duration > 0 else { continue }
            let source = AVURLAsset(url: media.url)
            let range = CMTimeRange(
                start: CMTime(seconds: clip.sourceIn, preferredTimescale: timescale),
                duration: CMTime(seconds: clip.duration, preferredTimescale: timescale)
            )
            let insertion = CMTime(seconds: clip.start, preferredTimescale: timescale)

            if let sourceVideo = try await source.loadTracks(withMediaType: .video).first {
                try videoTrack.insertTimeRange(range, of: sourceVideo, at: insertion)
            }
            if let audio = audioTrack,
               let sourceAudio = try await source.loadTracks(withMediaType: .audio).first {
                try audio.insertTimeRange(range, of: sourceAudio, at: insertion)
            }
        }

        let videoComposition = AVMutableVideoComposition()
        videoComposition.renderSize = renderSize
        videoComposition.frameDuration = CMTime(value: 1, timescale: frameRate)

        var instructions: [AVMutableVideoCompositionInstruction] = []
        for clip in clips {
            guard let media = assets[clip.media], clip.duration > 0 else { continue }

            let instruction = AVMutableVideoCompositionInstruction()
            instruction.timeRange = CMTimeRange(
                start: CMTime(seconds: clip.start, preferredTimescale: timescale),
                duration: CMTime(seconds: clip.duration, preferredTimescale: timescale)
            )
            instruction.backgroundColor = CGColor(gray: 0, alpha: 1)

            let layer = AVMutableVideoCompositionLayerInstruction(assetTrack: videoTrack)
            layer.setTransform(fitTransform(for: media), at: .zero)
            instruction.layerInstructions = [layer]
            instructions.append(instruction)
        }
        videoComposition.instructions = instructions

        return CompositionResult(composition: composition, videoComposition: videoComposition)
    }

    /// Rotates the footage upright and scales it to fit the render size, keeping
    /// its aspect ratio and centring it on black.
    static func fitTransform(for media: MediaAsset) -> CGAffineTransform {
        let oriented = CGRect(origin: .zero, size: media.naturalSize).applying(media.preferredTransform)
        let width = abs(oriented.width)
        let height = abs(oriented.height)
        guard width > 0, height > 0 else { return media.preferredTransform }

        let scale = min(renderSize.width / width, renderSize.height / height)
        let dx = (renderSize.width - width * scale) / 2 - oriented.minX * scale
        let dy = (renderSize.height - height * scale) / 2 - oriented.minY * scale

        return media.preferredTransform
            .concatenating(CGAffineTransform(scaleX: scale, y: scale))
            .concatenating(CGAffineTransform(translationX: dx, y: dy))
    }
}
