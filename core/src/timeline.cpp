#include "vx/timeline.hpp"

#include <algorithm>

namespace vx {
namespace {

EditResult fail(EditError error) {
    EditResult result;
    result.error = error;
    return result;
}

EditResult succeed(Project project, ClipId inserted = 0) {
    EditResult result;
    result.project = std::move(project);
    result.insertedClip = inserted;
    return result;
}

void sortClips(Track& track) {
    std::stable_sort(track.clips.begin(), track.clips.end(),
                     [](const Clip& a, const Clip& b) { return a.start < b.start; });
}

ClipId splitInPlace(Track& track, std::size_t index, const Time& at, ClipId newId) {
    Clip& left = track.clips[index];
    const auto offset = at - left.start;
    const auto sourceOffset = scaleBySpeed(offset, left.speedNumerator, left.speedDenominator);

    Clip right = left;
    right.id = newId;
    right.start = at;
    right.sourceIn = left.sourceIn + sourceOffset;

    left.sourceOut = right.sourceIn;

    track.clips.push_back(right);
    return right.id;
}

}  // namespace

const char* toString(EditError error) {
    switch (error) {
        case EditError::None: return "none";
        case EditError::UnknownSequence: return "unknown sequence";
        case EditError::UnknownTrack: return "unknown track";
        case EditError::UnknownClip: return "unknown clip";
        case EditError::TrackKindMismatch: return "track kind mismatch";
        case EditError::InvalidRange: return "invalid range";
        case EditError::OverlapsExistingClip: return "overlaps existing clip";
        case EditError::WouldProduceEmptyClip: return "would produce empty clip";
        case EditError::SourceRangeExceeded: return "source range exceeded";
        case EditError::ValidationFailed: return "validation failed";
    }
    return "unknown";
}

Sequence* findSequence(Project& project, SequenceId id) {
    for (auto& sequence : project.sequences) {
        if (sequence.id == id) return &sequence;
    }
    return nullptr;
}

const Sequence* findSequence(const Project& project, SequenceId id) {
    for (const auto& sequence : project.sequences) {
        if (sequence.id == id) return &sequence;
    }
    return nullptr;
}

Track* findTrack(Sequence& sequence, TrackId id) {
    for (auto& track : sequence.tracks) {
        if (track.id == id) return &track;
    }
    return nullptr;
}

const Track* findTrack(const Sequence& sequence, TrackId id) {
    for (const auto& track : sequence.tracks) {
        if (track.id == id) return &track;
    }
    return nullptr;
}

std::optional<ClipLocation> locateClip(const Sequence& sequence, ClipId clip) {
    for (std::size_t t = 0; t < sequence.tracks.size(); ++t) {
        const auto& track = sequence.tracks[t];
        for (std::size_t c = 0; c < track.clips.size(); ++c) {
            if (track.clips[c].id == clip) return ClipLocation{t, c};
        }
    }
    return std::nullopt;
}

const Clip* clipAt(const Track& track, const Time& time) {
    for (const auto& clip : track.clips) {
        if (clip.contains(time)) return &clip;
    }
    return nullptr;
}

EditResult appendClip(const Project& project, SequenceId sequenceId, TrackId trackId, Clip clip) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    if (clip.sourceOut <= clip.sourceIn) return fail(EditError::InvalidRange);

    Time end = Time::zero();
    for (const auto& existing : track->clips) {
        if (existing.end() > end) end = existing.end();
    }

    const auto insertedId = next.allocateId();
    clip.id = insertedId;
    clip.start = end;
    track->clips.push_back(clip);
    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next), insertedId);
}

EditResult insertClip(const Project& project, SequenceId sequenceId, TrackId trackId, Clip clip) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    if (clip.sourceOut <= clip.sourceIn || clip.start.isNegative()) {
        return fail(EditError::InvalidRange);
    }

    const auto duration = clip.duration();
    if (duration.isZero()) return fail(EditError::WouldProduceEmptyClip);

    const auto insertAt = clip.start;

    for (std::size_t i = 0; i < track->clips.size(); ++i) {
        const auto& existing = track->clips[i];
        if (existing.start < insertAt && insertAt < existing.end()) {
            splitInPlace(*track, i, insertAt, next.allocateId());
            break;
        }
    }

    const auto insertedId = next.allocateId();
    clip.id = insertedId;

    for (auto& existing : track->clips) {
        if (existing.start >= insertAt) existing.start = existing.start + duration;
    }

    track->clips.push_back(clip);
    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next), insertedId);
}

EditResult overwriteClip(const Project& project, SequenceId sequenceId, TrackId trackId, Clip clip) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    if (clip.sourceOut <= clip.sourceIn || clip.start.isNegative()) {
        return fail(EditError::InvalidRange);
    }

    const auto newId = next.allocateId();
    const auto rangeStart = clip.start;
    const auto rangeEnd = clip.start + clip.duration();
    if (rangeEnd <= rangeStart) return fail(EditError::WouldProduceEmptyClip);

    std::vector<Clip> result;
    result.reserve(track->clips.size() + 2);

    for (const auto& existing : track->clips) {
        if (existing.end() <= rangeStart || existing.start >= rangeEnd) {
            result.push_back(existing);
            continue;
        }

        if (existing.start < rangeStart) {
            Clip left = existing;
            const auto offset = rangeStart - existing.start;
            left.sourceOut = existing.sourceIn +
                             scaleBySpeed(offset, existing.speedNumerator, existing.speedDenominator);
            if (left.sourceOut > left.sourceIn) result.push_back(left);
        }

        if (existing.end() > rangeEnd) {
            Clip right = existing;
            const auto offset = rangeEnd - existing.start;
            right.id = next.allocateId();
            right.start = rangeEnd;
            right.sourceIn = existing.sourceIn +
                             scaleBySpeed(offset, existing.speedNumerator, existing.speedDenominator);
            if (right.sourceOut > right.sourceIn) result.push_back(right);
        }
    }

    clip.id = newId;
    result.push_back(clip);
    track->clips = std::move(result);
    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next), newId);
}

EditResult liftClip(const Project& project, SequenceId sequenceId, TrackId trackId, ClipId clipId) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    const auto before = track->clips.size();
    track->clips.erase(std::remove_if(track->clips.begin(), track->clips.end(),
                                      [clipId](const Clip& c) { return c.id == clipId; }),
                       track->clips.end());
    if (track->clips.size() == before) return fail(EditError::UnknownClip);

    return succeed(std::move(next));
}

EditResult rippleDeleteClip(const Project& project, SequenceId sequenceId, TrackId trackId,
                            ClipId clipId) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    const auto found = locateClip(*sequence, clipId);
    if (!found) return fail(EditError::UnknownClip);

    const auto start = track->clips[found->clipIndex].start;
    const auto duration = track->clips[found->clipIndex].duration();
    track->clips.erase(track->clips.begin() + static_cast<std::ptrdiff_t>(found->clipIndex));

    for (auto& existing : track->clips) {
        if (existing.start >= start) existing.start = existing.start - duration;
    }

    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next));
}

EditResult splitClip(const Project& project, SequenceId sequenceId, TrackId trackId, ClipId clipId,
                     Time at) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    const auto found = locateClip(*sequence, clipId);
    if (!found) return fail(EditError::UnknownClip);

    const auto& target = track->clips[found->clipIndex];
    if (at <= target.start || at >= target.end()) return fail(EditError::InvalidRange);

    const auto newId = next.allocateId();
    splitInPlace(*track, found->clipIndex, at, newId);
    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next), newId);
}

EditResult trimStart(const Project& project, SequenceId sequenceId, TrackId trackId, ClipId clipId,
                     Time delta, bool ripple) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    const auto found = locateClip(*sequence, clipId);
    if (!found) return fail(EditError::UnknownClip);

    Clip& target = track->clips[found->clipIndex];

    auto sourceDelta = scaleBySpeed(delta, target.speedNumerator, target.speedDenominator);

    if (target.sourceIn + sourceDelta < Time::zero()) {
        sourceDelta = Time::zero() - target.sourceIn;
    }
    if (target.sourceOut - sourceDelta <= target.sourceIn) {
        return fail(EditError::WouldProduceEmptyClip);
    }
    // The source range may have been clamped at the head of the media, so the
    // timeline move has to follow the clamped amount rather than the request.
    const auto effectiveDelta =
        unscaleBySpeed(sourceDelta, target.speedNumerator, target.speedDenominator);
    if (target.start + effectiveDelta < Time::zero()) return fail(EditError::InvalidRange);

    const auto oldStart = target.start;
    const auto oldEnd = target.end();
    target.sourceIn = target.sourceIn + sourceDelta;

    if (ripple) {
        // A ripple trim keeps the clip anchored and closes or opens the gap by
        // moving everything that followed it. Only the clips after this one
        // move; moving the clip being trimmed would shift it twice.
        const auto newEnd = target.start + target.duration();
        const auto shift = newEnd - oldEnd;
        for (auto& other : track->clips) {
            if (other.id == clipId) continue;
            if (other.start >= oldEnd) other.start = other.start + shift;
        }
    } else {
        target.start = oldStart + effectiveDelta;
    }

    sortClips(*track);
    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next));
}

EditResult trimEnd(const Project& project, SequenceId sequenceId, TrackId trackId, ClipId clipId,
                   Time delta, bool ripple) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    const auto found = locateClip(*sequence, clipId);
    if (!found) return fail(EditError::UnknownClip);

    Clip& target = track->clips[found->clipIndex];

    auto sourceDelta = scaleBySpeed(delta, target.speedNumerator, target.speedDenominator);

    if (target.sourceOut + sourceDelta <= target.sourceIn) {
        return fail(EditError::WouldProduceEmptyClip);
    }

    const auto oldEnd = target.end();
    target.sourceOut = target.sourceOut + sourceDelta;

    if (ripple) {
        const auto shift = target.end() - oldEnd;
        for (auto& other : track->clips) {
            if (other.id == clipId) continue;
            if (other.start >= oldEnd) other.start = other.start + shift;
        }
    }

    sortClips(*track);
    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next));
}

EditResult moveClip(const Project& project, SequenceId sequenceId, TrackId trackId, ClipId clipId,
                    Time newStart) {
    Project next = project;
    auto* sequence = findSequence(next, sequenceId);
    if (!sequence) return fail(EditError::UnknownSequence);
    auto* track = findTrack(*sequence, trackId);
    if (!track) return fail(EditError::UnknownTrack);

    if (newStart.isNegative()) return fail(EditError::InvalidRange);

    const auto found = locateClip(*sequence, clipId);
    if (!found) return fail(EditError::UnknownClip);

    const auto duration = track->clips[found->clipIndex].duration();
    const auto rangeEnd = newStart + duration;

    for (const auto& other : track->clips) {
        if (other.id == clipId) continue;
        if (newStart < other.end() && rangeEnd > other.start) {
            return fail(EditError::OverlapsExistingClip);
        }
    }

    track->clips[found->clipIndex].start = newStart;
    sortClips(*track);

    if (!validate(next).ok()) return fail(EditError::ValidationFailed);
    return succeed(std::move(next));
}

}  // namespace vx
