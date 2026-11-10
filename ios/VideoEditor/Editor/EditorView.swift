import AVKit
import PhotosUI
import SwiftUI

struct EditorView: View {
    @StateObject private var model = EditorModel()
    @State private var pickerItems: [PhotosPickerItem] = []
    @State private var exportURL: URL?
    @Environment(\.scenePhase) private var scenePhase

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                preview
                TimelineView(model: model)
                controls
            }
            .background(Color.black.ignoresSafeArea())
            .navigationTitle(model.clips.isEmpty ? "New Project" : "Untitled")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar { toolbarContent }
            .alert("Something went wrong", isPresented: errorBinding) {
                Button("OK", role: .cancel) { model.errorMessage = nil }
            } message: {
                Text(model.errorMessage ?? "")
            }
            .sheet(item: exportBinding) { item in
                ExportSheet(url: item.url)
            }
            .onChange(of: pickerItems) { _, items in
                guard !items.isEmpty else { return }
                Task {
                    await model.importItems(items)
                    pickerItems = []
                }
            }
            .task {
                await model.restoreLastProject()
            }
            .onChange(of: scenePhase) { _, phase in
                // The app can be killed while in the background, so this is the
                // last certain moment to write the project out.
                if phase != .active {
                    model.saveNow()
                }
            }
        }
    }

    // MARK: - Pieces

    private var preview: some View {
        ZStack {
            Color.black
            if model.clips.isEmpty {
                ContentUnavailableView(
                    "No clips yet",
                    systemImage: "film.stack",
                    description: Text("Import a video to start editing.")
                )
            } else {
                VideoPlayer(player: model.player)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    private var controls: some View {
        VStack(spacing: 14) {
            HStack {
                Text(timecode(model.playhead))
                Slider(value: $model.playhead, in: 0...max(model.duration, 0.1))
                    .onChange(of: model.playhead) { _, value in
                        model.player.seek(
                            to: CMTime(seconds: value, preferredTimescale: 600),
                            toleranceBefore: .zero,
                            toleranceAfter: .zero
                        )
                    }
                Text(timecode(model.duration))
            }
            .font(.footnote.monospacedDigit())
            .foregroundStyle(.secondary)

            HStack(spacing: 10) {
                editButton("Undo", systemImage: "arrow.uturn.backward", enabled: model.canUndo) {
                    model.undo()
                }
                editButton("Redo", systemImage: "arrow.uturn.forward", enabled: model.canRedo) {
                    model.redo()
                }
                editButton("In -1s", systemImage: "arrow.right.to.line", enabled: model.selectedClip != nil) {
                    model.trimStart(by: 1)
                }
                editButton("In +1s", systemImage: "arrow.left.to.line", enabled: model.selectedClip != nil) {
                    model.trimStart(by: -1)
                }
                editButton("Out -1s", systemImage: "arrow.left.to.line", enabled: model.selectedClip != nil) {
                    model.trimEnd(by: -1)
                }
                editButton("Out +1s", systemImage: "arrow.right.to.line", enabled: model.selectedClip != nil) {
                    model.trimEnd(by: 1)
                }
                editButton("Split", systemImage: "scissors", enabled: model.selectedClip != nil) {
                    model.splitAtPlayhead()
                }
                editButton("Delete", systemImage: "trash", enabled: model.selectedClip != nil) {
                    model.removeSelected()
                }
            }
            .labelStyle(.iconOnly)
            .buttonStyle(.bordered)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(.ultraThinMaterial)
    }

    @ToolbarContentBuilder
    private var toolbarContent: some ToolbarContent {
        ToolbarItem(placement: .topBarLeading) {
            PhotosPicker(selection: $pickerItems, maxSelectionCount: 10, matching: .videos, photoLibrary: .shared()) {
                Label("Import", systemImage: "plus")
            }
            .disabled(model.isBusy)
        }
        ToolbarItem(placement: .topBarTrailing) {
            Button {
                Task {
                    if let url = await model.exportProject() {
                        exportURL = url
                    }
                }
            } label: {
                Label("Export", systemImage: "square.and.arrow.up")
            }
            .disabled(model.clips.isEmpty || model.isBusy)
        }
    }

    private func editButton(
        _ title: LocalizedStringKey,
        systemImage: String,
        enabled: Bool,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            Label(title, systemImage: systemImage)
        }
        .disabled(!enabled)
    }

    // MARK: - Helpers

    private var errorBinding: Binding<Bool> {
        Binding(
            get: { model.errorMessage != nil },
            set: { showing in if !showing { model.errorMessage = nil } }
        )
    }

    private var exportBinding: Binding<ExportItem?> {
        Binding(
            get: { exportURL.map(ExportItem.init(url:)) },
            set: { if $0 == nil { exportURL = nil } }
        )
    }

    private func timecode(_ seconds: Double) -> String {
        guard seconds.isFinite, seconds >= 0 else { return "00:00.0" }
        let tenths = Int((seconds * 10).rounded())
        return String(format: "%02d:%02d.%d", tenths / 600, (tenths / 10) % 60, tenths % 10)
    }
}

struct ExportItem: Identifiable {
    let url: URL
    var id: URL { url }
}

struct ExportSheet: View {
    let url: URL
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                Image(systemName: "checkmark.circle")
                    .font(.system(size: 48))
                    .foregroundStyle(.green)
                Text("Export finished")
                    .font(.headline)
                Text(url.lastPathComponent)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
                ShareLink(item: url) {
                    Label("Save or share", systemImage: "square.and.arrow.up")
                }
                .buttonStyle(.borderedProminent)
            }
            .padding()
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
    }
}
