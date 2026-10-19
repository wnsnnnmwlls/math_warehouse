#pragma once

#include "ui.h"

#include <windows.h>

// Desmos-style math keyboard. The target is normally a native EDIT control.
// The popup is created with WS_EX_NOACTIVATE so the caret stays in target.
bool ShowMathKeyboard(HWND owner, HWND target, ui::Theme& theme);
void CloseMathKeyboard();
bool MathKeyboardVisible();
