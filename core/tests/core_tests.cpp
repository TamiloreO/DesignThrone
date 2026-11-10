// Tests for the editing core. No framework dependency on purpose: the core has
// to build and test on any machine, including CI runners with nothing installed.

#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "vx/model.hpp"
#include "vx/project_io.hpp"
#include "vx/timeline.hpp"
#include "vx/time.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;
const char* g_currentTest = "";

void reportFailure(const char* file, int line, const std::string& message) {
    ++g_failures;
    std::printf("  FAIL [%s] %s:%d\n    %s\n", g_currentTest, file, line, message.c_str());
}

#define CHECK(condition)                                                        \
    do {                                                                        \
        ++g_checks;                                                             \
        if (!(condition)) {                                                     \
            reportFailure(__FILE__, __LINE__, "expected: " #condition);         \
        }                                                                       \
    } while (0)

#define CHECK_EQ(actual, expected)                                              \
    do {                                                                        \
        ++g_checks;                                                             \
        const auto actualValue = (actual);                                      \
        const auto expectedValue = (expected);                                  \
        if (!(actualValue == expectedValue)) {                                  \
            reportFailure(__FILE__, __LINE__,                                   \
                          std::string("expected ") + #actual + " == " + #expected); \
        }                                                                       \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                 \
    do {                                                                        \
        ++g_checks;                                                             \
        const double a = static_cast<double>(actual);                           \
        const double e = static_cast<double>(expected);                         \
        if ((a - e) > (tolerance) || (e - a) > (tolerance)) {                   \
            reportFailure(__FILE__, __LINE__,                                   \
                          std::string("expected ") + #actual + " near " + #expected); \
        }                                                                       \
    } while (0)

#define RUN(test)                                                               \
    do {                                                                        \
        g_currentTest = #test;                                                  \
        const int before = g_failures;                                          \
        test();                                                                 \
        std::printf("%s %s\n", (g_failures == before) ? "PASS" : "FAIL", #test); \
    } while (0)

using namespace vx;

constexpr SequenceId kSeq = 1;
constexpr TrackId kVideoTrack = 10;
constexpr TrackId kAudioTrack = 11;

Project makeProject() {
    Project project;
    project.name = "test";

    Sequence sequence;
    sequence.id = kSeq;
    sequence.name = "Sequence 1";
    sequence.width = 1920;
    sequence.height = 1080;
    sequence.frameRate = FrameRate(30000, 1001);

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

Clip makeClip(double startSeconds, double durationSeconds) {
    Clip clip;
    clip.media = 1;
    clip.start = Time::fromSecondsDouble(startSeconds);
    clip.sourceIn = Time::zero();
    clip.sourceOut = Time::fromSecondsDouble(durationSeconds);
    clip.speedNumerator = 1;
    clip.speedDenominator = 1;
    return clip;
}

const Track& videoTrack(const Project& project) {
    return project.sequences[0].tracks[0];
}

// ---------------------------------------------------------------------------

void testTimeArithmeticIsExact() {
    const auto oneFrame = frameDuration(FrameRate(30000, 1001));
    // 1001/30000 seconds is 33.3666... ms, which rounds to 33366667 ns.
    CHECK_EQ(oneFrame.ns, 33366667);

    // Ten thousand frames must land on the same nanosecond as the direct
    // computation, not drift by rounding.
    const auto byFrames = timeOfFrame(10000, FrameRate(30000, 1001));
    CHECK_EQ(byFrames.ns, 333666666667);

    CHECK_EQ(framesIn(byFrames, FrameRate(30000, 1001)), 10000);
    CHECK_EQ(timeOfFrame(30, FrameRate::fps(30)).ns, Time::kNsPerSecond);
    // 33 ms is closer to frame 1 (33.33 ms) than to frame 0.
    CHECK_EQ(snapToFrame(Time::fromMilliseconds(33), FrameRate::fps(30)).ns, 33333333);
    CHECK_EQ(snapToFrame(Time::fromMilliseconds(10), FrameRate::fps(30)).ns, 0);
}

void testTimecodeFormatting() {
    CHECK_EQ(timecodeString(Time::fromSeconds(3661), FrameRate::fps(30)),
             std::string("01:01:01:00"));
    CHECK_EQ(timecodeString(Time::zero(), FrameRate::fps(30)), std::string("00:00:00:00"));
}

void testClipDurationAndSpeedMapping() {
    Clip clip = makeClip(0, 10);
    CHECK_EQ(clip.duration().ns, Time::fromSeconds(10).ns);
    CHECK_EQ(clip.end().ns, Time::fromSeconds(10).ns);

    // 2x speed halves the sequence duration without changing the source range.
    clip.speedNumerator = 2;
    CHECK_EQ(clip.duration().ns, Time::fromSeconds(5).ns);

    // Halfway through the clip is halfway through the source range.
    CHECK_EQ(clip.sourceTimeAt(Time::fromSecondsDouble(2.5)).ns, Time::fromSeconds(5).ns);

    clip.speedNumerator = 1;
    CHECK_EQ(clip.sourceTimeAt(Time::fromSecondsDouble(4)).ns, Time::fromSeconds(4).ns);
}

void testAppendPlacesAfterLastClip() {
    Project project = makeProject();
    auto first = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(first.ok());
    CHECK_EQ(videoTrack(first.project).clips.size(), std::size_t(1));

    auto second = appendClip(first.project, kSeq, kVideoTrack, makeClip(0, 3));
    CHECK(second.ok());
    const auto& clips = videoTrack(second.project).clips;
    CHECK_EQ(clips.size(), std::size_t(2));
    CHECK_EQ(clips[1].start.ns, Time::fromSeconds(5).ns);
    CHECK_EQ(second.project.sequences[0].duration().ns, Time::fromSeconds(8).ns);
}

void testInsertRipplesLaterClips() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(b.ok());
    CHECK_EQ(videoTrack(b.project).clips[1].start.ns, Time::fromSeconds(5).ns);

    Clip inserted = makeClip(2, 2);
    auto result = insertClip(b.project, kSeq, kVideoTrack, inserted);
    CHECK(result.ok());

    // Inserting at 2s lands inside the first clip, so that clip is cut in two:
    // [0,2) stays, [2,4) is the new clip, the tail of the old clip becomes
    // [4,7), and the second clip is pushed to [7,12).
    const auto& clips = videoTrack(result.project).clips;
    CHECK_EQ(clips.size(), std::size_t(4));
    CHECK_EQ(clips[0].start.ns, Time::fromSeconds(0).ns);
    CHECK_EQ(clips[0].end().ns, Time::fromSeconds(2).ns);
    CHECK_EQ(clips[1].start.ns, Time::fromSeconds(2).ns);
    CHECK_EQ(clips[1].end().ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[2].start.ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[2].end().ns, Time::fromSeconds(7).ns);
    CHECK_EQ(clips[3].start.ns, Time::fromSeconds(7).ns);
    CHECK(validate(result.project).ok());
}

void testOverwriteSplitsStraddlingClip() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    CHECK(a.ok());

    Clip replacement = makeClip(4, 2);
    auto result = overwriteClip(a.project, kSeq, kVideoTrack, replacement);
    CHECK(result.ok());

    const auto& clips = videoTrack(result.project).clips;
    CHECK_EQ(clips.size(), std::size_t(3));
    CHECK_EQ(clips[0].start.ns, Time::fromSeconds(0).ns);
    CHECK_EQ(clips[0].end().ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[1].start.ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[1].end().ns, Time::fromSeconds(6).ns);
    CHECK_EQ(clips[2].start.ns, Time::fromSeconds(6).ns);
    CHECK_EQ(clips[2].end().ns, Time::fromSeconds(10).ns);
    CHECK_EQ(clips[2].sourceIn.ns, Time::fromSeconds(6).ns);
    CHECK(validate(result.project).ok());
}

void testLiftLeavesGapAndRippleDeleteClosesIt() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 4));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 4));
    CHECK(b.ok());

    const auto firstId = videoTrack(b.project).clips[0].id;
    const auto secondId = videoTrack(b.project).clips[1].id;

    auto lifted = liftClip(b.project, kSeq, kVideoTrack, firstId);
    CHECK(lifted.ok());
    CHECK_EQ(videoTrack(lifted.project).clips.size(), std::size_t(1));
    CHECK_EQ(videoTrack(lifted.project).clips[0].start.ns, Time::fromSeconds(4).ns);

    auto rippled = rippleDeleteClip(b.project, kSeq, kVideoTrack, firstId);
    CHECK(rippled.ok());
    CHECK_EQ(videoTrack(rippled.project).clips.size(), std::size_t(1));
    CHECK_EQ(videoTrack(rippled.project).clips[0].id, secondId);
    CHECK_EQ(videoTrack(rippled.project).clips[0].start.ns, Time::zero().ns);
}

void testSplitAtPlayheadKeepsSourceMapping() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    CHECK(a.ok());
    const auto id = videoTrack(a.project).clips[0].id;

    auto result = splitClip(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(4));
    CHECK(result.ok());

    const auto& clips = videoTrack(result.project).clips;
    CHECK_EQ(clips.size(), std::size_t(2));
    CHECK_EQ(clips[0].end().ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[1].start.ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[1].sourceIn.ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[1].sourceOut.ns, Time::fromSeconds(10).ns);
    CHECK_EQ(clips[0].id, id);
    CHECK(clips[0].id != clips[1].id);
    CHECK(validate(result.project).ok());
}

void testSplitAtSpeed() {
    Project project = makeProject();
    Clip clip = makeClip(0, 10);
    clip.speedNumerator = 2;  // Sequence duration 5, source range 0 to 10.
    auto a = appendClip(project, kSeq, kVideoTrack, clip);
    CHECK(a.ok());
    const auto id = videoTrack(a.project).clips[0].id;

    auto result = splitClip(a.project, kSeq, kVideoTrack, id, Time::fromSecondsDouble(2));
    CHECK(result.ok());

    const auto& clips = videoTrack(result.project).clips;
    CHECK_EQ(clips.size(), std::size_t(2));
    // Two seconds into a 2x clip is four seconds into the source.
    CHECK_EQ(clips[1].sourceIn.ns, Time::fromSeconds(4).ns);
    CHECK_EQ(clips[0].sourceOut.ns, Time::fromSeconds(4).ns);
}

void testSplitRejectsBoundaries() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    const auto id = videoTrack(a.project).clips[0].id;

    CHECK(splitClip(a.project, kSeq, kVideoTrack, id, Time::zero()).error ==
          EditError::InvalidRange);
    CHECK(splitClip(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(10)).error ==
          EditError::InvalidRange);
    CHECK(splitClip(a.project, kSeq, kVideoTrack, 9999, Time::fromSeconds(5)).error ==
          EditError::UnknownClip);
}

void testTrimStartWithAndWithoutRipple() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(b.ok());
    CHECK_EQ(videoTrack(b.project).clips.size(), std::size_t(2));

    const auto firstId = videoTrack(b.project).clips[0].id;

    // Without ripple the clip slides right and leaves a gap behind it.
    auto noRipple = trimStart(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(2), false);
    CHECK(noRipple.ok());
    const auto& clipsA = videoTrack(noRipple.project).clips;
    CHECK_EQ(clipsA[0].start.ns, Time::fromSeconds(2).ns);
    CHECK_EQ(clipsA[0].sourceIn.ns, Time::fromSeconds(2).ns);
    CHECK_EQ(clipsA[0].end().ns, Time::fromSeconds(10).ns);
    CHECK_EQ(clipsA[1].start.ns, Time::fromSeconds(10).ns);

    // With ripple the clip stays anchored and the following clips close the
    // gap, so the clips stay adjacent instead of overlapping.
    auto withRipple = trimStart(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(2), true);
    CHECK(withRipple.ok());
    const auto& clipsB = videoTrack(withRipple.project).clips;
    CHECK_EQ(clipsB[0].start.ns, Time::fromSeconds(0).ns);
    CHECK_EQ(clipsB[0].sourceIn.ns, Time::fromSeconds(2).ns);
    CHECK_EQ(clipsB[0].end().ns, Time::fromSeconds(8).ns);
    CHECK_EQ(clipsB[1].start.ns, Time::fromSeconds(8).ns);
}

void testTrimEndExtendsAndShortens() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    CHECK(a.ok());
    const auto id = videoTrack(a.project).clips[0].id;

    auto shorter = trimEnd(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(-3), false);
    CHECK(shorter.ok());
    CHECK_EQ(videoTrack(shorter.project).clips[0].end().ns, Time::fromSeconds(7).ns);
    CHECK_EQ(videoTrack(shorter.project).clips[0].sourceOut.ns, Time::fromSeconds(7).ns);

    auto longer = trimEnd(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(2), false);
    CHECK(longer.ok());
    CHECK_EQ(videoTrack(longer.project).clips[0].end().ns, Time::fromSeconds(12).ns);
    CHECK_EQ(videoTrack(longer.project).clips[0].sourceOut.ns, Time::fromSeconds(12).ns);
}

void testTrimEndWithRipple() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 5));
    const auto firstId = videoTrack(b.project).clips[0].id;

    auto shorter = trimEnd(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(-4), true);
    CHECK(shorter.ok());
    CHECK_EQ(videoTrack(shorter.project).clips[0].end().ns, Time::fromSeconds(6).ns);
    CHECK_EQ(videoTrack(shorter.project).clips[1].start.ns, Time::fromSeconds(6).ns);

    auto longer = trimEnd(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(2), true);
    CHECK(longer.ok());
    CHECK_EQ(videoTrack(longer.project).clips[0].end().ns, Time::fromSeconds(12).ns);
    CHECK_EQ(videoTrack(longer.project).clips[1].start.ns, Time::fromSeconds(12).ns);
}

void testTrimRespectsClipSpeed() {
    Project project = makeProject();
    auto clip = makeClip(0, 10);
    clip.speedNumerator = 2;
    clip.speedDenominator = 1;
    auto a = appendClip(project, kSeq, kVideoTrack, clip);
    CHECK(a.ok());
    const auto id = videoTrack(a.project).clips[0].id;
    // Ten seconds of source at 2x is five seconds on the timeline.
    CHECK_EQ(videoTrack(a.project).clips[0].duration().ns, Time::fromSeconds(5).ns);

    // One timeline second off the head consumes two source seconds.
    auto head = trimStart(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(1), false);
    CHECK(head.ok());
    CHECK_EQ(videoTrack(head.project).clips[0].sourceIn.ns, Time::fromSeconds(2).ns);
    CHECK_EQ(videoTrack(head.project).clips[0].duration().ns, Time::fromSeconds(4).ns);

    // Two timeline seconds off the tail consumes four source seconds.
    auto tail = trimEnd(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(-2), false);
    CHECK(tail.ok());
    CHECK_EQ(videoTrack(tail.project).clips[0].sourceOut.ns, Time::fromSeconds(6).ns);
    CHECK_EQ(videoTrack(tail.project).clips[0].duration().ns, Time::fromSeconds(3).ns);
}

void testTrimCannotInvertClip() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    const auto id = videoTrack(a.project).clips[0].id;

    const auto result = trimEnd(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(-10), false);
    CHECK(result.error == EditError::WouldProduceEmptyClip);
    CHECK(result.project.sequences.empty());

    const auto head = trimStart(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(10), false);
    CHECK(head.error == EditError::WouldProduceEmptyClip);
    CHECK(head.project.sequences.empty());
}

void testTrimStartClampsAtMediaHead() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 10));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 5));
    const auto firstId = videoTrack(b.project).clips[0].id;

    // There is no media before sourceIn, so the request is clamped instead of
    // pulling in frames that do not exist.
    auto clamped = trimStart(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(-2), false);
    CHECK(clamped.ok());
    CHECK_EQ(videoTrack(clamped.project).clips[0].start.ns, Time::fromSeconds(0).ns);
    CHECK_EQ(videoTrack(clamped.project).clips[0].sourceIn.ns, Time::fromSeconds(0).ns);
    CHECK_EQ(videoTrack(clamped.project).clips[1].start.ns, Time::fromSeconds(10).ns);
}

void testSecondsDoubleRoundsSymmetrically() {
    CHECK_EQ(Time::fromSecondsDouble(-3).ns, std::int64_t(-3'000'000'000));
    CHECK_EQ(Time::fromSecondsDouble(3).ns, std::int64_t(3'000'000'000));
    CHECK_EQ(Time::fromSecondsDouble(-0.5).ns, std::int64_t(-500'000'000));
    CHECK_EQ(Time::fromSecondsDouble(2.5).ns, std::int64_t(2'500'000'000));
}

void testMulDivIsExactAndPortable() {
    CHECK_EQ(mulDiv(10'000'000'000, 3, 2), std::int64_t(15'000'000'000));
    CHECK_EQ(mulDiv(-10'000'000'000, 3, 2), std::int64_t(-15'000'000'000));
    // Truncation toward zero, matching integer division.
    CHECK_EQ(mulDiv(7, 1, 2), std::int64_t(3));
    CHECK_EQ(mulDiv(-7, 1, 2), std::int64_t(-3));
    CHECK_EQ(mulDiv(0, 1'000'000, 3), std::int64_t(0));

    // A full day at 1000x does not fit in a 64 bit product.
    const auto day = Time::fromSeconds(86400).ns;
    CHECK_EQ(mulDiv(day, 1000, 1), day * 1000);
    CHECK_EQ(mulDiv(day * 1000, 1, 1000), day);

    // Out of range results saturate instead of wrapping.
    const auto max = std::numeric_limits<std::int64_t>::max();
    CHECK_EQ(mulDiv(max, 2, 1), max);
    CHECK_EQ(mulDiv(-max, 2, 1), std::numeric_limits<std::int64_t>::min());
    CHECK_EQ(mulDiv(-max, 1, 1), -max);

#if defined(__SIZEOF_INT128__)
    // Cross-check against the compiler's own 128 bit arithmetic across a spread
    // of values, including ones where a 64 bit product would overflow.
    const std::int64_t values[] = {0,      1,      -1,    7,      -7,    day,
                                   -day,   day * 1000, max / 4, max / 1'000'000,
                                   std::numeric_limits<std::int64_t>::min() / 4};
    const std::int64_t numerators[] = {1, 2, 3, 999, 1'000'000, -1, -7};
    const std::int64_t denominators[] = {1, 2, 3, 7, 1001, 30'000};
    for (const auto value : values) {
        for (const auto numerator : numerators) {
            for (const auto denominator : denominators) {
                const auto exact = static_cast<__int128>(value) * static_cast<__int128>(numerator) /
                                   static_cast<__int128>(denominator);
                std::int64_t expected = 0;
                if (exact > static_cast<__int128>(max)) {
                    expected = max;
                } else if (exact < static_cast<__int128>(std::numeric_limits<std::int64_t>::min())) {
                    expected = std::numeric_limits<std::int64_t>::min();
                } else {
                    expected = static_cast<std::int64_t>(exact);
                }
                CHECK_EQ(mulDiv(value, numerator, denominator), expected);
            }
        }
    }
#endif
}

void testProjectJsonRoundTripIsExact() {
    Project project = makeProject();
    project.name = "Round trip";

    MediaItem media;
    media.id = 1;
    media.path = "Media/clip one.mov";
    media.displayName = "clip \"one\"\nsecond line";
    media.duration = Time::fromSeconds(12) + Time::fromNanoseconds(345678901);
    media.width = 3840;
    media.height = 2160;
    media.frameRate = FrameRate(30000, 1001);
    media.contentHash = "abc123";
    project.media.push_back(media);

    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 4));
    CHECK(a.ok());
    Clip odd = makeClip(0, 9);
    odd.speedNumerator = 3;
    odd.speedDenominator = 2;
    odd.sourceIn = Time::fromNanoseconds(1234567);
    odd.sourceOut = Time::fromNanoseconds(9000001234);
    odd.name = "odd clip";
    auto b = appendClip(a.project, kSeq, kVideoTrack, odd);
    CHECK(b.ok());

    const auto text = toJson(b.project);
    const auto parsed = parseProjectJson(text);
    CHECK(parsed.ok);
    CHECK_EQ(parsed.error, std::string());

    // Times have to come back as the same nanoseconds, not as a rounded second.
    const auto& before = videoTrack(b.project).clips;
    const auto& after = videoTrack(parsed.project).clips;
    CHECK_EQ(after.size(), before.size());
    for (std::size_t i = 0; i < before.size() && i < after.size(); ++i) {
        CHECK_EQ(after[i].id, before[i].id);
        CHECK_EQ(after[i].start.ns, before[i].start.ns);
        CHECK_EQ(after[i].sourceIn.ns, before[i].sourceIn.ns);
        CHECK_EQ(after[i].sourceOut.ns, before[i].sourceOut.ns);
        CHECK_EQ(after[i].speedNumerator, before[i].speedNumerator);
        CHECK_EQ(after[i].speedDenominator, before[i].speedDenominator);
        CHECK_EQ(after[i].name, before[i].name);
    }
    CHECK_EQ(parsed.project.name, std::string("Round trip"));
    CHECK_EQ(parsed.project.media.size(), std::size_t(1));
    CHECK_EQ(parsed.project.media[0].duration.ns, media.duration.ns);
    CHECK_EQ(parsed.project.media[0].displayName, media.displayName);
    CHECK_EQ(parsed.project.media[0].width, 3840);
    CHECK_EQ(parsed.project.media[0].frameRate.numerator, 30000);

    // The text is stable, so a project that is loaded and saved does not churn.
    CHECK_EQ(toJson(parsed.project), text);
    CHECK(validate(parsed.project).ok());
}

void testProjectJsonRejectsBadInput() {
    CHECK(!parseProjectJson("").ok);
    CHECK(!parseProjectJson("{").ok);
    CHECK(!parseProjectJson("[]").ok);
    CHECK(!parseProjectJson("{\"format\":\"other\",\"version\":1}").ok);

    // A newer schema is refused with an explanation, not guessed at.
    const auto future = parseProjectJson(
        "{\"format\":\"vx.project\",\"version\":99,\"sequences\":[{\"id\":1,\"tracks\":[]}]}");
    CHECK(!future.ok);
    CHECK(future.error.find("newer") != std::string::npos);

    // Missing sequences is a broken project, not an empty one.
    const auto empty = parseProjectJson("{\"format\":\"vx.project\",\"version\":1,\"sequences\":[]}");
    CHECK(!empty.ok);

    // Type errors name the field that is wrong.
    const auto wrongType = parseProjectJson(
        "{\"format\":\"vx.project\",\"version\":1,\"sequences\":[{\"id\":\"one\"}]}");
    CHECK(!wrongType.ok);
    CHECK(wrongType.error.find("id") != std::string::npos);

    // Overlapping clips would corrupt the timeline, so the file is refused.
    const auto overlap = parseProjectJson(
        "{\"format\":\"vx.project\",\"version\":1,\"sequences\":[{\"id\":1,\"width\":1920,"
        "\"height\":1080,\"frameRate\":{\"numerator\":30,\"denominator\":1},\"tracks\":[{\"id\":10,"
        "\"kind\":\"video\",\"clips\":["
        "{\"id\":2,\"media\":1,\"startNs\":0,\"sourceInNs\":0,\"sourceOutNs\":5000000000},"
        "{\"id\":3,\"media\":1,\"startNs\":1000000000,\"sourceInNs\":0,\"sourceOutNs\":5000000000}"
        "]}]}]}");
    CHECK(!overlap.ok);
}

void testProjectJsonToleratesUnknownFieldsAndFixesIds() {
    // A project saved by a later build with extra fields still opens, and the id
    // counter is repaired so a new clip cannot collide with a loaded one.
    const auto parsed = parseProjectJson(
        "{\"format\":\"vx.project\",\"version\":1,\"name\":\"From the future\",\"nextId\":2,"
        "\"colourSpace\":\"rec2020\","
        "\"sequences\":[{\"id\":1,\"width\":1920,\"height\":1080,"
        "\"frameRate\":{\"numerator\":30,\"denominator\":1},\"tracks\":[{\"id\":10,"
        "\"kind\":\"video\",\"clips\":["
        "{\"id\":40,\"media\":7,\"startNs\":0,\"sourceInNs\":0,\"sourceOutNs\":2000000000,"
        "\"blendMode\":\"screen\"}]}]}]}");
    CHECK(parsed.ok);
    CHECK(parsed.project.nextId > 40);
    CHECK_EQ(parsed.project.name, std::string("From the future"));
    CHECK_EQ(videoTrack(parsed.project).clips.size(), std::size_t(1));

    Project next = parsed.project;
    Clip added = makeClip(0, 1);
    const auto appended = appendClip(next, kSeq, kVideoTrack, added);
    CHECK(appended.ok());
    CHECK(videoTrack(appended.project).clips.size() == 2);
}

void testProjectFileRoundTrip() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(a.ok());

    const std::string path = "/tmp/vx_round_trip.vxproj";
    std::string error;
    CHECK(saveProjectToFile(a.project, path, &error));
    CHECK_EQ(error, std::string());

    Project loaded;
    CHECK(loadProjectFromFile(path, loaded, &error));
    CHECK_EQ(videoTrack(loaded).clips.size(), std::size_t(1));
    CHECK_EQ(videoTrack(loaded).clips[0].duration().ns, Time::fromSeconds(5).ns);

    CHECK(!loadProjectFromFile("/tmp/vx_does_not_exist.vxproj", loaded, &error));
    CHECK(!error.empty());
}

void testMoveRejectsOverlap() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    auto b = appendClip(a.project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(b.ok());

    const auto firstId = videoTrack(b.project).clips[0].id;

    auto blocked = moveClip(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(2));
    CHECK(blocked.error == EditError::OverlapsExistingClip);

    auto moved = moveClip(b.project, kSeq, kVideoTrack, firstId, Time::fromSeconds(10));
    CHECK(moved.ok());
    CHECK_EQ(videoTrack(moved.project).clips[1].start.ns, Time::fromSeconds(10).ns);
    CHECK(validate(moved.project).ok());
}

void testFailedEditLeavesProjectUntouched() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    const auto id = videoTrack(a.project).clips[0].id;
    const auto idBefore = a.project.nextId;

    const auto result = splitClip(a.project, kSeq, kVideoTrack, id, Time::fromSeconds(5));
    CHECK(!result.ok());
    CHECK_EQ(videoTrack(a.project).clips.size(), std::size_t(1));
    CHECK_EQ(a.project.nextId, idBefore);
    CHECK(result.project.sequences.empty());
}

void testValidationCatchesOverlap() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(a.ok());

    Project broken = a.project;
    Clip overlapping = makeClip(3, 4);
    overlapping.id = 12345;
    broken.sequences[0].tracks[0].clips.push_back(overlapping);

    const auto result = validate(broken);
    CHECK(result.error == ValidationError::OverlappingClips);
}

void testUndoAndRedoRestoreExactState() {
    Project project = makeProject();
    Document document(project);
    CHECK(!document.canUndo());

    auto first = appendClip(document.project(), kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(first.ok());
    document.commit(first.project);
    CHECK(document.canUndo());
    CHECK_EQ(videoTrack(document.project()).clips.size(), std::size_t(1));

    auto second = appendClip(document.project(), kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(second.ok());
    document.commit(second.project);
    CHECK_EQ(videoTrack(document.project()).clips.size(), std::size_t(2));

    CHECK(document.undo());
    CHECK_EQ(videoTrack(document.project()).clips.size(), std::size_t(1));
    CHECK(document.canRedo());

    CHECK(document.redo());
    CHECK_EQ(videoTrack(document.project()).clips.size(), std::size_t(2));
    CHECK(!document.canRedo());
}

void testClipLookup() {
    Project project = makeProject();
    auto a = appendClip(project, kSeq, kVideoTrack, makeClip(0, 5));
    CHECK(a.ok());

    const auto& sequence = a.project.sequences[0];
    CHECK(clipAt(sequence.tracks[0], Time::fromSeconds(2)) != nullptr);
    CHECK(clipAt(sequence.tracks[0], Time::fromSeconds(5)) == nullptr);
    CHECK(clipAt(sequence.tracks[0], Time::fromSeconds(4)) != nullptr);

    const auto id = sequence.tracks[0].clips[0].id;
    const auto location = locateClip(sequence, id);
    CHECK(location.has_value());
    CHECK_EQ(location->trackIndex, std::size_t(0));
    CHECK(!locateClip(sequence, 424242).has_value());
}

}  // namespace

int main() {
    std::printf("core tests\n");

    RUN(testTimeArithmeticIsExact);
    RUN(testTimecodeFormatting);
    RUN(testClipDurationAndSpeedMapping);
    RUN(testAppendPlacesAfterLastClip);
    RUN(testInsertRipplesLaterClips);
    RUN(testOverwriteSplitsStraddlingClip);
    RUN(testLiftLeavesGapAndRippleDeleteClosesIt);
    RUN(testSplitAtPlayheadKeepsSourceMapping);
    RUN(testSplitAtSpeed);
    RUN(testSplitRejectsBoundaries);
    RUN(testTrimStartWithAndWithoutRipple);
    RUN(testTrimEndExtendsAndShortens);
    RUN(testTrimEndWithRipple);
    RUN(testTrimRespectsClipSpeed);
    RUN(testTrimCannotInvertClip);
    RUN(testTrimStartClampsAtMediaHead);
    RUN(testSecondsDoubleRoundsSymmetrically);
    RUN(testMulDivIsExactAndPortable);
    RUN(testProjectJsonRoundTripIsExact);
    RUN(testProjectJsonRejectsBadInput);
    RUN(testProjectJsonToleratesUnknownFieldsAndFixesIds);
    RUN(testProjectFileRoundTrip);
    RUN(testMoveRejectsOverlap);
    RUN(testFailedEditLeavesProjectUntouched);
    RUN(testValidationCatchesOverlap);
    RUN(testUndoAndRedoRestoreExactState);
    RUN(testClipLookup);

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
