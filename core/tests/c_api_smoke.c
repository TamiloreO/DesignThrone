/* Smoke test for the C ABI, compiled as C11 rather than C++ so that a C host
 * (the Swift bridging header, or a future C# / Rust front end) is proven to link
 * and behave. The exhaustive editing tests live in core_tests.cpp. */

#include <stdio.h>
#include <stdlib.h>

#include "vx/c_api.h"

static int g_failures = 0;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);      \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main(void) {
    vxe_project* project = vxe_project_create();
    CHECK(project != NULL);
    CHECK(vxe_clip_count(project) == 0);
    CHECK(vxe_duration(project) == 0.0);
    CHECK(vxe_can_undo(project) == 0);

    uint64_t first = 0;
    CHECK(vxe_append(project, 7, 0.0, 10.0, &first) == VXE_OK);
    CHECK(first != 0);
    CHECK(vxe_clip_count(project) == 1);
    CHECK(vxe_duration(project) == 10.0);

    uint64_t second = 0;
    CHECK(vxe_append(project, 7, 0.0, 5.0, &second) == VXE_OK);
    CHECK(vxe_clip_count(project) == 2);
    CHECK(vxe_duration(project) == 15.0);

    vxe_clip clips[4];
    CHECK(vxe_copy_clips(project, clips, 4) == 2);
    CHECK(clips[0].id == first);
    CHECK(clips[0].start == 0.0);
    CHECK(clips[0].duration == 10.0);
    CHECK(clips[0].media == 7);
    CHECK(clips[0].speed == 1.0);
    CHECK(clips[1].id == second);
    CHECK(clips[1].start == 10.0);

    /* Trimming the head with ripple keeps the clips adjacent. */
    CHECK(vxe_trim_start(project, first, 2.0, 1) == VXE_OK);
    CHECK(vxe_copy_clips(project, clips, 4) == 2);
    CHECK(clips[0].duration == 8.0);
    CHECK(clips[1].start == 8.0);

    /* Trimming the tail shortens the clip without moving it. */
    CHECK(vxe_trim_end(project, first, -3.0, 1) == VXE_OK);
    CHECK(vxe_copy_clips(project, clips, 4) == 2);
    CHECK(clips[0].duration == 5.0);
    CHECK(clips[1].start == 5.0);

    /* Splitting adds a clip and the halves stay adjacent. */
    uint64_t right = 0;
    CHECK(vxe_split(project, first, 2.0, &right) == VXE_OK);
    CHECK(right != 0);
    CHECK(vxe_clip_count(project) == 3);
    CHECK(vxe_copy_clips(project, clips, 4) == 3);
    CHECK(clips[0].duration == 2.0);
    CHECK(clips[1].start == 2.0);
    CHECK(clips[1].id == right);
    CHECK(clips[1].duration == 3.0);

    /* Undo restores the pre-split state, redo puts it back. */
    CHECK(vxe_can_undo(project) == 1);
    CHECK(vxe_undo(project) == VXE_OK);
    CHECK(vxe_clip_count(project) == 2);
    CHECK(vxe_can_redo(project) == 1);
    CHECK(vxe_redo(project) == VXE_OK);
    CHECK(vxe_clip_count(project) == 3);

    /* Errors are reported, not applied. */
    CHECK(vxe_trim_end(project, first, -100.0, 0) == VXE_EMPTY_CLIP);
    CHECK(vxe_trim_start(project, 999999, 1.0, 0) == VXE_UNKNOWN_CLIP);
    /* Moving the head clip onto its neighbour is refused, not clamped. */
    CHECK(vxe_move(project, first, 3.0) == VXE_OVERLAP);
    CHECK(vxe_move(project, first, 100.0) == VXE_OK);
    CHECK(vxe_move(project, first, 0.0) == VXE_OK);
    CHECK(vxe_append(project, 7, 5.0, 5.0, NULL) == VXE_INVALID_RANGE);
    CHECK(vxe_clip_count(project) == 3);
    CHECK(vxe_copy_clips(project, NULL, 4) == -1);
    CHECK(vxe_copy_clips(project, clips, 1) == 1);
    CHECK(vxe_status_string(VXE_EMPTY_CLIP) != NULL);

    /* Removing with ripple closes the gap. */
    CHECK(vxe_remove(project, right, 1) == VXE_OK);
    CHECK(vxe_clip_count(project) == 2);
    CHECK(vxe_copy_clips(project, clips, 4) == 2);
    CHECK(clips[1].start == 2.0);

    vxe_project_reset(project);
    CHECK(vxe_clip_count(project) == 0);
    CHECK(vxe_can_undo(project) == 0);

    vxe_project_destroy(project);

    printf("%s C ABI smoke test\n", g_failures == 0 ? "PASS" : "FAIL");
    return g_failures == 0 ? 0 : 1;
}
