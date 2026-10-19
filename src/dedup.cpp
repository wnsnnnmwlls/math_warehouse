#include "dedup.h"

#include "math_text.h"
#include "util.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

void AppendUtf8(std::string& out, unsigned codepoint) {
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

bool NextCodepoint(const std::string& text, size_t& position, unsigned& codepoint) {
    if (position >= text.size()) return false;
    unsigned char first = static_cast<unsigned char>(text[position]);
    if (first < 0x80) {
        codepoint = first;
        ++position;
        return true;
    }
    int extra = 0;
    if ((first & 0xE0) == 0xC0) {
        codepoint = first & 0x1F;
        extra = 1;
    } else if ((first & 0xF0) == 0xE0) {
        codepoint = first & 0x0F;
        extra = 2;
    } else if ((first & 0xF8) == 0xF0) {
        codepoint = first & 0x07;
        extra = 3;
    } else {
        codepoint = 0xFFFD;
        ++position;
        return true;
    }
    if (position + static_cast<size_t>(extra) >= text.size()) {
        position = text.size();
        codepoint = 0xFFFD;
        return true;
    }
    for (int i = 1; i <= extra; ++i) {
        unsigned char next = static_cast<unsigned char>(text[position + static_cast<size_t>(i)]);
        if ((next & 0xC0) != 0x80) {
            codepoint = 0xFFFD;
            position += static_cast<size_t>(i);
            return true;
        }
        codepoint = (codepoint << 6) | (next & 0x3F);
    }
    position += static_cast<size_t>(extra) + 1;
    return true;
}

bool IsSentencePunctuation(unsigned cp) {
    switch (cp) {
        case ',': case '.': case ';': case ':': case '!': case '?':
        case 0x3001:  // 、
        case 0x3002:  // 。
        case 0xFF0C:  // ，
        case 0xFF0E:  // ．
        case 0xFF1B:  // ；
        case 0xFF1A:  // ：
        case 0xFF01:  // ！
        case 0xFF1F:  // ？
        case 0x2018: case 0x2019: case 0x201C: case 0x201D:
            return true;
        default:
            return false;
    }
}

bool IsIgnoredMathSymbol(unsigned cp) {
    switch (cp) {
        case 0x00D7:  // ×
        case 0x00B7:  // ·
        case 0x22C5:  // ⋅
        case 0x2217:  // ∗
        case '*':
            return true;
        default:
            return false;
    }
}

std::string AsciiLower(std::string text) {
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return text;
}

}  // namespace

std::string CanonicalQuestionKey(const std::string& stem) {
    std::string normalized = NormalizeMathText(stem);
    std::string output;
    output.reserve(normalized.size());
    size_t position = 0;
    while (position < normalized.size()) {
        unsigned cp = 0;
        if (!NextCodepoint(normalized, position, cp)) break;
        if (cp >= 0xFF01 && cp <= 0xFF5E) cp = cp - 0xFF01 + 0x21;
        if (cp == 0x3000) continue;
        if (cp == ' ' || cp == '\t' || cp == '\r' || cp == '\n' || cp == '\f' || cp == '\v') {
            continue;
        }
        if (IsSentencePunctuation(cp)) continue;
        if (IsIgnoredMathSymbol(cp)) continue;
        if (cp == '_') continue;  // blank markers such as ______
        if (cp == 0x5EA6) {       // 度 -> °
            AppendUtf8(output, 0x00B0);
            continue;
        }
        if (cp >= 'A' && cp <= 'Z') cp = cp - 'A' + 'a';
        AppendUtf8(output, cp);
    }
    return util::Trim(AsciiLower(output));
}

std::vector<DuplicateGroup> FindDuplicateGroups(const std::vector<Question>& questions) {
    std::map<std::string, std::vector<size_t>> grouped;
    for (size_t i = 0; i < questions.size(); ++i) {
        std::string key = CanonicalQuestionKey(questions[i].stem);
        if (key.empty()) continue;
        grouped[key].push_back(i);
    }
    std::vector<DuplicateGroup> groups;
    for (auto& [key, indices] : grouped) {
        if (indices.size() >= 2) {
            groups.push_back(DuplicateGroup{key, std::move(indices)});
        }
    }
    std::stable_sort(groups.begin(), groups.end(), [](const DuplicateGroup& a, const DuplicateGroup& b) {
        return a.questionIndices.front() < b.questionIndices.front();
    });
    return groups;
}

std::string InsertImplicitMultiplication(const std::string& text) {
    // Only handle very obvious patterns. The goal is readability, not a full TeX parser.
    static const char* functionNames[] = {"sin", "cos", "tan", "cot", "sec", "csc", "ln", "log",
                                          "sqrt"};
    std::string output = text;
    size_t position = 0;
    while (position < output.size()) {
        bool inserted = false;
        if (output[position] == ')' && position + 1 < output.size() &&
            (output[position + 1] == '(' || std::isalnum(static_cast<unsigned char>(output[position + 1])))) {
            output.insert(position + 1, "·");
            position += 2;
            inserted = true;
        }
        if (!inserted && (output[position] == '\xC2' && position + 1 < output.size() &&
                          static_cast<unsigned char>(output[position + 1]) == 0xB0)) {
            // UTF-8 degree sign: ° followed by a function name or digit.
            size_t next = position + 2;
            if (next < output.size() &&
                (std::isdigit(static_cast<unsigned char>(output[next])) ||
                 std::strncmp(output.c_str() + next, "sin", 3) == 0 ||
                 std::strncmp(output.c_str() + next, "cos", 3) == 0 ||
                 std::strncmp(output.c_str() + next, "tan", 3) == 0)) {
                output.insert(next, "·");
                position = next + 1;
                inserted = true;
            }
        }
        if (!inserted && std::isdigit(static_cast<unsigned char>(output[position]))) {
            size_t next = position + 1;
            while (next < output.size() &&
                   (std::isdigit(static_cast<unsigned char>(output[next])) || output[next] == '.')) {
                ++next;
            }
            for (const char* name : functionNames) {
                if (output.compare(next, std::strlen(name), name) == 0) {
                    output.insert(next, "·");
                    position = next + 1;
                    inserted = true;
                    break;
                }
            }
        }
        if (!inserted) ++position;
    }
    return output;
}
