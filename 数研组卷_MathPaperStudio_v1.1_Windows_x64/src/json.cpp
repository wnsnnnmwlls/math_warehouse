#include "json.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <utility>

namespace jsonx {
namespace {

const Value kNullValue;

void AppendUtf8(std::string& out, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

void EscapeString(std::string& out, const std::string& text) {
    out.push_back('"');
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[8] = {};
                    std::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast<unsigned>(c));
                    out += buffer;
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    out.push_back('"');
}

void NumberToString(double value, std::string& out) {
    if (!std::isfinite(value)) {
        out += "0";
        return;
    }
    char buffer[64] = {};
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc()) {
        out += "0";
        return;
    }
    out.append(buffer, result.ptr);
}

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool Parse(Value& out) {
        if (text_.size() >= 3 && static_cast<unsigned char>(text_[0]) == 0xEF &&
            static_cast<unsigned char>(text_[1]) == 0xBB &&
            static_cast<unsigned char>(text_[2]) == 0xBF) {
            pos_ = 3;
        }
        SkipWhitespace();
        if (!ParseValue(out, 0)) {
            return false;
        }
        SkipWhitespace();
        if (pos_ != text_.size()) {
            return Fail("顶层 JSON 后还有多余字符");
        }
        return true;
    }

    const std::string& error() const { return error_; }

private:
    const std::string& text_;
    std::size_t pos_ = 0;
    std::string error_;

    bool Fail(const std::string& message) {
        if (error_.empty()) {
            error_ = "JSON 解析失败（位置 " + std::to_string(pos_) + "）：" + message;
        }
        return false;
    }

    void SkipWhitespace() {
        while (pos_ < text_.size()) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    bool Consume(char expected) {
        if (pos_ < text_.size() && text_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool ParseValue(Value& out, int depth) {
        if (depth > 96) {
            return Fail("嵌套层级过深");
        }
        SkipWhitespace();
        if (pos_ >= text_.size()) {
            return Fail("缺少值");
        }
        char c = text_[pos_];
        if (c == '{') {
            return ParseObject(out, depth);
        }
        if (c == '[') {
            return ParseArray(out, depth);
        }
        if (c == '"') {
            std::string value;
            if (!ParseString(value)) {
                return false;
            }
            out = Value(std::move(value));
            return true;
        }
        if (c == 't') {
            if (text_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                out = Value(true);
                return true;
            }
            return Fail("无效的字面量");
        }
        if (c == 'f') {
            if (text_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                out = Value(false);
                return true;
            }
            return Fail("无效的字面量");
        }
        if (c == 'n') {
            if (text_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                out = Value();
                return true;
            }
            return Fail("无效的字面量");
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return ParseNumber(out);
        }
        return Fail("无法识别的字符");
    }

    bool ParseObject(Value& out, int depth) {
        ++pos_;  // {
        Value::Object object;
        SkipWhitespace();
        if (Consume('}')) {
            out = Value(std::move(object));
            return true;
        }
        while (true) {
            SkipWhitespace();
            std::string key;
            if (!ParseString(key)) {
                return false;
            }
            SkipWhitespace();
            if (!Consume(':')) {
                return Fail("对象成员缺少冒号");
            }
            Value value;
            if (!ParseValue(value, depth + 1)) {
                return false;
            }
            object[std::move(key)] = std::move(value);
            SkipWhitespace();
            if (Consume('}')) {
                break;
            }
            if (!Consume(',')) {
                return Fail("对象成员之间缺少逗号");
            }
        }
        out = Value(std::move(object));
        return true;
    }

    bool ParseArray(Value& out, int depth) {
        ++pos_;  // [
        Value::Array array;
        SkipWhitespace();
        if (Consume(']')) {
            out = Value(std::move(array));
            return true;
        }
        while (true) {
            Value value;
            if (!ParseValue(value, depth + 1)) {
                return false;
            }
            array.push_back(std::move(value));
            SkipWhitespace();
            if (Consume(']')) {
                break;
            }
            if (!Consume(',')) {
                return Fail("数组元素之间缺少逗号");
            }
        }
        out = Value(std::move(array));
        return true;
    }

    bool ParseHex4(uint32_t& value) {
        if (pos_ + 4 > text_.size()) {
            return Fail("Unicode 转义不完整");
        }
        value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text_[pos_++];
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value |= static_cast<uint32_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value |= static_cast<uint32_t>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                value |= static_cast<uint32_t>(c - 'A' + 10);
            } else {
                return Fail("Unicode 转义包含非法字符");
            }
        }
        return true;
    }

    bool ParseString(std::string& out) {
        if (!Consume('"')) {
            return Fail("缺少字符串引号");
        }
        out.clear();
        while (pos_ < text_.size()) {
            unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (pos_ >= text_.size()) {
                    return Fail("字符串转义不完整");
                }
                char escape = text_[pos_++];
                switch (escape) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        uint32_t codepoint = 0;
                        if (!ParseHex4(codepoint)) {
                            return false;
                        }
                        if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                            if (pos_ + 2 > text_.size() || text_[pos_] != '\\' ||
                                text_[pos_ + 1] != 'u') {
                                return Fail("高位代理项后缺少低位代理项");
                            }
                            pos_ += 2;
                            uint32_t low = 0;
                            if (!ParseHex4(low)) {
                                return false;
                            }
                            if (low < 0xDC00 || low > 0xDFFF) {
                                return Fail("低位代理项无效");
                            }
                            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                        } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                            return Fail("孤立的低位代理项");
                        }
                        AppendUtf8(out, codepoint);
                        break;
                    }
                    default:
                        return Fail("未知的字符串转义");
                }
            } else if (c < 0x20) {
                return Fail("字符串中存在控制字符");
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
        return Fail("字符串没有结束引号");
    }

    bool ParseNumber(Value& out) {
        std::size_t start = pos_;
        if (text_[pos_] == '-') {
            ++pos_;
        }
        if (pos_ >= text_.size()) {
            return Fail("数字不完整");
        }
        if (text_[pos_] == '0') {
            ++pos_;
            if (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                return Fail("数字不能有前导零");
            }
        } else if (text_[pos_] >= '1' && text_[pos_] <= '9') {
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                ++pos_;
            }
        } else {
            return Fail("数字格式无效");
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            if (pos_ >= text_.size() || text_[pos_] < '0' || text_[pos_] > '9') {
                return Fail("小数部分缺少数位");
            }
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                ++pos_;
            }
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) {
                ++pos_;
            }
            if (pos_ >= text_.size() || text_[pos_] < '0' || text_[pos_] > '9') {
                return Fail("科学计数法指数无效");
            }
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                ++pos_;
            }
        }
        double number = 0.0;
        auto result = std::from_chars(text_.data() + start, text_.data() + pos_, number);
        if (result.ec != std::errc() || result.ptr != text_.data() + pos_) {
            return Fail("数字格式无效");
        }
        out = Value(number);
        return true;
    }
};

}  // namespace

Value::Value() = default;

Value::Value(bool value) : kind_(Kind::Bool), bool_(value) {}

Value::Value(double value) : kind_(Kind::Number), number_(value) {}

Value::Value(int value) : kind_(Kind::Number), number_(static_cast<double>(value)) {}

Value::Value(const char* value) : kind_(Kind::String), string_(value ? value : "") {}

Value::Value(std::string value) : kind_(Kind::String), string_(std::move(value)) {}

Value::Value(Array value) : kind_(Kind::Array), array_(std::move(value)) {}

Value::Value(Object value) : kind_(Kind::Object), object_(std::move(value)) {}

Value Value::MakeArray() {
    return Value(Array{});
}

Value Value::MakeObject() {
    return Value(Object{});
}

Value Value::Parse(const std::string& text, std::string* error) {
    Parser parser(text);
    Value result;
    if (!parser.Parse(result)) {
        if (error) {
            *error = parser.error();
        }
        return Value();
    }
    if (error) {
        error->clear();
    }
    return result;
}

bool Value::AsBool(bool fallback) const {
    return kind_ == Kind::Bool ? bool_ : fallback;
}

double Value::AsNumber(double fallback) const {
    return kind_ == Kind::Number ? number_ : fallback;
}

int Value::AsInt(int fallback) const {
    return kind_ == Kind::Number ? static_cast<int>(number_) : fallback;
}

const std::string& Value::AsString() const {
    static const std::string empty;
    return kind_ == Kind::String ? string_ : empty;
}

std::string Value::AsString(const std::string& fallback) const {
    return kind_ == Kind::String ? string_ : fallback;
}

std::size_t Value::size() const {
    if (kind_ == Kind::Array) {
        return array_.size();
    }
    if (kind_ == Kind::Object) {
        return object_.size();
    }
    return 0;
}

bool Value::empty() const {
    return size() == 0;
}

const Value& Value::operator[](std::size_t index) const {
    if (kind_ == Kind::Array && index < array_.size()) {
        return array_[index];
    }
    return kNullValue;
}

const Value& Value::operator[](const std::string& key) const {
    if (kind_ == Kind::Object) {
        auto it = object_.find(key);
        if (it != object_.end()) {
            return it->second;
        }
    }
    return kNullValue;
}

Value& Value::operator[](const std::string& key) {
    if (kind_ != Kind::Object) {
        kind_ = Kind::Object;
        object_.clear();
    }
    return object_[key];
}

const Value* Value::Find(const std::string& key) const {
    if (kind_ != Kind::Object) {
        return nullptr;
    }
    auto it = object_.find(key);
    return it == object_.end() ? nullptr : &it->second;
}

void Value::Push(Value value) {
    if (kind_ != Kind::Array) {
        kind_ = Kind::Array;
        array_.clear();
    }
    array_.push_back(std::move(value));
}

void Value::Set(std::string key, Value value) {
    if (kind_ != Kind::Object) {
        kind_ = Kind::Object;
        object_.clear();
    }
    object_[std::move(key)] = std::move(value);
}

std::string Value::Dump(int indent) const {
    std::string out;
    DumpTo(out, indent, 0);
    return out;
}

std::string Value::DumpCompact() const {
    std::string out;
    DumpTo(out, 0, 0);
    return out;
}

void Value::DumpTo(std::string& out, int indent, int depth) const {
    switch (kind_) {
        case Kind::Null:
            out += "null";
            break;
        case Kind::Bool:
            out += bool_ ? "true" : "false";
            break;
        case Kind::Number:
            NumberToString(number_, out);
            break;
        case Kind::String:
            EscapeString(out, string_);
            break;
        case Kind::Array: {
            out.push_back('[');
            if (!array_.empty()) {
                bool pretty = indent > 0;
                if (pretty) out.push_back('\n');
                for (std::size_t i = 0; i < array_.size(); ++i) {
                    if (pretty) out.append(static_cast<std::size_t>(indent * (depth + 1)), ' ');
                    array_[i].DumpTo(out, indent, depth + 1);
                    if (i + 1 < array_.size()) out.push_back(',');
                    if (pretty) out.push_back('\n');
                }
                if (pretty) out.append(static_cast<std::size_t>(indent * depth), ' ');
            }
            out.push_back(']');
            break;
        }
        case Kind::Object: {
            out.push_back('{');
            if (!object_.empty()) {
                bool pretty = indent > 0;
                if (pretty) out.push_back('\n');
                std::size_t index = 0;
                for (const auto& member : object_) {
                    if (pretty) out.append(static_cast<std::size_t>(indent * (depth + 1)), ' ');
                    EscapeString(out, member.first);
                    out += pretty ? ": " : ":";
                    member.second.DumpTo(out, indent, depth + 1);
                    if (++index < object_.size()) out.push_back(',');
                    if (pretty) out.push_back('\n');
                }
                if (pretty) out.append(static_cast<std::size_t>(indent * depth), ' ');
            }
            out.push_back('}');
            break;
        }
    }
}

}  // namespace jsonx
