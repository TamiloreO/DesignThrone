#include "vx/model.hpp"

#include <algorithm>
#include <unordered_set>

namespace vx {

const char* toString(ValidationError error) {
    switch (error) {
        case ValidationError::None: return "none";
        case ValidationError::OverlappingClips: return "overlapping clips";
        case ValidationError::UnsortedClips: return "unsorted clips";
        case ValidationError::NegativeDuration: return "negative duration";
        case ValidationError::EmptySourceRange: return "empty source range";
        case ValidationError::InvalidSpeed: return "invalid speed";
        case ValidationError::DuplicateClipId: return "duplicate clip id";
        case ValidationError::InvalidSequenceSettings: return "invalid sequence settings";
        case ValidationError::UnknownTrack: return "unknown track";
        case ValidationError::UnknownClip: return "unknown clip";
    }
    return "unknown";
}

ValidationResult validate(const Sequence& sequence) {
    ValidationResult result;

    if (sequence.width <= 0 || sequence.height <= 0 || !sequence.frameRate.isValid()) {
        result.error = ValidationError::InvalidSequenceSettings;
        return result;
    }

    std::unordered_set<ClipId> seenIds;

    for (std::size_t t = 0; t < sequence.tracks.size(); ++t) {
        const auto& track = sequence.tracks[t];
        Time previousEnd = Time::zero();
        bool first = true;

        for (std::size_t c = 0; c < track.clips.size(); ++c) {
            const auto& clip = track.clips[c];

            if (clip.speedNumerator <= 0 || clip.speedDenominator <= 0) {
                result.error = ValidationError::InvalidSpeed;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
            if (clip.sourceOut <= clip.sourceIn) {
                result.error = ValidationError::EmptySourceRange;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
            if (clip.duration().isNegative() || clip.duration().isZero()) {
                result.error = ValidationError::NegativeDuration;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
            if (clip.start.isNegative()) {
                result.error = ValidationError::NegativeDuration;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
            if (!seenIds.insert(clip.id).second) {
                result.error = ValidationError::DuplicateClipId;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
            if (!first && clip.start < previousEnd) {
                // Distinguishes the two failure modes so callers get a useful message.
                result.error = (clip.start < track.clips[c - 1].start)
                                   ? ValidationError::UnsortedClips
                                   : ValidationError::OverlappingClips;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }

            previousEnd = clip.end();
            first = false;
        }
    }

    return result;
}

ValidationResult validate(const Project& project) {
    for (const auto& sequence : project.sequences) {
        const auto result = validate(sequence);
        if (!result.ok()) return result;
    }
    return ValidationResult{};
}

Document::Document(Project project) : project_(std::move(project)) {}

void Document::commit(Project next) {
    undo_.push_back(project_);
    if (undo_.size() > kMaxHistory) {
        undo_.erase(undo_.begin());
    }
    redo_.clear();
    project_ = std::move(next);
}

bool Document::undo() {
    if (undo_.empty()) return false;
    redo_.push_back(project_);
    project_ = std::move(undo_.back());
    undo_.pop_back();
    return true;
}

bool Document::redo() {
    if (redo_.empty()) return false;
    undo_.push_back(project_);
    project_ = std::move(redo_.back());
    redo_.pop_back();
    return true;
}

void Document::clearHistory() {
    undo_.clear();
    redo_.clear();
}

}  // namespace vx
