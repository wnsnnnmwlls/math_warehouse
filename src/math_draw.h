#pragma once

#include "ui.h"

#include <string>

namespace mathdraw {

// Draws a single line of text, typesetting simple inline fractions such as
// x^2/4, (cos 2a)/(sin a+cos a) or 1/2 with a horizontal bar.
// Returns false when the text has no typesettable fraction or cannot fit on
// one line; the caller should fall back to its normal text renderer.
bool DrawLine(ui::Canvas& canvas, const std::wstring& text, const ui::Rect& rect,
              float pixels, const Gdiplus::Color& color, bool bold = false,
              Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear);

}  // namespace mathdraw
