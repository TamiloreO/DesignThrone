import AVFoundation
import PhotosUI
import SwiftUI

/// Owns the engine, the media, and everything the editor view renders.
///
/// All state changes go engine first, then the published arrays are refreshed
/// from the engine. That keeps Swift from drifting out of sync with the timeline
/// rules (no overlaps, no inverted clips, ripple behaviour).
@MainActor
final class EditorModel: ObservableObject {
    @Published private(set) var clips: [TimelineClip] = []
    @Published private(set) var assets: [UInt64: MediaAsset] = [:]
    @Published private(set) var canUndo = false
    @Published private(set) var canRedo = false
    @Published private(set) var isBusy = false
    @Published var selectedClip: UInt64?
    @Published var playhead: Double = 0
    @Published var errorMessage: String?

    let player = AVPlayer()

    private let engine = Engine()
    private var nextMediaID: UInt64 = 1
    private var rebuildTask: Task<Void, Never>?

    var duration: Double { engine.duration }

    var selectedClipValue: TimelineClip? {
        guard let selectedClip else { return nil }
        return clips.first { $0.id == selectedClip }
    }

    // MARK: - Import

    func importItems(_ items: [PhotosPickerItem]) async {
        guard !items.isEmpty else { return }
        isBusy = true
        defer { isBusy = false }

        for item in items {
            do {
                guard let movie = try await item.loadTransferable(type: PickedMovie.self) else {
                    continue
                }
                let mediaID = nextMediaID
                nextMediaID += 1
                let asset = try await MediaStore.importMovie(at: movie.url, id: mediaID)
                assets[asset.id] = asset
                try engine.append(media: asset.id, sourceIn: 0, sourceOut: asset.duration)
            } catch {
                errorMessage = error.localizedDescription
            }
        }
        await refresh()
    }

    // MARK: - Edits

    func trimStart(by delta: Double) {
        guard let selectedClip else { return }
        apply { try engine.trimStart(clip: selectedClip, by: delta) }
    }

    func trimEnd(by delta: Double) {
        guard let selectedClip else { return }
        apply { try engine.trimEnd(clip: selectedClip, by: delta) }
    }

    func splitAtPlayhead() {
        guard let clip = clips.first(where: { playhead > $0.start && playhead < $0.end }) else {
            return
        }
        apply { _ = try engine.split(clip: clip.id, at: playhead) }
    }

    func removeSelected() {
        guard let selectedClip else { return }
        apply { try engine.remove(clip: selectedClip) }
    }

    func undo() { apply { try engine.undo() } }
    func redo() { apply { try engine.redo() } }

    func ensureSelection() {
        if selectedClip == nil {
            selectedClip = clips.first?.id
        }
    }

    // MARK: - Export

    func exportProject() async -> URL? {
        isBusy = true
        defer { isBusy = false }
        do {
            guard let result = try await CompositionBuilder.build(clips: clips, assets: assets) else {
                return nil
            }
            return try await ExportService.export(result, named: "Export")
        } catch {
            errorMessage = error.localizedDescription
            return nil
        }
    }

    // MARK: - Internals

    private func apply(_ edit: () throws -> Void) {
        do {
            try edit()
            Task { await refresh() }
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    private func refresh() async {
        clips = engine.clips
        canUndo = engine.canUndo
        canRedo = engine.canRedo
        if let selected = selectedClip, !clips.contains(where: { $0.id == selected }) {
            selectedClip = clips.first?.id
        } else {
            ensureSelection()
        }
        playhead = min(playhead, max(0, engine.duration))
        schedulePreviewRebuild()
    }

    private func schedulePreviewRebuild() {
        rebuildTask?.cancel()

        let currentClips = clips
        let currentAssets = assets
        rebuildTask = Task { [weak self] in
            guard let self else { return }
            do {
                let result = try await CompositionBuilder.build(clips: currentClips, assets: currentAssets)
                guard !Task.isCancelled else { return }
                if let result {
                    let item = AVPlayerItem(asset: result.composition)
                    item.videoComposition = result.videoComposition
                    self.player.replaceCurrentItem(with: item)
                } else {
                    self.player.replaceCurrentItem(with: nil)
                }
            } catch {
                self.errorMessage = error.localizedDescription
            }
        }
    }
}
