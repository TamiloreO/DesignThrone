import SwiftUI

/// A read-only clip strip for Phase 0.
///
/// Dragging is deliberately absent until the trim gestures are built on top of
/// the engine's trim semantics, which are frame exact and already tested. Buttons
/// edit a selected clip in the meantime, so the pipeline is exercisable.
struct TimelineView: View {
    @ObservedObject var model: EditorModel

    private let pointsPerSecond: CGFloat = 24
    private let clipHeight: CGFloat = 56

    var body: some View {
        GeometryReader { proxy in
            ScrollView(.horizontal, showsIndicators: false) {
                ZStack(alignment: .topLeading) {
                    ForEach(model.clips) { clip in
                        clipBlock(clip)
                            .offset(x: CGFloat(clip.start) * pointsPerSecond)
                    }
                    Rectangle()
                        .fill(.red)
                        .frame(width: 2, height: clipHeight)
                        .offset(x: CGFloat(model.playhead) * pointsPerSecond)
                }
                .frame(
                    width: max(proxy.size.width, CGFloat(model.duration) * pointsPerSecond + 16),
                    height: clipHeight,
                    alignment: .topLeading
                )
                .padding(.vertical, 12)
            }
        }
        .frame(height: clipHeight + 24)
        .background(Color(white: 0.10))
    }

    private func clipBlock(_ clip: TimelineClip) -> some View {
        let width = max(10, CGFloat(clip.duration) * pointsPerSecond)
        let selected = model.selectedClip == clip.id

        return RoundedRectangle(cornerRadius: 6)
            .fill(selected ? Color.accentColor.opacity(0.85) : Color(white: 0.32))
            .frame(width: width, height: clipHeight)
            .overlay(alignment: .topLeading) {
                Text(String(format: "%.1fs", clip.duration))
                    .font(.caption2)
                    .foregroundStyle(.white.opacity(0.85))
                    .padding(5)
            }
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .strokeBorder(selected ? Color.white : Color.clear, lineWidth: 2)
            )
            .contentShape(Rectangle())
            .onTapGesture { model.selectedClip = clip.id }
    }
}
