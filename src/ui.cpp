#include "ui.h"

#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <cwchar>

namespace ui {
namespace {

std::unique_ptr<Gdiplus::FontFamily> TryFamily(const wchar_t* name) {
    auto family = std::make_unique<Gdiplus::FontFamily>(name);
    if (family->GetLastStatus() != Gdiplus::Ok) {
        return nullptr;
    }
    return family;
}

void BuildRoundedPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, float radius) {
    float r = std::min({radius, rect.Width / 2.0f, rect.Height / 2.0f});
    if (r <= 0.2f) {
        path.AddRectangle(rect);
        return;
    }
    const float diameter = r * 2.0f;
    path.AddArc(rect.X, rect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y, diameter, diameter, 270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y + rect.Height - diameter, diameter,
                diameter, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - diameter, diameter, diameter, 90.0f, 90.0f);
    path.CloseFigure();
}

size_t UnitLength(const std::wstring& text, size_t position) {
    if (position + 1 < text.size()) {
        wchar_t c = text[position];
        if (c >= 0xD800 && c <= 0xDBFF) {
            wchar_t d = text[position + 1];
            if (d >= 0xDC00 && d <= 0xDFFF) {
                return 2;
            }
        }
    }
    return 1;
}

std::wstring NormalizeForPreview(const std::wstring& text) {
    std::wstring normalized;
    normalized.reserve(text.size());
    bool lastSpace = false;
    for (wchar_t c : text) {
        if (c == L'\r' || c == L'\n' || c == L'\t') {
            if (!lastSpace) {
                normalized.push_back(L' ');
                lastSpace = true;
            }
        } else if (c == L' ') {
            if (!lastSpace) {
                normalized.push_back(L' ');
                lastSpace = true;
            }
        } else {
            normalized.push_back(c);
            lastSpace = false;
        }
    }
    return normalized;
}

}  // namespace

Theme::Theme() = default;
Theme::~Theme() = default;

bool Theme::Initialize(float scale) {
    scale_ = std::max(0.75f, std::min(scale, 3.0f));
    fonts_.clear();
    family_.reset();
    for (const wchar_t* name : {L"Microsoft YaHei UI", L"Microsoft YaHei", L"SimHei", L"Segoe UI"}) {
        family_ = TryFamily(name);
        if (family_) break;
    }
    if (!family_) {
        const Gdiplus::FontFamily* generic = Gdiplus::FontFamily::GenericSansSerif();
        if (generic) {
            family_.reset(generic->Clone());
        }
    }
    return family_ != nullptr;
}

Gdiplus::Font* Theme::Font(float pixels, bool bold) {
    int sizePx = std::max(8, static_cast<int>(std::lround(pixels * scale_)));
    int key = sizePx * 4 + (bold ? 1 : 0);
    auto it = fonts_.find(key);
    if (it != fonts_.end()) {
        return it->second.get();
    }
    auto font = std::make_unique<Gdiplus::Font>(
        family_.get(), static_cast<Gdiplus::REAL>(sizePx),
        bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font* raw = font.get();
    fonts_[key] = std::move(font);
    return raw;
}

Canvas::Canvas(Gdiplus::Graphics& graphics, Theme& theme) : graphics_(graphics), theme_(theme) {
    graphics_.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics_.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    graphics_.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics_.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
}

Gdiplus::RectF Canvas::Px(const Rect& rect) const {
    float s = scale();
    return Gdiplus::RectF(rect.x * s, rect.y * s, rect.w * s, rect.h * s);
}

void Canvas::FillRect(const Rect& rect, const Gdiplus::Color& color, float radius) {
    if (rect.w <= 0.0f || rect.h <= 0.0f || color.GetA() == 0) return;
    Gdiplus::SolidBrush brush(color);
    Gdiplus::RectF r = Px(rect);
    if (radius <= 0.2f) {
        graphics_.FillRectangle(&brush, r);
    } else {
        Gdiplus::GraphicsPath path;
        BuildRoundedPath(path, r, radius * scale());
        graphics_.FillPath(&brush, &path);
    }
}

void Canvas::StrokeRect(const Rect& rect, const Gdiplus::Color& color, float radius, float width) {
    if (rect.w <= 0.0f || rect.h <= 0.0f || color.GetA() == 0) return;
    Gdiplus::Pen pen(color, width * scale());
    Gdiplus::RectF r = Px(rect);
    if (radius <= 0.2f) {
        graphics_.DrawRectangle(&pen, r);
    } else {
        Gdiplus::GraphicsPath path;
        BuildRoundedPath(path, r, radius * scale());
        graphics_.DrawPath(&pen, &path);
    }
}

void Canvas::DrawShadow(const Rect& rect, float radius) {
    for (int i = 3; i >= 1; --i) {
        Rect shadow = rect.Offset(0.0f, static_cast<float>(i) * 1.4f);
        if (theme_.dark()) {
            FillRect(shadow, Gdiplus::Color(static_cast<BYTE>(14 + i * 4), 0, 0, 0), radius);
        } else {
            FillRect(shadow, Gdiplus::Color(static_cast<BYTE>(5 + i * 2), 23, 29, 45), radius);
        }
    }
}

void Canvas::Line(float x1, float y1, float x2, float y2, const Gdiplus::Color& color,
                  float width) {
    Gdiplus::Pen pen(color, width * scale());
    float s = scale();
    graphics_.DrawLine(&pen, x1 * s, y1 * s, x2 * s, y2 * s);
}

void Canvas::Text(const std::wstring& text, const Rect& rect, float pixels,
                  const Gdiplus::Color& color, bool bold, Gdiplus::StringAlignment align,
                  Gdiplus::StringAlignment lineAlign, bool wrap) {
    if (text.empty() || rect.w <= 0.0f || rect.h <= 0.0f) return;
    Gdiplus::Font* font = theme_.Font(pixels, bold);
    Gdiplus::StringFormat format;
    SetupTextFormat(format, align, lineAlign, wrap);
    Gdiplus::SolidBrush brush(color);
    Gdiplus::RectF r = Px(rect);
    graphics_.DrawString(text.c_str(), static_cast<INT>(text.size()), font, r, &format, &brush);
}

void Canvas::TextEllipsis(const std::wstring& text, const Rect& rect, float pixels,
                          const Gdiplus::Color& color, bool bold,
                          Gdiplus::StringAlignment align) {
    if (text.empty()) return;
    Gdiplus::Font* font = theme_.Font(pixels, bold);
    Gdiplus::StringFormat format;
    SetupTextFormat(format, align, Gdiplus::StringAlignmentCenter, false);
    format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    Gdiplus::SolidBrush brush(color);
    Gdiplus::RectF r = Px(rect);
    graphics_.DrawString(text.c_str(), static_cast<INT>(text.size()), font, r, &format, &brush);
}

void Canvas::TextWrapped(const std::wstring& text, const Rect& rect, float pixels,
                         const Gdiplus::Color& color, bool bold,
                         Gdiplus::StringAlignment align,
                         Gdiplus::StringAlignment lineAlign) {
    if (text.empty()) return;
    Gdiplus::Font* font = theme_.Font(pixels, bold);
    Gdiplus::StringFormat format;
    SetupTextFormat(format, align, lineAlign, true);
    Gdiplus::SolidBrush brush(color);
    Gdiplus::RectF r = Px(rect);
    graphics_.DrawString(text.c_str(), static_cast<INT>(text.size()), font, r, &format, &brush);
}

float Canvas::TextBlock(const std::wstring& text, const Rect& rect, float pixels,
                        const Gdiplus::Color& color, int maxLines, bool bold) {
    if (text.empty() || maxLines <= 0) return 0.0f;
    Gdiplus::Font* font = theme_.Font(pixels, bold);
    std::wstring normalized = NormalizeForPreview(text);
    if (normalized.size() > 1200) {
        normalized.resize(1200);
    }
    const float lineHeight = pixels * 1.48f;
    std::vector<std::wstring> lines;
    size_t position = 0;
    while (position < normalized.size() && static_cast<int>(lines.size()) < maxLines) {
        while (position < normalized.size() && normalized[position] == L' ') ++position;
        if (position >= normalized.size()) break;
        std::wstring line;
        while (position < normalized.size()) {
            if (normalized[position] == L' ') {
                line.push_back(L' ');
                ++position;
                continue;
            }
            size_t unit = UnitLength(normalized, position);
            std::wstring candidate = line + normalized.substr(position, unit);
            Gdiplus::StringFormat format;
            SetupTextFormat(format, Gdiplus::StringAlignmentNear,
                            Gdiplus::StringAlignmentNear, false);
            Gdiplus::RectF layout(0, 0, rect.w * scale() + 2, 10000);
            Gdiplus::RectF bounds;
            graphics_.MeasureString(candidate.c_str(), static_cast<INT>(candidate.size()), font,
                                    layout, &format, &bounds);
            if (!line.empty() && bounds.Width / scale() > rect.w) {
                break;
            }
            line = candidate;
            position += unit;
        }
        if (line.empty() && position < normalized.size()) {
            line = normalized.substr(position, UnitLength(normalized, position));
            position += UnitLength(normalized, position);
        }
        if (position < normalized.size() && static_cast<int>(lines.size()) == maxLines - 1) {
            for (;;) {
                std::wstring withEllipsis = line + L"…";
                Gdiplus::StringFormat format;
                SetupTextFormat(format, Gdiplus::StringAlignmentNear,
                                Gdiplus::StringAlignmentNear, false);
                Gdiplus::RectF layout(0, 0, 10000, 10000);
                Gdiplus::RectF bounds;
                graphics_.MeasureString(withEllipsis.c_str(), static_cast<INT>(withEllipsis.size()),
                                        font, layout, &format, &bounds);
                if (bounds.Width / scale() <= rect.w || line.empty()) {
                    line = std::move(withEllipsis);
                    break;
                }
                line.erase(line.size() - 1);
            }
        }
        lines.push_back(line);
    }
    float y = rect.y;
    for (const auto& line : lines) {
        Rect lineRect{rect.x, y, rect.w, lineHeight};
        Text(line, lineRect, pixels, color, bold, Gdiplus::StringAlignmentNear,
             Gdiplus::StringAlignmentNear, false);
        y += lineHeight;
    }
    return static_cast<float>(lines.size()) * lineHeight;
}

Gdiplus::SizeF Canvas::Measure(const std::wstring& text, float pixels, bool bold, bool wrap,
                               float maxWidth) {
    Gdiplus::Font* font = theme_.Font(pixels, bold);
    Gdiplus::StringFormat format;
    SetupTextFormat(format, Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentNear, wrap);
    Gdiplus::RectF layout(0, 0, wrap && maxWidth > 0 ? maxWidth * scale() : 100000.0f, 100000.0f);
    Gdiplus::RectF bounds;
    graphics_.MeasureString(text.c_str(), static_cast<INT>(text.size()), font, layout, &format,
                            &bounds);
    return Gdiplus::SizeF(bounds.Width / scale(), bounds.Height / scale());
}

void Canvas::Checkbox(const Rect& rect, bool checked, bool hovered, bool disabled) {
    const bool dark = theme_.dark();
    Gdiplus::Color fill = checked ? (dark ? Color(139, 132, 255) : Color(79, 70, 229))
                                  : (dark ? Color(21, 27, 38) : Color(255, 255, 255));
    Gdiplus::Color border = checked ? (dark ? Color(139, 132, 255) : Color(79, 70, 229))
                                    : (dark ? Color(70, 82, 102) : Color(183, 188, 201));
    if (disabled) {
        fill = checked ? (dark ? Color(53, 61, 77) : Color(196, 199, 212))
                       : (dark ? Color(20, 26, 36) : Color(247, 248, 251));
        border = dark ? Color(54, 63, 80) : Color(212, 216, 226);
    } else if (hovered) {
        border = checked ? (dark ? Color(166, 160, 255) : Color(67, 56, 202))
                         : (dark ? Color(112, 121, 145) : Color(109, 99, 240));
        if (!checked) fill = dark ? Color(30, 36, 50) : Color(246, 245, 255);
    }
    FillRect(rect, fill, 5.0f);
    StrokeRect(rect, border, 5.0f, 1.2f);
    if (checked) {
        float s = scale();
        Gdiplus::Pen pen(Color(255, 255, 255), 2.0f * s);
        pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
        Gdiplus::PointF points[3] = {
            Gdiplus::PointF((rect.x + rect.w * 0.23f) * s, (rect.y + rect.h * 0.52f) * s),
            Gdiplus::PointF((rect.x + rect.w * 0.43f) * s, (rect.y + rect.h * 0.72f) * s),
            Gdiplus::PointF((rect.x + rect.w * 0.78f) * s, (rect.y + rect.h * 0.30f) * s),
        };
        graphics_.DrawLines(&pen, points, 3);
    }
}

void Canvas::PushClip(const Rect& rect) {
    clipStates_.push_back(graphics_.Save());
    graphics_.SetClip(Px(rect), Gdiplus::CombineModeIntersect);
    ++clipDepth_;
}

void Canvas::PopClip() {
    if (clipDepth_ <= 0) return;
    graphics_.Restore(clipStates_.back());
    clipStates_.pop_back();
    --clipDepth_;
}

void Canvas::Circle(float centerX, float centerY, float radius, const Gdiplus::Color& fill) {
    Gdiplus::SolidBrush brush(fill);
    float s = scale();
    graphics_.FillEllipse(&brush, (centerX - radius) * s, (centerY - radius) * s,
                          radius * 2 * s, radius * 2 * s);
}

void Canvas::CircleOutline(float centerX, float centerY, float radius,
                           const Gdiplus::Color& color, float width) {
    Gdiplus::Pen pen(color, width * scale());
    float s = scale();
    graphics_.DrawEllipse(&pen, (centerX - radius) * s, (centerY - radius) * s,
                          radius * 2 * s, radius * 2 * s);
}

void Canvas::Triangle(float x1, float y1, float x2, float y2, float x3, float y3,
                      const Gdiplus::Color& fill) {
    Gdiplus::SolidBrush brush(fill);
    float s = scale();
    Gdiplus::PointF points[3] = {
        Gdiplus::PointF(x1 * s, y1 * s),
        Gdiplus::PointF(x2 * s, y2 * s),
        Gdiplus::PointF(x3 * s, y3 * s),
    };
    graphics_.FillPolygon(&brush, points, 3);
}

void Canvas::SetupTextFormat(Gdiplus::StringFormat& format, Gdiplus::StringAlignment align,
                             Gdiplus::StringAlignment lineAlign, bool wrap) const {
    format.SetAlignment(align);
    format.SetLineAlignment(lineAlign);
    format.SetHotkeyPrefix(Gdiplus::HotkeyPrefixNone);
    format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    if (!wrap) {
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
    }
}

void ApplyWindowTheme(HWND hwnd, bool dark) {
    if (!hwnd) return;
    const BOOL enabled = dark ? TRUE : FALSE;
    // 20 is DWMWA_USE_IMMERSIVE_DARK_MODE on current Windows; 19 was used by early Win10.
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &enabled, sizeof(enabled)))) {
        DwmSetWindowAttribute(hwnd, 19, &enabled, sizeof(enabled));
    }
    const COLORREF caption = dark ? RGB(11, 15, 23) : static_cast<COLORREF>(0xFFFFFFFF);
    const COLORREF border = dark ? RGB(35, 43, 58) : static_cast<COLORREF>(0xFFFFFFFF);
    const COLORREF text = dark ? RGB(232, 236, 245) : static_cast<COLORREF>(0xFFFFFFFF);
    DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, 34, &border, sizeof(border));
    DwmSetWindowAttribute(hwnd, 36, &text, sizeof(text));
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

}  // namespace ui
