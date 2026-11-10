#include "vx/c_api.h"

#include <utility>

#include "vx/model.hpp"
#include "vx/timeline.hpp"

using namespace vx;

// Phase 0 has exactly one sequence with one video and one audio track. The C ABI
// keeps that assumption in one place so the Swift layer never hardcodes ids.
namespace {

constexpr SequenceId kSequence = 1;
constexpr TrackId kVideoTrack = 10;
constexpr TrackId kAudioTrack = 11;

Project makeEmptyProject() {
    Project project;
    project.name = "Untitled";

    Sequence sequence;
    sequence.id = kSequence;
    sequence.name = "Sequence 1";
    sequence.width = 1920;
    sequence.height = 1080;
    sequence.frameRate = FrameRate::fps(30);

    Track video;
    video.id = kVideoTrack;
    video.kind = TrackKind::Video;
    video.name = "V1";

    Track audio;
    audio.id = kAudioTrack;
    audio.kind = TrackKind::Audio;
    audio.name = "A1";

    sequence.tracks.push_back(video);
    sequence.tracks.push_back(audio);
    project.sequences.push_back(sequence);
    project.nextId = 100;
    return project;
}

const Track* videoTrackOf(const Project& project) {
    const auto* sequence = findSequence(project, kSequence);
    if (sequence == nullptr) return nullptr;
    return findTrack(*sequence, kVideoTrack);
}

std::int32_t statusOf(EditError error) {
    switch (error) {
        case EditError::None: return VXE_OK;
        case EditError::UnknownSequence: return VXE_UNKNOWN_TRACK;
        case EditError::UnknownTrack: return VXE_UNKNOWN_TRACK;
        case EditError::TrackKindMismatch: return VXE_UNKNOWN_TRACK;
        case EditError::UnknownClip: return VXE_UNKNOWN_CLIP;
        case EditError::InvalidRange: return VXE_INVALID_RANGE;
        case EditError::SourceRangeExceeded: return VXE_INVALID_RANGE;
        case EditError::OverlapsExistingClip: return VXE_OVERLAP;
        case EditError::WouldProduceEmptyClip: return VXE_EMPTY_CLIP;
        case EditError::ValidationFailed: return VXE_VALIDATION_FAILED;
    }
    return VXE_VALIDATION_FAILED;
}

// Commits a successful edit, which is what creates the undo step. A failed edit
// leaves the document untouched.
std::int32_t apply(Document& document, EditResult result, std::uint64_t* outClip) {
    const auto status = statusOf(result.error);
    if (status != VXE_OK) return status;
    if (outClip != nullptr) *outClip = result.insertedClip;
    document.commit(std::move(result.project));
    return VXE_OK;
}

}  // namespace

struct vxe_project {
    Document document{makeEmptyProject()};
};

extern "C" {

vxe_project* vxe_project_create(void) { return new vxe_project(); }

void vxe_project_destroy(vxe_project* project) { delete project; }

void vxe_project_reset(vxe_project* project) {
    if (project == nullptr) return;
    project->document = Document(makeEmptyProject());
}

int32_t vxe_clip_count(const vxe_project* project) {
    if (project == nullptr) return -1;
    const auto* track = videoTrackOf(project->document.project());
    if (track == nullptr) return -1;
    return static_cast<std::int32_t>(track->clips.size());
}

int32_t vxe_copy_clips(const vxe_project* project, vxe_clip* out, int32_t capacity) {
    if (project == nullptr || out == nullptr || capacity < 0) return -1;
    const auto* track = videoTrackOf(project->document.project());
    if (track == nullptr) return -1;

    std::int32_t written = 0;
    for (const auto& clip : track->clips) {
        if (written >= capacity) break;
        vxe_clip snapshot;
        snapshot.id = clip.id;
        snapshot.media = clip.media;
        snapshot.start = clip.start.seconds();
        snapshot.duration = clip.duration().seconds();
        snapshot.source_in = clip.sourceIn.seconds();
        snapshot.source_out = clip.sourceOut.seconds();
        snapshot.speed = clip.speed().value();
        out[written] = snapshot;
        ++written;
    }
    return written;
}

double vxe_duration(const vxe_project* project) {
    if (project == nullptr) return 0.0;
    const auto* sequence = findSequence(project->document.project(), kSequence);
    if (sequence == nullptr) return 0.0;
    return sequence->duration().seconds();
}

int32_t vxe_append(vxe_project* project, uint64_t media, double source_in, double source_out,
                   uint64_t* out_clip) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;

    Clip clip;
    clip.media = media;
    clip.sourceIn = Time::fromSecondsDouble(source_in);
    clip.sourceOut = Time::fromSecondsDouble(source_out);
    if (clip.sourceOut <= clip.sourceIn) return VXE_INVALID_RANGE;

    return apply(project->document,
                 appendClip(project->document.project(), kSequence, kVideoTrack, clip), out_clip);
}

int32_t vxe_insert(vxe_project* project, uint64_t media, double start, double source_in,
                   double source_out, uint64_t* out_clip) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;

    Clip clip;
    clip.media = media;
    clip.start = Time::fromSecondsDouble(start);
    clip.sourceIn = Time::fromSecondsDouble(source_in);
    clip.sourceOut = Time::fromSecondsDouble(source_out);
    if (clip.start.isNegative()) return VXE_INVALID_RANGE;
    if (clip.sourceOut <= clip.sourceIn) return VXE_INVALID_RANGE;

    return apply(project->document,
                 insertClip(project->document.project(), kSequence, kVideoTrack, clip), out_clip);
}

int32_t vxe_trim_start(vxe_project* project, uint64_t clip, double delta, int32_t ripple) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return apply(project->document,
                 trimStart(project->document.project(), kSequence, kVideoTrack, clip,
                           Time::fromSecondsDouble(delta), ripple != 0),
                 nullptr);
}

int32_t vxe_trim_end(vxe_project* project, uint64_t clip, double delta, int32_t ripple) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return apply(project->document,
                 trimEnd(project->document.project(), kSequence, kVideoTrack, clip,
                         Time::fromSecondsDouble(delta), ripple != 0),
                 nullptr);
}

int32_t vxe_split(vxe_project* project, uint64_t clip, double at, uint64_t* out_clip) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return apply(project->document,
                 splitClip(project->document.project(), kSequence, kVideoTrack, clip,
                           Time::fromSecondsDouble(at)),
                 out_clip);
}

int32_t vxe_move(vxe_project* project, uint64_t clip, double start) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return apply(project->document,
                 moveClip(project->document.project(), kSequence, kVideoTrack, clip,
                          Time::fromSecondsDouble(start)),
                 nullptr);
}

int32_t vxe_remove(vxe_project* project, uint64_t clip, int32_t ripple) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    const auto edit = ripple != 0
                          ? rippleDeleteClip(project->document.project(), kSequence, kVideoTrack,
                                             clip)
                          : liftClip(project->document.project(), kSequence, kVideoTrack, clip);
    return apply(project->document, edit, nullptr);
}

int32_t vxe_can_undo(const vxe_project* project) {
    if (project == nullptr) return 0;
    return project->document.canUndo() ? 1 : 0;
}

int32_t vxe_can_redo(const vxe_project* project) {
    if (project == nullptr) return 0;
    return project->document.canRedo() ? 1 : 0;
}

int32_t vxe_undo(vxe_project* project) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return project->document.undo() ? VXE_OK : VXE_INVALID_ARGUMENT;
}

int32_t vxe_redo(vxe_project* project) {
    if (project == nullptr) return VXE_INVALID_ARGUMENT;
    return project->document.redo() ? VXE_OK : VXE_INVALID_ARGUMENT;
}

const char* vxe_status_string(int32_t status) {
    switch (status) {
        case VXE_OK: return "ok";
        case VXE_UNKNOWN_TRACK: return "unknown track";
        case VXE_UNKNOWN_CLIP: return "unknown clip";
        case VXE_INVALID_RANGE: return "invalid range";
        case VXE_EMPTY_CLIP: return "would produce an empty clip";
        case VXE_OVERLAP: return "overlaps existing clip";
        case VXE_VALIDATION_FAILED: return "validation failed";
        case VXE_INVALID_ARGUMENT: return "invalid argument";
    }
    return "unknown status";
}

}  // extern "C"
