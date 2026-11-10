#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "vx/arith.hpp"
#include "vx/time.hpp"

namespace vx {

using ClipId = std::uint64_t;
using TrackId = std::uint64_t;
using SequenceId = std::uint64_t;
using MediaId = std::uint64_t;

// Speed mapping multiplies a nanosecond value by the speed numerator before
// dividing. A 24 hour timeline is 8.6e13 ns and a speed numerator can be in the
// millions, so the intermediate product overflows a signed 64 bit integer and
// wraps silently. Everything that scales time by speed goes through these two
// helpers, which use the portable wide arithmetic in vx/arith.hpp.
inline Time scaleBySpeed(const Time& time, std::int32_t numerator, std::int32_t denominator) {
    if (numerator <= 0 || denominator <= 0) return time;
    return Time(mulDiv(time.ns, numerator, denominator));
}

inline Time unscaleBySpeed(const Time& time, std::int32_t numerator, std::int32_t denominator) {
    if (numerator <= 0 || denominator <= 0) return time;
    return Time(mulDiv(time.ns, denominator, numerator));
}

enum class TrackKind { Video, Audio };

// A non-destructive reference to a clip on a track.
//
// sourceIn and sourceOut address the source media. start addresses the
// sequence. The two are independent, which is what makes trimming and moving
// cheap: neither operation touches the media.
//
// speed is expressed as a fraction so that a 0.8x clip is exactly 4/5 and a
// two hour timeline does not drift. duration on the sequence is
// (sourceOut - sourceIn) / speed.
struct Clip {
    ClipId id = 0;
    MediaId media = 0;
    Time start;
    Time sourceIn;
    Time sourceOut;
    std::int32_t speedNumerator = 1;
    std::int32_t speedDenominator = 1;
    bool enabled = true;
    std::string name;

    Time sourceDuration() const { return sourceOut - sourceIn; }

    Time duration() const { return unscaleBySpeed(sourceDuration(), speedNumerator, speedDenominator); }

    Time end() const { return start + duration(); }

    bool contains(const Time& t) const { return t >= start && t < end(); }

    // Map a sequence time to a source time, honouring speed. Clamped to the
    // clip's source range so a rounding error can never read outside the clip.
    Time sourceTimeAt(const Time& timelineTime) const {
        const auto offset = timelineTime - start;
        const auto sourceOffset = scaleBySpeed(offset, speedNumerator, speedDenominator);
        auto mapped = sourceIn + sourceOffset;
        if (mapped < sourceIn) mapped = sourceIn;
        if (mapped > sourceOut) mapped = sourceOut;
        return mapped;
    }

    FrameRate speed() const { return FrameRate(speedNumerator, speedDenominator); }
};

struct Track {
    TrackId id = 0;
    TrackKind kind = TrackKind::Video;
    std::string name;
    bool muted = false;
    bool hidden = false;
    bool locked = false;
    std::vector<Clip> clips;  // Invariant: sorted by start, never overlapping.
};

struct Sequence {
    SequenceId id = 0;
    std::string name;
    std::int32_t width = 1920;
    std::int32_t height = 1080;
    FrameRate frameRate = FrameRate::fps(30);
    std::vector<Track> tracks;

    Time duration() const {
        Time end = Time::zero();
        for (const auto& track : tracks) {
            for (const auto& clip : track.clips) {
                if (clip.end() > end) end = clip.end();
            }
        }
        return end;
    }
};

struct MediaItem {
    MediaId id = 0;
    std::string path;
    std::string displayName;
    Time duration;
    std::int32_t width = 0;
    std::int32_t height = 0;
    FrameRate frameRate = FrameRate::fps(30);
    std::string contentHash;
};

struct Project {
    std::string name;
    std::vector<MediaItem> media;
    std::vector<Sequence> sequences;
    std::uint64_t nextId = 1;

    std::uint64_t allocateId() { return nextId++; }
};

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

enum class ValidationError {
    None,
    OverlappingClips,
    UnsortedClips,
    NegativeDuration,
    EmptySourceRange,
    InvalidSpeed,
    DuplicateClipId,
    InvalidSequenceSettings,
    UnknownTrack,
    UnknownClip,
};

const char* toString(ValidationError error);

struct ValidationResult {
    ValidationError error = ValidationError::None;
    std::size_t trackIndex = 0;
    std::size_t clipIndex = 0;
    bool ok() const { return error == ValidationError::None; }
};

ValidationResult validate(const Sequence& sequence);
ValidationResult validate(const Project& project);

// ---------------------------------------------------------------------------
// Document: the project plus undo history
// ---------------------------------------------------------------------------

class Document {
public:
    Document() = default;
    explicit Document(Project project);

    const Project& project() const { return project_; }
    Project& mutableProject() { return project_; }

    // Replace the whole project and record an undo step.
    void commit(Project next);

    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    bool undo();
    bool redo();

    void clearHistory();

    std::size_t undoDepth() const { return undo_.size(); }
    std::size_t redoDepth() const { return redo_.size(); }

    // History is bounded so a long session cannot grow without limit.
    static constexpr std::size_t kMaxHistory = 200;

private:
    Project project_;
    std::vector<Project> undo_;
    std::vector<Project> redo_;
};

}  // namespace vx
