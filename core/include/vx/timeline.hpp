#pragma once

#include <optional>

#include "vx/model.hpp"

namespace vx {

// Every edit operation is a pure function: it takes a project and returns a new
// one, or an error. Nothing mutates in place.
//
// This is what makes undo trustworthy and what makes the operations testable
// without a UI, a GPU, or a device. Document::commit() is the only thing that
// makes an edit visible, so an operation that fails leaves no trace.

enum class EditError {
    None,
    UnknownSequence,
    UnknownTrack,
    UnknownClip,
    TrackKindMismatch,
    InvalidRange,
    OverlapsExistingClip,
    WouldProduceEmptyClip,
    SourceRangeExceeded,
    ValidationFailed,
};

const char* toString(EditError error);

struct EditResult {
    EditError error = EditError::None;
    Project project;
    ClipId insertedClip = 0;

    bool ok() const { return error == EditError::None; }
};

Sequence* findSequence(Project& project, SequenceId id);
const Sequence* findSequence(const Project& project, SequenceId id);

Track* findTrack(Sequence& sequence, TrackId id);
const Track* findTrack(const Sequence& sequence, TrackId id);

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

// Place a clip after the last clip on the track. The position in `clip` is ignored.
EditResult appendClip(const Project& project, SequenceId sequence, TrackId track, Clip clip);

// Insert at clip.start and ripple everything at or after that point later by
// the clip's duration.
EditResult insertClip(const Project& project, SequenceId sequence, TrackId track, Clip clip);

// Place at clip.start, replacing whatever is there. Clips that straddle the
// boundary are split, and the middle is discarded.
EditResult overwriteClip(const Project& project, SequenceId sequence, TrackId track, Clip clip);

// ---------------------------------------------------------------------------
// Removal and cutting
// ---------------------------------------------------------------------------

// Remove the clip and leave a gap.
EditResult liftClip(const Project& project, SequenceId sequence, TrackId track, ClipId clip);

// Remove the clip and close the gap by pulling later clips back.
EditResult rippleDeleteClip(const Project& project, SequenceId sequence, TrackId track, ClipId clip);

// Cut the clip in two at an absolute sequence time. Both halves keep the
// clip's speed and source mapping.
EditResult splitClip(const Project& project, SequenceId sequence, TrackId track, ClipId clip,
                     Time at);

// ---------------------------------------------------------------------------
// Trimming
// ---------------------------------------------------------------------------

// Move the clip's in point by delta. Positive shortens, negative extends.
// When ripple is true, later clips close or open the gap.
EditResult trimStart(const Project& project, SequenceId sequence, TrackId track, ClipId clip,
                     Time delta, bool ripple);

// Move the clip's out point by delta. Positive extends, negative shortens.
EditResult trimEnd(const Project& project, SequenceId sequence, TrackId track, ClipId clip,
                   Time delta, bool ripple);

// ---------------------------------------------------------------------------
// Moving
// ---------------------------------------------------------------------------

// Move a clip to a new start time on the same track. Fails rather than
// silently overwriting if the destination overlaps another clip.
EditResult moveClip(const Project& project, SequenceId sequence, TrackId track, ClipId clip,
                    Time newStart);

// ---------------------------------------------------------------------------
// Queries used by the renderer and the UI
// ---------------------------------------------------------------------------

struct ClipLocation {
    std::size_t trackIndex = 0;
    std::size_t clipIndex = 0;
};

std::optional<ClipLocation> locateClip(const Sequence& sequence, ClipId clip);

// The clip visible at a given time on a track, or nullptr.
const Clip* clipAt(const Track& track, const Time& time);

}  // namespace vx
