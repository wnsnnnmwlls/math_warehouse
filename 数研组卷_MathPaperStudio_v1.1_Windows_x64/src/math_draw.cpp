#include "math_draw.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <vector>

namespace mathdraw {
namespace {

struct Part {
    bool fraction = false;
    std::wstring text;
    std::wstring numerator;
    std::wstring denominator;
};

bool IsSimpleMathChar(wchar_t ch) {
    if (std::iswalnum(ch)) return true;
    switch (ch) {
        case L'^': case L'.': case L'\'': case L'°': case L'π': case L'α':
        case L'β': case L'γ': case L'θ': case L'φ': case L'ω': case L'∞':
        case L'√': case L'²': case L'³': case L'⁴': case L'₁': case L'₂':
        case L'₃': case L'₄': case L'%':
            return true;
        default:
            return false;
    }
}

bool IsOpening(wchar_t ch) { return ch == L'(' || ch == L'（' || ch == L'['; }
bool IsClosing(wchar_t ch) { return ch == L')' || ch == L'）' || ch == L']'; }

size_t MatchOpenParen(const std::wstring& text, size_t closeIndex) {
    int depth = 0;
    for (size_t i = closeIndex + 1; i-- > 0;) {
        if (IsClosing(text[i])) {
            ++depth;
        } else if (IsOpening(text[i])) {
            --depth;
            if (depth == 0) return i;
        }
        if (i == 0) break;
    }
    return std::wstring::npos;
}

size_t MatchCloseParen(const std::wstring& text, size_t openIndex) {
    int depth = 0;
    for (size_t i = openIndex; i < text.size(); ++i) {
        if (IsOpening(text[i])) {
            ++depth;
        } else if (IsClosing(text[i])) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::wstring::npos;
}

bool FindFractionAt(const std::wstring& text, size_t slash, size_t& left, size_t& right) {
    if (slash == 0 || slash + 1 >= text.size()) return false;
    left = slash;
    if (IsClosing(text[slash - 1])) {
        left = MatchOpenParen(text, slash - 1);
        if (left == std::wstring::npos) return false;
    } else {
        size_t i = slash;
        while (i > 0 && IsSimpleMathChar(text[i - 1])) --i;
        left = i;
        if (left == slash) return false;
    }

    right = slash + 1;
    if (IsOpening(text[slash + 1])) {
        size_t close = MatchCloseParen(text, slash + 1);
        if (close == std::wstring::npos) return false;
        right = close + 1;
    } else {
        size_t i = slash + 1;
        while (i < text.size() && IsSimpleMathChar(text[i])) ++i;
        right = i;
        if (right == slash + 1) return false;
    }
    if (right > text.size()) return false;
    return left < slash && slash + 1 < right;
}

std::vector<Part> Parse(const std::wstring& text, bool& hasFraction) {
    std::vector<Part> parts;
    hasFraction = false;
    std::wstring plain;
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] != L'/') {
            plain.push_back(text[i++]);
            continue;
        }
        size_t left = 0;
        size_t right = 0;
        if (!FindFractionAt(text, i, left, right)) {
            plain.push_back(text[i++]);
            continue;
        }
        // Do not treat a URL or a path-like token as a fraction.
        size_t previous = left;
        if (previous >= 2 && text[previous - 1] == L'/' && text[previous - 2] == L'/') {
            plain.push_back(text[i++]);
            continue;
        }
        if (!plain.empty()) {
            parts.push_back(Part{false, plain, {}, {}});
            plain.clear();
        }
        Part part;
        part.fraction = true;
        part.numerator = text.substr(left, i - left);
        part.denominator = text.substr(i + 1, right - i - 1);
        parts.push_back(std::move(part));
        hasFraction = true;
        i = right;
    }
    if (!plain.empty()) parts.push_back(Part{false, plain, {}, {}});
    return parts;
}

}  // namespace

bool DrawLine(ui::Canvas& canvas, const std::wstring& text, const ui::Rect& rect,
              float pixels, const Gdiplus::Color& color, bool bold,
              Gdiplus::StringAlignment align) {
    bool hasFraction = false;
    const std::vector<Part> parts = Parse(text, hasFraction);
    if (!hasFraction) return false;

    const float fractionPixels = std::max(8.0f, pixels * 0.70f);
    const float fractionLine = fractionPixels * 1.10f;
    const float fractionGap = 2.0f;
    const float fractionHeight = fractionLine * 2.0f + fractionGap;

    float totalWidth = 0.0f;
    for (const auto& part : parts) {
        if (part.fraction) {
            const float num = canvas.Measure(part.numerator, fractionPixels, bold).Width;
            const float den = canvas.Measure(part.denominator, fractionPixels, bold).Width;
            totalWidth += std::max(num, den) + 6.0f;
        } else {
            totalWidth += canvas.Measure(part.text, pixels, bold).Width;
        }
    }
    if (totalWidth > rect.w + 0.5f) return false;

    float x = rect.x;
    if (align == Gdiplus::StringAlignmentCenter) {
        x = rect.x + (rect.w - totalWidth) * 0.5f;
    } else if (align == Gdiplus::StringAlignmentFar) {
        x = rect.right() - totalWidth;
    }
    const float centerY = rect.y + rect.h * 0.5f;
    const float textHeight = pixels * 1.45f;
    const float fracTop = centerY - fractionHeight * 0.5f;

    for (const auto& part : parts) {
        if (!part.fraction) {
            const float partWidth = canvas.Measure(part.text, pixels, bold).Width;
            canvas.Text(part.text, ui::Rect{x, centerY - textHeight * 0.5f, partWidth,
                                            textHeight},
                        pixels, color, bold, Gdiplus::StringAlignmentNear,
                        Gdiplus::StringAlignmentCenter);
            x += partWidth;
            continue;
        }
        const float numWidth = canvas.Measure(part.numerator, fractionPixels, bold).Width;
        const float denWidth = canvas.Measure(part.denominator, fractionPixels, bold).Width;
        const float partWidth = std::max(numWidth, denWidth) + 6.0f;
        canvas.Text(part.numerator,
                    ui::Rect{x, fracTop, partWidth, fractionLine}, fractionPixels, color, bold,
                    Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
        canvas.Line(x + 2.0f, fracTop + fractionLine + fractionGap * 0.5f,
                    x + partWidth - 2.0f, fracTop + fractionLine + fractionGap * 0.5f,
                    color, 0.9f);
        canvas.Text(part.denominator,
                    ui::Rect{x, fracTop + fractionLine + fractionGap, partWidth, fractionLine},
                    fractionPixels, color, bold, Gdiplus::StringAlignmentCenter,
                    Gdiplus::StringAlignmentCenter);
        x += partWidth;
    }
    return true;
}

}  // namespace mathdraw
