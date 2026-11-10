import Foundation

/// One clip as the engine sees it. Times are seconds on the sequence.
struct TimelineClip: Identifiable, Equatable {
    let id: UInt64
    let media: UInt64
    var start: Double
    var duration: Double
    var sourceIn: Double
    var sourceOut: Double
    var speed: Double

    var end: Double { start + duration }
}

enum EngineError: Error, LocalizedError {
    case status(Int32)

    var errorDescription: String? {
        switch self {
        case .status(let code):
            return String(cString: vxe_status_string(code))
        }
    }
}

/// Thin wrapper over the C ABI. Every edit goes through the engine, which owns
/// the timeline invariants and the undo stack. The Swift side never reorders
/// clips or does time math of its own.
final class Engine {
    private let handle: OpaquePointer

    init() {
        guard let handle = vxe_project_create() else {
            fatalError("The editing engine could not be created")
        }
        self.handle = handle
    }

    deinit {
        vxe_project_destroy(handle)
    }

    var clips: [TimelineClip] {
        let count = Int(vxe_clip_count(handle))
        guard count > 0 else { return [] }

        var buffer = [vxe_clip](repeating: vxe_clip(), count: count)
        let written = Int(vxe_copy_clips(handle, &buffer, Int32(count)))
        guard written > 0 else { return [] }

        return buffer.prefix(written).map { clip in
            TimelineClip(
                id: clip.id,
                media: clip.media,
                start: clip.start,
                duration: clip.duration,
                sourceIn: clip.source_in,
                sourceOut: clip.source_out,
                speed: clip.speed
            )
        }
    }

    var duration: Double { vxe_duration(handle) }
    var canUndo: Bool { vxe_can_undo(handle) != 0 }
    var canRedo: Bool { vxe_can_redo(handle) != 0 }

    @discardableResult
    func append(media: UInt64, sourceIn: Double, sourceOut: Double) throws -> UInt64 {
        var clip: UInt64 = 0
        try check(vxe_append(handle, media, sourceIn, sourceOut, &clip))
        return clip
    }

    @discardableResult
    func insert(media: UInt64, at start: Double, sourceIn: Double, sourceOut: Double) throws -> UInt64 {
        var clip: UInt64 = 0
        try check(vxe_insert(handle, media, start, sourceIn, sourceOut, &clip))
        return clip
    }

    func trimStart(clip: UInt64, by delta: Double, ripple: Bool = true) throws {
        try check(vxe_trim_start(handle, clip, delta, ripple ? 1 : 0))
    }

    func trimEnd(clip: UInt64, by delta: Double, ripple: Bool = true) throws {
        try check(vxe_trim_end(handle, clip, delta, ripple ? 1 : 0))
    }

    @discardableResult
    func split(clip: UInt64, at time: Double) throws -> UInt64 {
        var right: UInt64 = 0
        try check(vxe_split(handle, clip, time, &right))
        return right
    }

    func move(clip: UInt64, to start: Double) throws {
        try check(vxe_move(handle, clip, start))
    }

    func remove(clip: UInt64, ripple: Bool = true) throws {
        try check(vxe_remove(handle, clip, ripple ? 1 : 0))
    }

    func undo() throws { try check(vxe_undo(handle)) }
    func redo() throws { try check(vxe_redo(handle)) }
    func reset() { vxe_project_reset(handle) }

    private func check(_ status: Int32) throws {
        if status != VXE_OK {
            throw EngineError.status(status)
        }
    }
}
