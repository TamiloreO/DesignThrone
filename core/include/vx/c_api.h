#pragma once

// C ABI over the editing core.
//
// The Swift and (later) Windows front ends both talk to the engine through this
// header. Keeping the boundary in C means neither app needs C++ interop, and the
// ABI stays stable when the C++ internals change.
//
// Times crossing this boundary are seconds as doubles, because that is what the
// UI works in (AVFoundation, gestures, timecode fields). They are converted to
// exact nanoseconds once, inside the engine.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vxe_project vxe_project;

// Status codes are plain integer constants rather than an enum so that every
// language binds them the same way. Swift, for example, imports these as Int32
// without needing an enum bridge.
#define VXE_OK 0
#define VXE_UNKNOWN_TRACK 1
#define VXE_UNKNOWN_CLIP 2
#define VXE_INVALID_RANGE 3
#define VXE_EMPTY_CLIP 4
#define VXE_OVERLAP 5
#define VXE_VALIDATION_FAILED 6
#define VXE_INVALID_ARGUMENT 7

// A flat snapshot of one clip, laid out for easy use from Swift.
typedef struct vxe_clip {
    uint64_t id;
    uint64_t media;
    double start;       // position on the sequence, seconds
    double duration;    // length on the sequence, seconds
    double source_in;   // in point in the source media, seconds
    double source_out;  // out point in the source media, seconds
    double speed;       // 1.0 is normal speed
} vxe_clip;

vxe_project* vxe_project_create(void);
void vxe_project_destroy(vxe_project* project);

// Clears the timeline and the undo history.
void vxe_project_reset(vxe_project* project);

// Number of clips on the video track.
int32_t vxe_clip_count(const vxe_project* project);

// Copies up to capacity clips into out, in timeline order. Returns the number
// written, or -1 if the arguments are invalid.
int32_t vxe_copy_clips(const vxe_project* project, vxe_clip* out, int32_t capacity);

// Total sequence duration in seconds.
double vxe_duration(const vxe_project* project);

int32_t vxe_append(vxe_project* project, uint64_t media, double source_in, double source_out,
                   uint64_t* out_clip);
int32_t vxe_insert(vxe_project* project, uint64_t media, double start, double source_in,
                   double source_out, uint64_t* out_clip);
int32_t vxe_trim_start(vxe_project* project, uint64_t clip, double delta, int32_t ripple);
int32_t vxe_trim_end(vxe_project* project, uint64_t clip, double delta, int32_t ripple);
int32_t vxe_split(vxe_project* project, uint64_t clip, double at, uint64_t* out_clip);
int32_t vxe_move(vxe_project* project, uint64_t clip, double start);
int32_t vxe_remove(vxe_project* project, uint64_t clip, int32_t ripple);

int32_t vxe_can_undo(const vxe_project* project);
int32_t vxe_can_redo(const vxe_project* project);
int32_t vxe_undo(vxe_project* project);
int32_t vxe_redo(vxe_project* project);

const char* vxe_status_string(int32_t status);

#ifdef __cplusplus
}  // extern "C"
#endif
