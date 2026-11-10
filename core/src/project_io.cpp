#include "vx/project_io.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace vx {

const char* const kProjectFormatName = "vx.project";

namespace {

// ---------------------------------------------------------------------------
// A tiny JSON tree. The manifest is small and the core has no dependencies, so
// a few hundred lines here beats vendoring a parser for the whole platform.
// ---------------------------------------------------------------------------

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    bool isInteger = true;
    std::int64_t integer = 0;
    double number = 0.0;
    std::string text;
    std::vector<JsonValue> items;
    std::vector<std::pair<std::string, JsonValue>> members;

    static JsonValue makeBool(bool value) {
        JsonValue node;
        node.type = Type::Bool;
        node.boolean = value;
        return node;
    }
    static JsonValue makeInt(std::int64_t value) {
        JsonValue node;
        node.type = Type::Number;
        node.isInteger = true;
        node.integer = value;
        node.number = static_cast<double>(value);
        return node;
    }
    static JsonValue makeString(std::string value) {
        JsonValue node;
        node.type = Type::String;
        node.text = std::move(value);
        return node;
    }
    static JsonValue makeObject() {
        JsonValue node;
        node.type = Type::Object;
        return node;
    }
    static JsonValue makeArray() {
        JsonValue node;
        node.type = Type::Array;
        return node;
    }

    JsonValue& set(const char* key, JsonValue value) {
        members.emplace_back(key, std::move(value));
        return *this;
    }
};

void appendEscaped(std::string& out, const std::string& value) {
    out += '"';
    for (const char raw : value) {
        const auto c = static_cast<unsigned char>(raw);
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (c < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(c));
                    out += buffer;
                } else {
                    out += raw;
                }
        }
    }
    out += '"';
}

void dump(const JsonValue& value, std::string& out) {
    switch (value.type) {
        case JsonValue::Type::Null: out += "null"; break;
        case JsonValue::Type::Bool: out += value.boolean ? "true" : "false"; break;
        case JsonValue::Type::Number:
            if (value.isInteger) {
                out += std::to_string(value.integer);
            } else {
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%.17g", value.number);
                out += buffer;
            }
            break;
        case JsonValue::Type::String: appendEscaped(out, value.text); break;
        case JsonValue::Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& item : value.items) {
                if (!first) out += ',';
                first = false;
                dump(item, out);
            }
            out += ']';
            break;
        }
        case JsonValue::Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& member : value.members) {
                if (!first) out += ',';
                first = false;
                appendEscaped(out, member.first);
                out += ':';
                dump(member.second, out);
            }
            out += '}';
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool parseDocument(JsonValue& out, std::string& error) {
        skipWhitespace();
        if (!parseValue(out, 0)) {
            error = error_.empty() ? "invalid JSON" : error_;
            return false;
        }
        skipWhitespace();
        if (position_ != text_.size()) {
            error = "unexpected trailing characters";
            return false;
        }
        return true;
    }

private:
    static constexpr int kMaxDepth = 64;

    bool fail(const char* message) {
        if (error_.empty()) {
            error_ = message;
            errorPosition_ = position_;
        }
        return false;
    }

    void skipWhitespace() {
        while (position_ < text_.size()) {
            const char c = text_[position_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++position_;
            } else {
                break;
            }
        }
    }

    bool consume(const char* literal) {
        const std::size_t length = std::char_traits<char>::length(literal);
        if (text_.compare(position_, length, literal) != 0) return false;
        position_ += length;
        return true;
    }

    bool parseValue(JsonValue& out, int depth) {
        if (depth > kMaxDepth) return fail("document nested too deeply");
        skipWhitespace();
        if (position_ >= text_.size()) return fail("unexpected end of input");

        const char c = text_[position_];
        switch (c) {
            case '{': return parseObject(out, depth);
            case '[': return parseArray(out, depth);
            case '"': {
                std::string value;
                if (!parseString(value)) return false;
                out = JsonValue::makeString(std::move(value));
                return true;
            }
            case 't':
                if (!consume("true")) return fail("invalid literal");
                out = JsonValue::makeBool(true);
                return true;
            case 'f':
                if (!consume("false")) return fail("invalid literal");
                out = JsonValue::makeBool(false);
                return true;
            case 'n':
                if (!consume("null")) return fail("invalid literal");
                out = JsonValue();
                return true;
            default: return parseNumber(out);
        }
    }

    bool parseObject(JsonValue& out, int depth) {
        if (text_[position_] != '{') return fail("expected an object");
        ++position_;
        out = JsonValue::makeObject();
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return true;
        }
        while (true) {
            skipWhitespace();
            std::string key;
            if (!parseString(key)) return false;
            skipWhitespace();
            if (position_ >= text_.size() || text_[position_] != ':') return fail("expected ':'");
            ++position_;
            JsonValue value;
            if (!parseValue(value, depth + 1)) return false;
            out.members.emplace_back(std::move(key), std::move(value));
            skipWhitespace();
            if (position_ >= text_.size()) return fail("unterminated object");
            if (text_[position_] == ',') {
                ++position_;
                continue;
            }
            if (text_[position_] == '}') {
                ++position_;
                return true;
            }
            return fail("expected ',' or '}'");
        }
    }

    bool parseArray(JsonValue& out, int depth) {
        if (text_[position_] != '[') return fail("expected an array");
        ++position_;
        out = JsonValue::makeArray();
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == ']') {
            ++position_;
            return true;
        }
        while (true) {
            JsonValue value;
            if (!parseValue(value, depth + 1)) return false;
            out.items.push_back(std::move(value));
            skipWhitespace();
            if (position_ >= text_.size()) return fail("unterminated array");
            if (text_[position_] == ',') {
                ++position_;
                continue;
            }
            if (text_[position_] == ']') {
                ++position_;
                return true;
            }
            return fail("expected ',' or ']'");
        }
    }

    bool parseHex4(unsigned& value) {
        if (position_ + 4 > text_.size()) return fail("truncated escape");
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[position_++];
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value |= static_cast<unsigned>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value |= static_cast<unsigned>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                value |= static_cast<unsigned>(c - 'A' + 10);
            } else {
                return fail("invalid hex escape");
            }
        }
        return true;
    }

    static void appendUtf8(std::string& out, unsigned codePoint) {
        if (codePoint < 0x80) {
            out += static_cast<char>(codePoint);
        } else if (codePoint < 0x800) {
            out += static_cast<char>(0xC0 | (codePoint >> 6));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else if (codePoint < 0x10000) {
            out += static_cast<char>(0xE0 | (codePoint >> 12));
            out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (codePoint >> 18));
            out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
    }

    bool parseString(std::string& out) {
        if (position_ >= text_.size() || text_[position_] != '"') return fail("expected a string");
        ++position_;
        out.clear();
        while (true) {
            if (position_ >= text_.size()) return fail("unterminated string");
            const char c = text_[position_++];
            if (c == '"') return true;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (position_ >= text_.size()) return fail("unterminated escape");
            const char escape = text_[position_++];
            switch (escape) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned codePoint = 0;
                    if (!parseHex4(codePoint)) return false;
                    // Combine a surrogate pair into one code point.
                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF &&
                        position_ + 6 <= text_.size() && text_[position_] == '\\' &&
                        text_[position_ + 1] == 'u') {
                        position_ += 2;
                        unsigned low = 0;
                        if (!parseHex4(low)) return false;
                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                        } else {
                            appendUtf8(out, codePoint);
                            codePoint = low;
                        }
                    }
                    appendUtf8(out, codePoint);
                    break;
                }
                default: return fail("invalid escape");
            }
        }
    }

    bool parseNumber(JsonValue& out) {
        const std::size_t start = position_;
        if (position_ < text_.size() && text_[position_] == '-') ++position_;
        bool digits = false;
        while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
            ++position_;
            digits = true;
        }
        bool integral = true;
        if (position_ < text_.size() && text_[position_] == '.') {
            integral = false;
            ++position_;
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
                digits = true;
            }
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            integral = false;
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) {
                ++position_;
            }
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        if (!digits) return fail("invalid number");

        const std::string literal = text_.substr(start, position_ - start);
        out = JsonValue();
        out.type = JsonValue::Type::Number;
        out.isInteger = integral;
        if (integral) {
            errno = 0;
            char* end = nullptr;
            const long long value = std::strtoll(literal.c_str(), &end, 10);
            if (errno == ERANGE) return fail("integer out of range");
            out.integer = static_cast<std::int64_t>(value);
            out.number = static_cast<double>(value);
        } else {
            out.number = std::strtod(literal.c_str(), nullptr);
        }
        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;
    std::size_t errorPosition_ = 0;
};

// ---------------------------------------------------------------------------
// Tree to project and back
// ---------------------------------------------------------------------------

const JsonValue* member(const JsonValue& object, const char* key) {
    if (object.type != JsonValue::Type::Object) return nullptr;
    for (const auto& entry : object.members) {
        if (entry.first == key) return &entry.second;
    }
    return nullptr;
}

bool readInt(const JsonValue& object, const char* key, std::int64_t& value, std::string& error) {
    const auto* node = member(object, key);
    if (node == nullptr) return true;
    if (node->type != JsonValue::Type::Number || !node->isInteger) {
        error = std::string("expected an integer for '") + key + "'";
        return false;
    }
    value = node->integer;
    return true;
}

bool readString(const JsonValue& object, const char* key, std::string& value, std::string& error) {
    const auto* node = member(object, key);
    if (node == nullptr) return true;
    if (node->type != JsonValue::Type::String) {
        error = std::string("expected a string for '") + key + "'";
        return false;
    }
    value = node->text;
    return true;
}

bool readBool(const JsonValue& object, const char* key, bool& value, std::string& error) {
    const auto* node = member(object, key);
    if (node == nullptr) return true;
    if (node->type != JsonValue::Type::Bool) {
        error = std::string("expected a boolean for '") + key + "'";
        return false;
    }
    value = node->boolean;
    return true;
}

bool readTime(const JsonValue& object, const char* key, Time& time, std::string& error) {
    std::int64_t nanoseconds = time.ns;
    if (!readInt(object, key, nanoseconds, error)) return false;
    time = Time(nanoseconds);
    return true;
}

bool readFrameRate(const JsonValue& object, FrameRate& rate, std::string& error) {
    const auto* node = member(object, "frameRate");
    if (node == nullptr) return true;
    if (node->type != JsonValue::Type::Object) {
        error = "expected an object for 'frameRate'";
        return false;
    }
    std::int64_t numerator = rate.numerator;
    std::int64_t denominator = rate.denominator;
    if (!readInt(*node, "numerator", numerator, error)) return false;
    if (!readInt(*node, "denominator", denominator, error)) return false;
    if (numerator <= 0 || numerator > 1000000 || denominator <= 0 || denominator > 1000000) {
        error = "frameRate is out of range";
        return false;
    }
    rate = FrameRate(static_cast<std::int32_t>(numerator), static_cast<std::int32_t>(denominator));
    return true;
}

bool readClip(const JsonValue& node, Clip& clip, std::string& error) {
    std::int64_t id = static_cast<std::int64_t>(clip.id);
    std::int64_t media = static_cast<std::int64_t>(clip.media);
    std::int64_t speedNumerator = clip.speedNumerator;
    std::int64_t speedDenominator = clip.speedDenominator;

    if (!readInt(node, "id", id, error)) return false;
    if (!readInt(node, "media", media, error)) return false;
    if (!readTime(node, "startNs", clip.start, error)) return false;
    if (!readTime(node, "sourceInNs", clip.sourceIn, error)) return false;
    if (!readTime(node, "sourceOutNs", clip.sourceOut, error)) return false;
    if (!readInt(node, "speedNumerator", speedNumerator, error)) return false;
    if (!readInt(node, "speedDenominator", speedDenominator, error)) return false;
    if (!readString(node, "name", clip.name, error)) return false;
    if (!readBool(node, "enabled", clip.enabled, error)) return false;

    if (id <= 0) {
        error = "clip id must be positive";
        return false;
    }
    clip.id = static_cast<ClipId>(id);
    clip.media = static_cast<MediaId>(media);
    clip.speedNumerator = static_cast<std::int32_t>(speedNumerator);
    clip.speedDenominator = static_cast<std::int32_t>(speedDenominator);
    return true;
}

bool readTrack(const JsonValue& node, Track& track, std::string& error) {
    std::int64_t id = static_cast<std::int64_t>(track.id);
    if (!readInt(node, "id", id, error)) return false;

    std::string kind = track.kind == TrackKind::Video ? "video" : "audio";
    if (!readString(node, "kind", kind, error)) return false;
    if (kind != "video" && kind != "audio") {
        error = "track kind must be 'video' or 'audio'";
        return false;
    }
    track.kind = kind == "video" ? TrackKind::Video : TrackKind::Audio;

    if (!readString(node, "name", track.name, error)) return false;
    if (!readBool(node, "muted", track.muted, error)) return false;
    if (!readBool(node, "hidden", track.hidden, error)) return false;
    if (!readBool(node, "locked", track.locked, error)) return false;

    if (id <= 0) {
        error = "track id must be positive";
        return false;
    }
    track.id = static_cast<TrackId>(id);

    const auto* clips = member(node, "clips");
    if (clips != nullptr) {
        if (clips->type != JsonValue::Type::Array) {
            error = "expected an array for 'clips'";
            return false;
        }
        track.clips.clear();
        track.clips.reserve(clips->items.size());
        for (const auto& item : clips->items) {
            if (item.type != JsonValue::Type::Object) {
                error = "expected an object for a clip";
                return false;
            }
            Clip clip;
            if (!readClip(item, clip, error)) return false;
            track.clips.push_back(std::move(clip));
        }
    }
    return true;
}

bool readSequence(const JsonValue& node, Sequence& sequence, std::string& error) {
    std::int64_t id = static_cast<std::int64_t>(sequence.id);
    std::int64_t width = sequence.width;
    std::int64_t height = sequence.height;

    if (!readInt(node, "id", id, error)) return false;
    if (!readString(node, "name", sequence.name, error)) return false;
    if (!readInt(node, "width", width, error)) return false;
    if (!readInt(node, "height", height, error)) return false;
    if (!readFrameRate(node, sequence.frameRate, error)) return false;

    if (id <= 0) {
        error = "sequence id must be positive";
        return false;
    }
    if (width <= 0 || height <= 0) {
        error = "sequence dimensions must be positive";
        return false;
    }
    sequence.id = static_cast<SequenceId>(id);
    sequence.width = static_cast<std::int32_t>(width);
    sequence.height = static_cast<std::int32_t>(height);

    const auto* tracks = member(node, "tracks");
    if (tracks != nullptr) {
        if (tracks->type != JsonValue::Type::Array) {
            error = "expected an array for 'tracks'";
            return false;
        }
        sequence.tracks.clear();
        sequence.tracks.reserve(tracks->items.size());
        for (const auto& item : tracks->items) {
            if (item.type != JsonValue::Type::Object) {
                error = "expected an object for a track";
                return false;
            }
            Track track;
            if (!readTrack(item, track, error)) return false;
            sequence.tracks.push_back(std::move(track));
        }
    }
    return true;
}

bool readMedia(const JsonValue& node, MediaItem& media, std::string& error) {
    std::int64_t id = static_cast<std::int64_t>(media.id);
    std::int64_t width = media.width;
    std::int64_t height = media.height;

    if (!readInt(node, "id", id, error)) return false;
    if (!readString(node, "path", media.path, error)) return false;
    if (!readString(node, "name", media.displayName, error)) return false;
    if (!readTime(node, "durationNs", media.duration, error)) return false;
    if (!readInt(node, "width", width, error)) return false;
    if (!readInt(node, "height", height, error)) return false;
    if (!readFrameRate(node, media.frameRate, error)) return false;
    if (!readString(node, "hash", media.contentHash, error)) return false;

    if (id <= 0) {
        error = "media id must be positive";
        return false;
    }
    media.id = static_cast<MediaId>(id);
    media.width = static_cast<std::int32_t>(width);
    media.height = static_cast<std::int32_t>(height);
    return true;
}

JsonValue frameRateJson(const FrameRate& rate) {
    JsonValue node = JsonValue::makeObject();
    node.set("numerator", JsonValue::makeInt(rate.numerator));
    node.set("denominator", JsonValue::makeInt(rate.denominator));
    return node;
}

JsonValue clipJson(const Clip& clip) {
    JsonValue node = JsonValue::makeObject();
    node.set("id", JsonValue::makeInt(static_cast<std::int64_t>(clip.id)));
    node.set("media", JsonValue::makeInt(static_cast<std::int64_t>(clip.media)));
    node.set("startNs", JsonValue::makeInt(clip.start.ns));
    node.set("sourceInNs", JsonValue::makeInt(clip.sourceIn.ns));
    node.set("sourceOutNs", JsonValue::makeInt(clip.sourceOut.ns));
    node.set("speedNumerator", JsonValue::makeInt(clip.speedNumerator));
    node.set("speedDenominator", JsonValue::makeInt(clip.speedDenominator));
    node.set("enabled", JsonValue::makeBool(clip.enabled));
    node.set("name", JsonValue::makeString(clip.name));
    return node;
}

JsonValue trackJson(const Track& track) {
    JsonValue node = JsonValue::makeObject();
    node.set("id", JsonValue::makeInt(static_cast<std::int64_t>(track.id)));
    node.set("kind", JsonValue::makeString(track.kind == TrackKind::Video ? "video" : "audio"));
    node.set("name", JsonValue::makeString(track.name));
    node.set("muted", JsonValue::makeBool(track.muted));
    node.set("hidden", JsonValue::makeBool(track.hidden));
    node.set("locked", JsonValue::makeBool(track.locked));

    JsonValue clips = JsonValue::makeArray();
    for (const auto& clip : track.clips) clips.items.push_back(clipJson(clip));
    node.set("clips", std::move(clips));
    return node;
}

JsonValue sequenceJson(const Sequence& sequence) {
    JsonValue node = JsonValue::makeObject();
    node.set("id", JsonValue::makeInt(static_cast<std::int64_t>(sequence.id)));
    node.set("name", JsonValue::makeString(sequence.name));
    node.set("width", JsonValue::makeInt(sequence.width));
    node.set("height", JsonValue::makeInt(sequence.height));
    node.set("frameRate", frameRateJson(sequence.frameRate));

    JsonValue tracks = JsonValue::makeArray();
    for (const auto& track : sequence.tracks) tracks.items.push_back(trackJson(track));
    node.set("tracks", std::move(tracks));
    return node;
}

JsonValue mediaJson(const MediaItem& media) {
    JsonValue node = JsonValue::makeObject();
    node.set("id", JsonValue::makeInt(static_cast<std::int64_t>(media.id)));
    node.set("path", JsonValue::makeString(media.path));
    node.set("name", JsonValue::makeString(media.displayName));
    node.set("durationNs", JsonValue::makeInt(media.duration.ns));
    node.set("width", JsonValue::makeInt(media.width));
    node.set("height", JsonValue::makeInt(media.height));
    node.set("frameRate", frameRateJson(media.frameRate));
    node.set("hash", JsonValue::makeString(media.contentHash));
    return node;
}

}  // namespace

std::string toJson(const Project& project) {
    JsonValue root = JsonValue::makeObject();
    root.set("format", JsonValue::makeString(kProjectFormatName));
    root.set("version", JsonValue::makeInt(kProjectFormatVersion));
    root.set("name", JsonValue::makeString(project.name));
    root.set("nextId", JsonValue::makeInt(static_cast<std::int64_t>(project.nextId)));

    JsonValue media = JsonValue::makeArray();
    for (const auto& item : project.media) media.items.push_back(mediaJson(item));
    root.set("media", std::move(media));

    JsonValue sequences = JsonValue::makeArray();
    for (const auto& sequence : project.sequences) sequences.items.push_back(sequenceJson(sequence));
    root.set("sequences", std::move(sequences));

    std::string out;
    out.reserve(1024);
    dump(root, out);
    out += '\n';
    return out;
}

ParseResult parseProjectJson(const std::string& json) {
    ParseResult result;

    JsonValue root;
    if (!Parser(json).parseDocument(root, result.error)) return result;

    if (root.type != JsonValue::Type::Object) {
        result.error = "the project manifest must be a JSON object";
        return result;
    }

    const auto* format = member(root, "format");
    if (format == nullptr || format->type != JsonValue::Type::String ||
        format->text != kProjectFormatName) {
        result.error = "this is not a video editor project file";
        return result;
    }

    std::int64_t version = 0;
    if (!readInt(root, "version", version, result.error)) return result;
    if (version <= 0) {
        result.error = "the project file has no valid version";
        return result;
    }
    if (version > kProjectFormatVersion) {
        result.error = "this project was saved by a newer version of the app";
        return result;
    }

    Project project;
    if (!readString(root, "name", project.name, result.error)) return result;

    std::int64_t nextId = 1;
    if (!readInt(root, "nextId", nextId, result.error)) return result;

    const auto* media = member(root, "media");
    if (media != nullptr) {
        if (media->type != JsonValue::Type::Array) {
            result.error = "expected an array for 'media'";
            return result;
        }
        for (const auto& item : media->items) {
            if (item.type != JsonValue::Type::Object) {
                result.error = "expected an object for a media item";
                return result;
            }
            MediaItem entry;
            if (!readMedia(item, entry, result.error)) return result;
            project.media.push_back(std::move(entry));
        }
    }

    const auto* sequences = member(root, "sequences");
    if (sequences != nullptr) {
        if (sequences->type != JsonValue::Type::Array) {
            result.error = "expected an array for 'sequences'";
            return result;
        }
        for (const auto& item : sequences->items) {
            if (item.type != JsonValue::Type::Object) {
                result.error = "expected an object for a sequence";
                return result;
            }
            Sequence sequence;
            if (!readSequence(item, sequence, result.error)) return result;
            project.sequences.push_back(std::move(sequence));
        }
    }

    if (project.sequences.empty()) {
        result.error = "the project has no sequences";
        return result;
    }

    // Never trust the saved counter: a hand-edited file could hand out an id that
    // is already in use.
    std::uint64_t highest = 0;
    for (const auto& sequence : project.sequences) {
        if (sequence.id > highest) highest = sequence.id;
        for (const auto& track : sequence.tracks) {
            if (track.id > highest) highest = track.id;
            for (const auto& clip : track.clips) {
                if (clip.id > highest) highest = clip.id;
            }
        }
    }
    for (const auto& item : project.media) {
        if (item.id > highest) highest = item.id;
    }
    const auto minimum = static_cast<std::int64_t>(highest) + 1;
    if (nextId < minimum) nextId = minimum;
    project.nextId = static_cast<std::uint64_t>(nextId);

    const auto validation = validate(project);
    if (!validation.ok()) {
        result.error = std::string("the project is not valid: ") + toString(validation.error);
        return result;
    }

    result.ok = true;
    result.project = std::move(project);
    return result;
}

bool saveProjectToFile(const Project& project, const std::string& path, std::string* error) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error != nullptr) *error = "could not open " + path + " for writing";
        return false;
    }
    const std::string json = toJson(project);
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!file) {
        if (error != nullptr) *error = "could not write " + path;
        return false;
    }
    return true;
}

bool loadProjectFromFile(const std::string& path, Project& project, std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (error != nullptr) *error = "could not open " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const auto result = parseProjectJson(buffer.str());
    if (!result.ok) {
        if (error != nullptr) *error = result.error;
        return false;
    }
    project = result.project;
    return true;
}

}  // namespace vx
