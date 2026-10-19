#include "keyboard.h"

#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr wchar_t kKeyboardClass[] = L"MathPaperStudioMathKeyboard";
constexpr float kDesignWidth = 832.0f;
constexpr float kDesignHeight = 272.0f;

enum class KeyAction { Insert, Backspace, Left, Right, Enter, Tab, Close, Space };

struct Key {
    ui::Rect rect;
    std::wstring label;
    std::wstring insert;
    KeyAction action = KeyAction::Insert;
    std::wstring tab;
    bool primary = false;
    bool function = false;
};

struct Palette {
    Gdiplus::Color bg;
    Gdiplus::Color border;
    Gdiplus::Color key;
    Gdiplus::Color keyHover;
    Gdiplus::Color special;
    Gdiplus::Color specialHover;
    Gdiplus::Color ink;
    Gdiplus::Color muted;
    Gdiplus::Color blue;
    Gdiplus::Color blueHover;
    Gdiplus::Color onBlue;
    Gdiplus::Color tabActive;
    Gdiplus::Color tabText;
};

Palette MakePalette(bool dark) {
    if (!dark) {
        return Palette{ui::Color(255, 255, 255), ui::Color(218, 222, 232),
                       ui::Color(249, 250, 253), ui::Color(240, 242, 248),
                       ui::Color(233, 236, 242), ui::Color(224, 228, 236),
                       ui::Color(29, 36, 52),      ui::Color(112, 121, 138),
                       ui::Color(43, 107, 228),    ui::Color(32, 92, 208),
                       ui::Color(255, 255, 255),   ui::Color(238, 237, 255),
                       ui::Color(79, 70, 229)};
    }
    return Palette{ui::Color(20, 26, 37),       ui::Color(43, 53, 70),
                   ui::Color(32, 40, 54),       ui::Color(42, 51, 68),
                   ui::Color(39, 48, 64),       ui::Color(50, 61, 81),
                   ui::Color(231, 235, 245),    ui::Color(148, 158, 177),
                   ui::Color(90, 137, 255),     ui::Color(112, 154, 255),
                   ui::Color(255, 255, 255),    ui::Color(38, 36, 71),
                   ui::Color(151, 145, 255)};
}

class MathKeyboard {
public:
    bool Show(HWND owner, HWND target, ui::Theme& theme);
    void Close();
    bool visible() const { return hwnd_ != nullptr; }

private:
    HWND hwnd_ = nullptr;
    HWND owner_ = nullptr;
    HWND target_ = nullptr;
    ui::Theme* theme_ = nullptr;
    std::vector<Key> keys_;
    int tab_ = 0;  // 0 numbers, 1 letters, 2 functions
    bool shift_ = false;
    int hoverIndex_ = -1;
    bool tracking_ = false;
    UINT dpi_ = 96;
    struct ExtraHit {
        ui::Rect rect;
        std::wstring action;
        std::wstring payload;
    };
    std::vector<ExtraHit> extraHits_;

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Paint();
    void BuildKeys();
    Key& AddKey(int column, int span, float row, const std::wstring& label,
                const std::wstring& insert, KeyAction action = KeyAction::Insert,
                bool special = false, bool primary = false);
    void AddHit(const ui::Rect& rect, std::wstring action, std::wstring payload = {});
    void OnClick(float x, float y);
    void InsertText(const std::wstring& text);
    void DeleteBackward();
    void MoveCaret(bool right);
    void Execute(const Key& key);
    bool TargetIsEdit() const;
};

MathKeyboard* g_keyboard = nullptr;

bool MathKeyboard::TargetIsEdit() const {
    if (!target_ || !IsWindow(target_)) return false;
    wchar_t className[64] = {};
    GetClassNameW(target_, className, 64);
    return wcscmp(className, L"Edit") == 0;
}

bool MathKeyboard::Show(HWND owner, HWND target, ui::Theme& theme) {
    if (!owner || !target || !IsWindow(owner) || !IsWindow(target)) return false;
    target_ = target;
    theme_ = &theme;
    if (hwnd_) {
        owner_ = owner;
        SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return true;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &MathKeyboard::StaticWndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kKeyboardClass;
    RegisterClassExW(&wc);

    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    dpi_ = 96;
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using GetDpiForWindowProc = UINT(WINAPI*)(HWND);
        auto getDpi = reinterpret_cast<GetDpiForWindowProc>(
            GetProcAddress(user32, "GetDpiForWindow"));
        if (getDpi) dpi_ = getDpi(owner);
    }
    float scale = std::max(0.75f, static_cast<float>(dpi_) / 96.0f);
    int width = static_cast<int>(std::lround(kDesignWidth * scale));
    int height = static_cast<int>(std::lround(kDesignHeight * scale));
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    int y = ownerRect.bottom - height - static_cast<int>(std::lround(68.0f * scale));
    int minX = static_cast<int>(work.left) + 8;
    int maxX = std::max(minX, static_cast<int>(work.right) - width - 8);
    int minY = static_cast<int>(work.top) + 8;
    int maxY = std::max(minY, static_cast<int>(work.bottom) - height - 8);
    x = std::clamp(x, minX, maxX);
    y = std::clamp(y, minY, maxY);

    hwnd_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kKeyboardClass,
                            L"数学键盘", WS_POPUP, x, y, width, height, owner, nullptr,
                            instance, this);
    if (!hwnd_) return false;
    owner_ = owner;
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    return true;
}

void MathKeyboard::Close() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    target_ = nullptr;
    owner_ = nullptr;
    g_keyboard = nullptr;
}

Key& MathKeyboard::AddKey(int column, int span, float row, const std::wstring& label,
                          const std::wstring& insert, KeyAction action, bool special,
                          bool primary) {
    const float pad = 12.0f;
    const float gap = 5.0f;
    const float available = kDesignWidth - pad * 2.0f;
    const float cell = (available - gap * 11.0f) / 12.0f;
    const float top = 48.0f;
    const float keyH = 48.0f;
    ui::Rect rect{pad + static_cast<float>(column) * (cell + gap),
                  top + row * (keyH + 6.0f),
                  cell * static_cast<float>(span) + gap * static_cast<float>(span - 1),
                  keyH};
    keys_.push_back(Key{rect, label, insert, action, {}, primary, special});
    return keys_.back();
}

void MathKeyboard::AddHit(const ui::Rect& rect, std::wstring action, std::wstring payload) {
    extraHits_.push_back(ExtraHit{rect, std::move(action), std::move(payload)});
}

void MathKeyboard::BuildKeys() {
    keys_.clear();
    if (tab_ == 0) {
        AddKey(0, 1, 0, L"x", L"x");
        AddKey(1, 1, 0, L"y", L"y");
        AddKey(2, 1, 0, L"a²", L"²");
        AddKey(3, 1, 0, L"aᵇ", L"^");
        AddKey(4, 1, 0, L"7", L"7");
        AddKey(5, 1, 0, L"8", L"8");
        AddKey(6, 1, 0, L"9", L"9");
        AddKey(7, 1, 0, L"÷", L"÷");
        AddKey(8, 4, 0, L"函数", L"", KeyAction::Tab, true, false).tab = L"函数";
        AddKey(0, 1, 1, L"(", L"(");
        AddKey(1, 1, 1, L")", L")");
        AddKey(2, 1, 1, L"<", L"<");
        AddKey(3, 1, 1, L">", L">");
        AddKey(4, 1, 1, L"4", L"4");
        AddKey(5, 1, 1, L"5", L"5");
        AddKey(6, 1, 1, L"6", L"6");
        AddKey(7, 1, 1, L"×", L"×");
        AddKey(8, 2, 1, L"←", L"", KeyAction::Left, true);
        AddKey(10, 2, 1, L"→", L"", KeyAction::Right, true);
        AddKey(0, 1, 2, L"|a|", L"|");
        AddKey(1, 1, 2, L",", L",");
        AddKey(2, 1, 2, L"≤", L"≤");
        AddKey(3, 1, 2, L"≥", L"≥");
        AddKey(4, 1, 2, L"1", L"1");
        AddKey(5, 1, 2, L"2", L"2");
        AddKey(6, 1, 2, L"3", L"3");
        AddKey(7, 1, 2, L"−", L"−");
        AddKey(8, 4, 2, L"⌫", L"", KeyAction::Backspace, true);
        AddKey(0, 1, 3, L"ABC", L"", KeyAction::Tab, true).tab = L"ABC";
        AddKey(1, 1, 3, L"␣", L" ", KeyAction::Space, true);
        AddKey(2, 1, 3, L"√", L"√");
        AddKey(3, 1, 3, L"π", L"π");
        AddKey(4, 1, 3, L"0", L"0");
        AddKey(5, 1, 3, L".", L".");
        AddKey(6, 1, 3, L"=", L"=");
        AddKey(7, 1, 3, L"+", L"+");
        AddKey(8, 4, 3, L"↵", L"", KeyAction::Enter, false, true);
    } else if (tab_ == 1) {
        const wchar_t* row0[] = {L"q", L"w", L"e", L"r", L"t", L"y", L"u", L"i", L"o", L"p"};
        for (int i = 0; i < 10; ++i) {
            std::wstring label = row0[i];
            std::wstring insert = shift_ ? std::wstring(1, static_cast<wchar_t>(towupper(label[0]))) : label;
            AddKey(i, 1, 0, label, insert);
        }
        AddKey(10, 2, 0, L"⌫", L"", KeyAction::Backspace, true);
        const wchar_t* row1[] = {L"a", L"s", L"d", L"f", L"g", L"h", L"j", L"k", L"l", L"θ"};
        for (int i = 0; i < 10; ++i) {
            std::wstring label = row1[i];
            std::wstring insert = shift_ ? std::wstring(1, static_cast<wchar_t>(towupper(label[0]))) : label;
            AddKey(i, 1, 1, label, insert);
        }
        AddKey(10, 2, 1, L"π", L"π", KeyAction::Insert, true);
        AddKey(0, 2, 2, L"⇧", L"", KeyAction::Tab, true, false).tab = L"shift";
        const wchar_t* row2[] = {L"z", L"x", L"c", L"v", L"b", L"n", L"m"};
        for (int i = 0; i < 7; ++i) {
            std::wstring label = row2[i];
            std::wstring insert = shift_ ? std::wstring(1, static_cast<wchar_t>(towupper(label[0]))) : label;
            AddKey(2 + i, 1, 2, label, insert);
        }
        AddKey(9, 3, 2, L"⌫", L"", KeyAction::Backspace, true);
        AddKey(0, 2, 3, L"123", L"", KeyAction::Tab, true).tab = L"123";
        AddKey(2, 1, 3, L"a_b", L"_");
        AddKey(3, 1, 3, L"!'", L"!");
        AddKey(4, 1, 3, L"[]", L"[]");
        AddKey(5, 1, 3, L"{}", L"{}");
        AddKey(6, 1, 3, L"~%", L"%");
        AddKey(7, 1, 3, L":;", L":");
        AddKey(8, 4, 3, L"↵", L"", KeyAction::Enter, false, true);
    } else {
        AddKey(0, 1, 0, L"sin", L"sin(");
        AddKey(1, 1, 0, L"cos", L"cos(");
        AddKey(2, 1, 0, L"tan", L"tan(");
        AddKey(3, 1, 0, L"cot", L"cot(");
        AddKey(4, 1, 0, L"sec", L"sec(");
        AddKey(5, 1, 0, L"csc", L"csc(");
        AddKey(6, 1, 0, L"arcsin", L"arcsin(");
        AddKey(7, 1, 0, L"arccos", L"arccos(");
        AddKey(8, 1, 0, L"arctan", L"arctan(");
        AddKey(9, 1, 0, L"ln", L"ln(");
        AddKey(10, 1, 0, L"log", L"log(");
        AddKey(11, 1, 0, L"exp", L"exp(");
        AddKey(0, 1, 1, L"√", L"√(");
        AddKey(1, 1, 1, L"|a|", L"|");
        AddKey(2, 1, 1, L"π", L"π");
        AddKey(3, 1, 1, L"e", L"e");
        AddKey(4, 1, 1, L"≤", L"≤");
        AddKey(5, 1, 1, L"≥", L"≥");
        AddKey(6, 1, 1, L"≠", L"≠");
        AddKey(7, 1, 1, L"∞", L"∞");
        AddKey(8, 1, 1, L"∑", L"∑");
        AddKey(9, 1, 1, L"∫", L"∫");
        AddKey(10, 1, 1, L"^", L"^");
        AddKey(11, 1, 1, L"°", L"°");
        AddKey(0, 1, 2, L"x²", L"²");
        AddKey(1, 1, 2, L"xⁿ", L"^");
        AddKey(2, 1, 2, L"<", L"<");
        AddKey(3, 1, 2, L">", L">");
        AddKey(4, 1, 2, L"(", L"(");
        AddKey(5, 1, 2, L")", L")");
        AddKey(6, 1, 2, L",", L",");
        AddKey(7, 1, 2, L";", L";");
        AddKey(8, 4, 2, L"⌫", L"", KeyAction::Backspace, true);
        AddKey(0, 2, 3, L"123", L"", KeyAction::Tab, true).tab = L"123";
        AddKey(2, 2, 3, L"ABC", L"", KeyAction::Tab, true).tab = L"ABC";
        AddKey(4, 2, 3, L"函数", L"", KeyAction::Tab, true).tab = L"函数";
        AddKey(6, 2, 3, L"←", L"", KeyAction::Left, true);
        AddKey(8, 2, 3, L"→", L"", KeyAction::Right, true);
        AddKey(10, 2, 3, L"↵", L"", KeyAction::Enter, false, true);
    }
}

void MathKeyboard::InsertText(const std::wstring& text) {
    if (!TargetIsEdit()) return;
    std::wstring value = text;
    if (text == L"[]") value = L"[]";
    else if (text == L"{}") value = L"{}";
    SendMessageW(target_, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(value.c_str()));
    SetFocus(target_);
}

void MathKeyboard::DeleteBackward() {
    if (!TargetIsEdit()) return;
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(target_, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                 reinterpret_cast<LPARAM>(&end));
    if (start != end) {
        SendMessageW(target_, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
    } else if (start > 0) {
        SendMessageW(target_, EM_SETSEL, start - 1, start);
        SendMessageW(target_, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
    }
    SetFocus(target_);
}

void MathKeyboard::MoveCaret(bool right) {
    if (!TargetIsEdit()) return;
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(target_, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                 reinterpret_cast<LPARAM>(&end));
    int length = GetWindowTextLengthW(target_);
    DWORD position = right ? std::min<DWORD>(static_cast<DWORD>(length), end + 1)
                           : (start > 0 ? start - 1 : 0);
    SendMessageW(target_, EM_SETSEL, position, position);
    SetFocus(target_);
}

void MathKeyboard::Execute(const Key& key) {
    switch (key.action) {
        case KeyAction::Insert: InsertText(key.insert); break;
        case KeyAction::Space: InsertText(L" "); break;
        case KeyAction::Backspace: DeleteBackward(); break;
        case KeyAction::Left: MoveCaret(false); break;
        case KeyAction::Right: MoveCaret(true); break;
        case KeyAction::Enter:
            if (TargetIsEdit()) {
                LONG style = GetWindowLongW(target_, GWL_STYLE);
                if (style & ES_MULTILINE) InsertText(L"\r\n");
                else {
                    SendMessageW(target_, WM_KEYDOWN, VK_RETURN, 0);
                    SendMessageW(target_, WM_KEYUP, VK_RETURN, 0);
                }
            }
            break;
        case KeyAction::Tab:
            if (key.tab == L"shift") {
                shift_ = !shift_;
                BuildKeys();
            } else if (key.tab == L"123") {
                tab_ = 0; shift_ = false; BuildKeys();
            } else if (key.tab == L"ABC") {
                tab_ = 1; shift_ = false; BuildKeys();
            } else {
                tab_ = 2; shift_ = false; BuildKeys();
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
            break;
        case KeyAction::Close: Close(); break;
    }
    if (hwnd_ && key.action != KeyAction::Close) InvalidateRect(hwnd_, nullptr, FALSE);
}

void MathKeyboard::Paint() {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    if (!dc) return;
    RECT client{};
    GetClientRect(hwnd_, &client);
    int width = client.right;
    int height = client.bottom;
    if (width <= 0 || height <= 0) {
        EndPaint(hwnd_, &paint);
        return;
    }
    const bool dark = theme_ && theme_->dark();
    const Palette p = MakePalette(dark);
    Gdiplus::Bitmap buffer(width, height, PixelFormat32bppRGB);
    Gdiplus::Graphics graphics(&buffer);
    graphics.Clear(p.bg);
    ui::Canvas canvas(graphics, *theme_);
    BuildKeys();
    extraHits_.clear();

    canvas.FillRect(ui::Rect{0, 0, kDesignWidth, kDesignHeight}, p.bg, 14.0f);
    canvas.StrokeRect(ui::Rect{0.5f, 0.5f, kDesignWidth - 1.0f, kDesignHeight - 1.0f},
                      p.border, 14.0f, 1.0f);
    const wchar_t* tabs[] = {L"123", L"ABC", L"函数"};
    for (int i = 0; i < 3; ++i) {
        ui::Rect tab{12.0f + static_cast<float>(i) * 76.0f, 9.0f, 68.0f, 28.0f};
        bool active = tab_ == i;
        canvas.FillRect(tab, active ? p.tabActive : p.key, 7.0f);
        canvas.Text(tabs[i], tab, 11.5f, active ? p.tabText : p.muted, active,
                    Gdiplus::StringAlignmentCenter);
        AddHit(tab, L"tab", tabs[i]);
    }
    // Close button.
    ui::Rect close{kDesignWidth - 40.0f, 10.0f, 26.0f, 26.0f};
    canvas.Text(L"×", close, 18.0f, p.muted, false, Gdiplus::StringAlignmentCenter);
    AddHit(close, L"close", L"");

    for (size_t i = 0; i < keys_.size(); ++i) {
        const Key& key = keys_[i];
        bool hovered = hoverIndex_ == static_cast<int>(i);
        Gdiplus::Color fill = key.primary ? (hovered ? p.blueHover : p.blue)
                                          : (key.function ? (hovered ? p.specialHover : p.special)
                                                          : (hovered ? p.keyHover : p.key));
        Gdiplus::Color text = key.primary ? p.onBlue : p.ink;
        float radius = key.primary ? 7.0f : 6.0f;
        canvas.FillRect(key.rect, fill, radius);
        canvas.StrokeRect(key.rect, key.primary ? p.blue : p.border, radius, 1.0f);
        canvas.Text(key.label, key.rect, key.label.size() > 4 ? 10.0f : 13.0f, text,
                    key.primary || key.label.size() <= 2, Gdiplus::StringAlignmentCenter,
                    Gdiplus::StringAlignmentCenter, false);
    }

    Gdiplus::Graphics target(dc);
    target.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    target.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    target.DrawImage(&buffer, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height,
                     Gdiplus::UnitPixel);
    EndPaint(hwnd_, &paint);
}

void MathKeyboard::OnClick(float x, float y) {
    for (size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i].rect.Contains(x, y)) {
            Execute(keys_[i]);
            return;
        }
    }
    for (const auto& hit : extraHits_) {
        if (!hit.rect.Contains(x, y)) continue;
        if (hit.action == L"close") {
            Close();
        } else if (hit.action == L"tab") {
            if (hit.payload == L"123") { tab_ = 0; shift_ = false; BuildKeys(); }
            else if (hit.payload == L"ABC") { tab_ = 1; shift_ = false; BuildKeys(); }
            else { tab_ = 2; shift_ = false; BuildKeys(); }
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return;
    }
}

LRESULT CALLBACK MathKeyboard::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam,
                                              LPARAM lParam) {
    MathKeyboard* keyboard =
        reinterpret_cast<MathKeyboard*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!keyboard && message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        keyboard = reinterpret_cast<MathKeyboard*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(keyboard));
    }
    if (keyboard) return keyboard->HandleMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MathKeyboard::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            Paint();
            return 0;
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(GET_X_LPARAM(lParam)) / (theme_ ? theme_->scale() : 1.0f);
            float y = static_cast<float>(GET_Y_LPARAM(lParam)) / (theme_ ? theme_->scale() : 1.0f);
            hoverIndex_ = -1;
            for (size_t i = 0; i < keys_.size(); ++i) {
                if (keys_[i].rect.Contains(x, y)) {
                    hoverIndex_ = static_cast<int>(i);
                    break;
                }
            }
            if (!tracking_) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd;
                TrackMouseEvent(&track);
                tracking_ = true;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSELEAVE:
            tracking_ = false;
            hoverIndex_ = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_LBUTTONUP:
            OnClick(static_cast<float>(GET_X_LPARAM(lParam)) / (theme_ ? theme_->scale() : 1.0f),
                    static_cast<float>(GET_Y_LPARAM(lParam)) / (theme_ ? theme_->scale() : 1.0f));
            return 0;
        case WM_CLOSE:
            Close();
            return 0;
        case WM_DESTROY:
            if (g_keyboard == this) g_keyboard = nullptr;
            hwnd_ = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

bool ShowMathKeyboard(HWND owner, HWND target, ui::Theme& theme) {
    if (!g_keyboard) g_keyboard = new MathKeyboard();
    if (!g_keyboard->Show(owner, target, theme)) {
        delete g_keyboard;
        g_keyboard = nullptr;
        return false;
    }
    return true;
}

void CloseMathKeyboard() {
    if (g_keyboard) {
        MathKeyboard* keyboard = g_keyboard;
        g_keyboard = nullptr;
        keyboard->Close();
        delete keyboard;
    }
}

bool MathKeyboardVisible() {
    return g_keyboard != nullptr && g_keyboard->visible();
}
