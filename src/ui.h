#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ui {

struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;

    float right() const { return x + w; }
    float bottom() const { return y + h; }
    bool Contains(float px, float py) const {
        return px >= x && py >= y && px <= x + w && py <= y + h;
    }
    Rect Inset(float dx, float dy) const {
        return Rect{x + dx, y + dy, w - dx * 2, h - dy * 2};
    }
    Rect Offset(float dx, float dy) const {
        return Rect{x + dx, y + dy, w, h};
    }
};

inline Gdiplus::Color Color(int r, int g, int b, int a = 255) {
    return Gdiplus::Color(static_cast<BYTE>(a), static_cast<BYTE>(r),
                          static_cast<BYTE>(g), static_cast<BYTE>(b));
}

inline Gdiplus::Color Alpha(const Gdiplus::Color& color, int alpha) {
    return Gdiplus::Color(static_cast<BYTE>(alpha), color.GetR(), color.GetG(), color.GetB());
}

class Theme {
public:
    Theme();
    ~Theme();

    bool Initialize(float scale);
    float scale() const { return scale_; }
    void SetDark(bool dark) { dark_ = dark; }
    bool dark() const { return dark_; }
    Gdiplus::Font* Font(float pixels, bool bold = false);
    float LineHeight(float pixels) const { return pixels * 1.45f; }

private:
    float scale_ = 1.0f;
    bool dark_ = false;
    std::unique_ptr<Gdiplus::FontFamily> family_;
    std::map<int, std::unique_ptr<Gdiplus::Font>> fonts_;
};

// Matches the native title bar to the current light/dark appearance.
void ApplyWindowTheme(HWND hwnd, bool dark);

class Canvas {
public:
    Canvas(Gdiplus::Graphics& graphics, Theme& theme);

    Theme& theme() { return theme_; }
    float scale() const { return theme_.scale(); }

    void FillRect(const Rect& rect, const Gdiplus::Color& color, float radius = 0.0f);
    void StrokeRect(const Rect& rect, const Gdiplus::Color& color, float radius = 0.0f,
                    float width = 1.0f);
    void DrawShadow(const Rect& rect, float radius = 0.0f);
    void Line(float x1, float y1, float x2, float y2, const Gdiplus::Color& color,
              float width = 1.0f);
    void Text(const std::wstring& text, const Rect& rect, float pixels, const Gdiplus::Color& color,
              bool bold = false, Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear,
              Gdiplus::StringAlignment lineAlign = Gdiplus::StringAlignmentCenter, bool wrap = false);
    void TextEllipsis(const std::wstring& text, const Rect& rect, float pixels,
                      const Gdiplus::Color& color, bool bold = false,
                      Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear);
    void TextWrapped(const std::wstring& text, const Rect& rect, float pixels,
                     const Gdiplus::Color& color, bool bold = false,
                     Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear,
                     Gdiplus::StringAlignment lineAlign = Gdiplus::StringAlignmentNear);
    float TextBlock(const std::wstring& text, const Rect& rect, float pixels,
                    const Gdiplus::Color& color, int maxLines, bool bold = false);
    Gdiplus::SizeF Measure(const std::wstring& text, float pixels, bool bold = false, bool wrap = false,
                           float maxWidth = 0.0f);
    void Checkbox(const Rect& rect, bool checked, bool hovered = false, bool disabled = false);
    void PushClip(const Rect& rect);
    void PopClip();
    void Circle(float centerX, float centerY, float radius, const Gdiplus::Color& fill);
    void CircleOutline(float centerX, float centerY, float radius, const Gdiplus::Color& color,
                       float width = 1.5f);
    void Triangle(float x1, float y1, float x2, float y2, float x3, float y3,
                  const Gdiplus::Color& fill);

private:
    Gdiplus::Graphics& graphics_;
    Theme& theme_;
    int clipDepth_ = 0;
    std::vector<Gdiplus::GraphicsState> clipStates_;

    Gdiplus::RectF Px(const Rect& rect) const;
    void SetupTextFormat(Gdiplus::StringFormat& format, Gdiplus::StringAlignment align,
                         Gdiplus::StringAlignment lineAlign, bool wrap) const;
};

}  // namespace ui
