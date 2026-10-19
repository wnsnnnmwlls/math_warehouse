#include "app.h"

#include "ai.h"
#include "docx.h"
#include "editor.h"
#include "image_data.h"
#include "math_text.h"
#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr UINT kMsgAiUpdate = WM_APP + 11;
constexpr UINT kMsgAiFinished = WM_APP + 12;
constexpr UINT kMsgTestFinished = WM_APP + 13;
constexpr UINT kMsgImageFinished = WM_APP + 14;
constexpr UINT_PTR kToastTimer = 1;
constexpr wchar_t kWindowClass[] = L"MathPaperStudioMainWindow";

namespace col {
Gdiplus::Color Background;
Gdiplus::Color Card;
Gdiplus::Color CardAlt;
Gdiplus::Color Field;
Gdiplus::Color Ink;
Gdiplus::Color InkSoft;
Gdiplus::Color Muted;
Gdiplus::Color Faint;
Gdiplus::Color Border;
Gdiplus::Color BorderStrong;
Gdiplus::Color Accent;
Gdiplus::Color AccentDark;
Gdiplus::Color AccentSoft;
Gdiplus::Color AccentSoft2;
Gdiplus::Color Navy;
Gdiplus::Color Green;
Gdiplus::Color GreenSoft;
Gdiplus::Color Amber;
Gdiplus::Color AmberSoft;
Gdiplus::Color Red;
Gdiplus::Color RedSoft;
Gdiplus::Color Blue;
Gdiplus::Color BlueSoft;
Gdiplus::Color TagText;
Gdiplus::Color ConceptTagText;
Gdiplus::Color OnAccent;
Gdiplus::Color OnLight;
Gdiplus::Color LightButtonHover;
Gdiplus::Color White;
Gdiplus::Color Hover;
Gdiplus::Color HeaderLine;
Gdiplus::Color DisabledFill;
Gdiplus::Color DisabledText;
Gdiplus::Color DisabledBorder;
Gdiplus::Color RedHover;
Gdiplus::Color AccentSoftHover;
Gdiplus::Color OnNavyMuted;
Gdiplus::Color ScrollTrack;
Gdiplus::Color ScrollThumb;
Gdiplus::Color AccentBorder;
Gdiplus::Color AccentRowHover;
Gdiplus::Color Track;
Gdiplus::Color StatusInfoBorder;
Gdiplus::Color StatusOkBorder;
Gdiplus::Color StatusFailBorder;
Gdiplus::Color InputBorder;
Gdiplus::Color GridLine;
Gdiplus::Color GridLineSoft;
Gdiplus::Color ToastBg;
Gdiplus::Color ToastText;
Gdiplus::Color ToastErrorBg;
Gdiplus::Color ToastErrorText;
Gdiplus::Color ToastOkDot;

void Apply(bool dark) {
    White = ui::Color(255, 255, 255);
    OnAccent = White;
    OnLight = ui::Color(22, 29, 45);
    LightButtonHover = ui::Color(235, 238, 245);
    if (!dark) {
        Background = ui::Color(245, 246, 250);
        Card = ui::Color(255, 255, 255);
        CardAlt = ui::Color(250, 250, 253);
        Field = ui::Color(255, 255, 255);
        Ink = ui::Color(22, 29, 45);
        InkSoft = ui::Color(49, 58, 78);
        Muted = ui::Color(105, 113, 130);
        Faint = ui::Color(145, 152, 167);
        Border = ui::Color(228, 231, 239);
        BorderStrong = ui::Color(209, 214, 226);
        Accent = ui::Color(79, 70, 229);
        AccentDark = ui::Color(67, 56, 202);
        AccentSoft = ui::Color(238, 237, 255);
        AccentSoft2 = ui::Color(247, 246, 255);
        Navy = ui::Color(23, 29, 47);
        Green = ui::Color(22, 138, 91);
        GreenSoft = ui::Color(233, 248, 240);
        Amber = ui::Color(178, 106, 9);
        AmberSoft = ui::Color(255, 244, 227);
        Red = ui::Color(194, 65, 75);
        RedSoft = ui::Color(254, 239, 241);
        Blue = ui::Color(43, 101, 174);
        BlueSoft = ui::Color(237, 244, 254);
        TagText = ui::Color(81, 69, 205);
        ConceptTagText = ui::Color(42, 99, 160);
        Hover = ui::Color(246, 246, 251);
        HeaderLine = ui::Color(236, 238, 244);
        DisabledFill = ui::Color(239, 241, 246);
        DisabledText = ui::Color(166, 171, 184);
        DisabledBorder = ui::Color(232, 234, 241);
        RedHover = ui::Color(174, 48, 59);
        AccentSoftHover = ui::Color(227, 224, 255);
        OnNavyMuted = ui::Color(194, 201, 216);
        ScrollTrack = ui::Color(238, 240, 245);
        ScrollThumb = ui::Color(188, 194, 208);
        AccentBorder = ui::Color(203, 199, 255);
        AccentRowHover = ui::Color(242, 241, 255);
        Track = ui::Color(235, 238, 243);
        StatusInfoBorder = ui::Color(209, 224, 244);
        StatusOkBorder = ui::Color(198, 235, 218);
        StatusFailBorder = ui::Color(244, 205, 213);
        InputBorder = ui::Color(221, 224, 232);
        GridLine = ui::Color(215, 219, 228);
        GridLineSoft = ui::Color(230, 233, 240);
        ToastBg = ui::Color(31, 38, 54);
        ToastText = ui::Color(246, 247, 251);
        ToastErrorBg = ui::Color(73, 32, 39);
        ToastErrorText = ui::Color(248, 162, 170);
        ToastOkDot = ui::Color(126, 231, 188);
    } else {
        Background = ui::Color(12, 16, 23);
        Card = ui::Color(21, 27, 38);
        CardAlt = ui::Color(17, 23, 34);
        Field = ui::Color(15, 21, 32);
        Ink = ui::Color(232, 236, 245);
        InkSoft = ui::Color(199, 206, 220);
        Muted = ui::Color(152, 161, 179);
        Faint = ui::Color(111, 121, 140);
        Border = ui::Color(38, 48, 65);
        BorderStrong = ui::Color(52, 64, 86);
        Accent = ui::Color(139, 132, 255);
        AccentDark = ui::Color(115, 107, 255);
        AccentSoft = ui::Color(38, 36, 71);
        AccentSoft2 = ui::Color(30, 32, 51);
        Navy = ui::Color(26, 34, 48);
        Green = ui::Color(91, 214, 163);
        GreenSoft = ui::Color(22, 53, 43);
        Amber = ui::Color(233, 185, 110);
        AmberSoft = ui::Color(58, 45, 27);
        Red = ui::Color(255, 123, 134);
        RedSoft = ui::Color(58, 34, 40);
        Blue = ui::Color(120, 183, 255);
        BlueSoft = ui::Color(27, 43, 64);
        TagText = ui::Color(183, 177, 255);
        ConceptTagText = ui::Color(141, 199, 255);
        Hover = ui::Color(32, 40, 54);
        HeaderLine = ui::Color(32, 40, 54);
        DisabledFill = ui::Color(26, 33, 48);
        DisabledText = ui::Color(94, 105, 125);
        DisabledBorder = ui::Color(38, 48, 65);
        RedHover = ui::Color(226, 98, 110);
        AccentSoftHover = ui::Color(49, 46, 92);
        OnNavyMuted = ui::Color(170, 179, 197);
        ScrollTrack = ui::Color(26, 34, 48);
        ScrollThumb = ui::Color(58, 69, 88);
        AccentBorder = ui::Color(76, 70, 140);
        AccentRowHover = ui::Color(37, 36, 67);
        Track = ui::Color(34, 43, 58);
        StatusInfoBorder = ui::Color(46, 70, 104);
        StatusOkBorder = ui::Color(42, 90, 70);
        StatusFailBorder = ui::Color(99, 52, 59);
        InputBorder = ui::Color(42, 52, 70);
        GridLine = ui::Color(42, 52, 70);
        GridLineSoft = ui::Color(34, 43, 58);
        ToastBg = ui::Color(34, 43, 58);
        ToastText = ui::Color(232, 236, 245);
        ToastErrorBg = ui::Color(74, 37, 44);
        ToastErrorText = ui::Color(255, 154, 164);
        ToastOkDot = ui::Color(110, 226, 176);
    }
}
}  // namespace col

constexpr float kSidebarWidth = 224.0f;
constexpr float kPageX = 254.0f;
constexpr float kPageY = 122.0f;
constexpr float kRightMargin = 30.0f;
constexpr float kBottomMargin = 26.0f;
constexpr float kHeaderSearchWidth = 238.0f;

std::wstring W(const std::string& text) {
    return util::ToWide(text);
}

std::wstring IntText(int value) {
    return std::to_wstring(value);
}

std::wstring GetControlText(HWND control) {
    if (!control) return {};
    int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring text(static_cast<size_t>(length + 1), L'\0');
    int copied = GetWindowTextW(control, text.data(), length + 1);
    text.resize(copied > 0 ? static_cast<size_t>(copied) : 0);
    return text;
}

std::string GetControlUtf8(HWND control) {
    return util::ToUtf8(GetControlText(control));
}

std::wstring NormalizeEditNewlines(const std::wstring& text) {
    std::wstring normalized;
    normalized.reserve(text.size() + 8);
    for (wchar_t c : text) {
        if (c == L'\r') continue;
        if (c == L'\n') {
            normalized.push_back(L'\r');
            normalized.push_back(L'\n');
        } else {
            normalized.push_back(c);
        }
    }
    return normalized;
}

void SetControlText(HWND control, const std::wstring& text) {
    if (control) {
        std::wstring normalized = NormalizeEditNewlines(text);
        SetWindowTextW(control, normalized.c_str());
    }
}

std::wstring PaddedNumber(int value) {
    if (value < 10) return L"0" + std::to_wstring(value);
    return std::to_wstring(value);
}

std::wstring SafeFileStem(std::wstring text) {
    text = util::Trim(text);
    for (wchar_t& c : text) {
        if (c == L'<' || c == L'>' || c == L':' || c == L'"' || c == L'/' || c == L'\\' ||
            c == L'|' || c == L'?' || c == L'*') {
            c = L'_';
        }
    }
    while (!text.empty() && (text.back() == L'.' || text.back() == L' ')) text.pop_back();
    if (text.size() > 56) text.resize(56);
    if (text.empty()) text = L"数学试卷";
    return text;
}

int CountTagUsages(const TagLibrary& library, const Store& store, bool formulas) {
    int used = 0;
    if (formulas) {
        for (const auto& tag : library.formulas) {
            if (store.FormulaUsage(tag) > 0) ++used;
        }
    } else {
        for (const auto& tag : library.concepts) {
            if (store.ConceptUsage(tag) > 0) ++used;
        }
    }
    return used;
}

struct FrameRects {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

FrameRects ComputeFrame(float width, float height) {
    FrameRects frame;
    frame.x = kPageX;
    frame.y = kPageY;
    frame.w = std::max(680.0f, width - kPageX - kRightMargin);
    frame.h = std::max(420.0f, height - kPageY - kBottomMargin);
    return frame;
}

struct BankRects {
    ui::Rect filter;
    ui::Rect listHeader;
    ui::Rect listViewport;
    ui::Rect selectionBar;
    ui::Rect search;
    ui::Rect addButton;
    ui::Rect imageButton;
    ui::Rect aiButton;
};

BankRects ComputeBankRects(float width, float height, bool hasSelection) {
    FrameRects frame = ComputeFrame(width, height);
    BankRects rects;
    rects.filter = ui::Rect{frame.x, frame.y, 272.0f, frame.h};
    rects.listHeader = ui::Rect{frame.x + 290.0f, frame.y, frame.w - 290.0f, 36.0f};
    rects.selectionBar = ui::Rect{frame.x + 290.0f, height - kBottomMargin - 64.0f,
                                  frame.w - 290.0f, 64.0f};
    float listBottom = hasSelection ? rects.selectionBar.y - 12.0f : height - kBottomMargin;
    rects.listViewport = ui::Rect{frame.x + 290.0f, frame.y + 44.0f, frame.w - 290.0f,
                                  std::max(120.0f, listBottom - (frame.y + 44.0f))};
    rects.search = ui::Rect{width - kRightMargin - kHeaderSearchWidth, 38.0f, kHeaderSearchWidth,
                            40.0f};
    rects.addButton = ui::Rect{rects.search.x - 12.0f - 112.0f, 39.0f, 112.0f, 38.0f};
    rects.imageButton = ui::Rect{rects.addButton.x - 10.0f - 112.0f, 39.0f, 112.0f, 38.0f};
    rects.aiButton = ui::Rect{rects.imageButton.x - 10.0f - 140.0f, 39.0f, 140.0f, 38.0f};
    return rects;
}

struct TagsRects {
    ui::Rect stats;
    ui::Rect formulaCard;
    ui::Rect conceptCard;
    ui::Rect headerButton;
};

TagsRects ComputeTagsRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    TagsRects rects;
    rects.stats = ui::Rect{frame.x, frame.y, frame.w, 88.0f};
    rects.headerButton = ui::Rect{width - kRightMargin - 132.0f, 39.0f, 132.0f, 38.0f};
    const float top = frame.y + 104.0f;
    const float columnWidth = (frame.w - 18.0f) * 0.5f;
    const float columnHeight = std::max(220.0f, height - kBottomMargin - top);
    rects.formulaCard = ui::Rect{frame.x, top, columnWidth, columnHeight};
    rects.conceptCard = ui::Rect{frame.x + columnWidth + 18.0f, top, columnWidth, columnHeight};
    return rects;
}

struct AiRects {
    ui::Rect leftCard;
    ui::Rect rightCard;
    ui::Rect workflowCard;
    ui::Rect securityCard;
    ui::Rect presetDeepSeek;
    ui::Rect presetOpenAI;
    ui::Rect presetCustom;
    ui::Rect baseField;
    ui::Rect modelField;
    ui::Rect keyField;
    ui::Rect keyToggle;
    ui::Rect saveButton;
    ui::Rect testButton;
    ui::Rect status;
};

AiRects ComputeAiRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    AiRects rects;
    const float gap = 18.0f;
    float leftWidth = std::clamp(frame.w * 0.58f, 552.0f, 760.0f);
    if (frame.w - leftWidth - gap < 270.0f) {
        leftWidth = std::max(500.0f, frame.w - gap - 270.0f);
    }
    rects.leftCard = ui::Rect{frame.x, frame.y, leftWidth, frame.h};
    rects.rightCard = ui::Rect{frame.x + leftWidth + gap, frame.y,
                               std::max(250.0f, frame.w - leftWidth - gap), frame.h};
    const float innerX = rects.leftCard.x + 28.0f;
    const float innerW = rects.leftCard.w - 56.0f;
    const float presetY = rects.leftCard.y + 94.0f;
    rects.presetDeepSeek = ui::Rect{innerX, presetY, 104.0f, 30.0f};
    rects.presetOpenAI = ui::Rect{innerX + 112.0f, presetY, 96.0f, 30.0f};
    rects.presetCustom = ui::Rect{innerX + 216.0f, presetY, 92.0f, 30.0f};
    rects.baseField = ui::Rect{innerX, rects.leftCard.y + 168.0f, innerW, 42.0f};
    rects.modelField = ui::Rect{innerX, rects.leftCard.y + 260.0f, innerW, 42.0f};
    rects.keyField = ui::Rect{innerX, rects.leftCard.y + 352.0f, innerW, 42.0f};
    rects.keyToggle = ui::Rect{rects.keyField.right() - 58.0f, rects.keyField.y + 9.0f, 46.0f,
                               24.0f};
    rects.saveButton = ui::Rect{innerX, rects.leftCard.y + 448.0f, 132.0f, 40.0f};
    rects.testButton = ui::Rect{innerX + 142.0f, rects.leftCard.y + 448.0f, 132.0f, 40.0f};
    const float statusTop = rects.saveButton.bottom() + 18.0f;
    float availableStatus = std::max(36.0f, rects.leftCard.bottom() - statusTop - 24.0f);
    rects.status = ui::Rect{innerX, statusTop, innerW, std::min(140.0f, availableStatus)};
    const float rightH = rects.rightCard.h;
    float workflowHeight = std::min(360.0f, rightH * 0.55f);
    workflowHeight = std::max(250.0f, workflowHeight);
    rects.workflowCard = ui::Rect{rects.rightCard.x, rects.rightCard.y, rects.rightCard.w,
                                  workflowHeight};
    rects.securityCard = ui::Rect{rects.rightCard.x, rects.workflowCard.bottom() + 16.0f,
                                  rects.rightCard.w,
                                  std::max(120.0f, rightH - workflowHeight - 16.0f)};
    return rects;
}

struct PaperRects {
    ui::Rect stats;
    ui::Rect formCard;
    ui::Rect previewCard;
    ui::Rect titleField;
    ui::Rect minutesField;
    ui::Rect notesField;
    ui::Rect generateButton;
    ui::Rect paper;
    bool compact = false;
};

PaperRects ComputePaperRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    PaperRects rects;
    rects.stats = ui::Rect{frame.x, frame.y, frame.w, 84.0f};
    const float top = frame.y + 100.0f;
    const float gap = 18.0f;
    float formWidth = std::max(500.0f, frame.w * 0.56f);
    if (frame.w - formWidth - gap < 300.0f) {
        formWidth = std::max(460.0f, frame.w - gap - 300.0f);
    }
    const float cardHeight = std::max(360.0f, height - kBottomMargin - top);
    rects.formCard = ui::Rect{frame.x, top, formWidth, cardHeight};
    rects.previewCard = ui::Rect{frame.x + formWidth + gap, top,
                                 std::max(260.0f, frame.w - formWidth - gap), cardHeight};
    rects.compact = cardHeight < 610.0f;
    if (rects.compact) {
        rects.titleField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 78.0f,
                                    rects.formCard.w - 52.0f, 38.0f};
        rects.minutesField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 140.0f,
                                      96.0f, 38.0f};
        rects.notesField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 212.0f,
                                    rects.formCard.w - 52.0f, 74.0f};
        rects.generateButton = ui::Rect{rects.formCard.x + 26.0f,
                                        rects.formCard.bottom() - 70.0f,
                                        rects.formCard.w - 52.0f, 46.0f};
    } else {
        rects.titleField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 96.0f,
                                    rects.formCard.w - 52.0f, 42.0f};
        rects.minutesField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 174.0f,
                                      96.0f, 42.0f};
        rects.notesField = ui::Rect{rects.formCard.x + 26.0f, rects.formCard.y + 250.0f,
                                    rects.formCard.w - 52.0f, 92.0f};
        rects.generateButton = ui::Rect{rects.formCard.x + 26.0f,
                                        rects.formCard.bottom() - 80.0f,
                                        rects.formCard.w - 52.0f, 46.0f};
    }
    const float maxPaperHeight = rects.previewCard.h - 150.0f;
    const float maxPaperWidth = rects.previewCard.w - 70.0f;
    float paperHeight = std::max(180.0f, maxPaperHeight);
    float paperWidth = paperHeight / 1.4142f;
    if (paperWidth > maxPaperWidth) {
        paperWidth = maxPaperWidth;
        paperHeight = paperWidth * 1.4142f;
    }
    rects.paper = ui::Rect{rects.previewCard.x + (rects.previewCard.w - paperWidth) * 0.5f,
                           rects.previewCard.y + 64.0f, paperWidth, paperHeight};
    return rects;
}

struct AiUpdate {
    std::string questionId;
    bool ok = false;
    std::vector<std::string> formulas;
    std::vector<std::string> concepts;
    std::string error;
};

struct TestUpdate {
    bool ok = false;
    std::string message;
};

struct ImageUpdate {
    bool ok = false;
    std::vector<Question> questions;
    std::string error;
};

std::wstring HitKey(const std::wstring& action, const std::wstring& payload) {
    return action + L"\x1F" + payload;
}

std::wstring DifficultyPillText(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return L"A 基础";
        case Difficulty::B: return L"B 提升";
        case Difficulty::C: return L"C 挑战";
    }
    return L"B 提升";
}

Gdiplus::Color DifficultyBackground(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return col::GreenSoft;
        case Difficulty::B: return col::AmberSoft;
        case Difficulty::C: return col::RedSoft;
    }
    return col::AmberSoft;
}

Gdiplus::Color DifficultyForeground(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return col::Green;
        case Difficulty::B: return col::Amber;
        case Difficulty::C: return col::Red;
    }
    return col::Amber;
}

void MergeUniqueTags(std::vector<std::string>& destination,
                     const std::vector<std::string>& additions) {
    for (const auto& tag : additions) {
        std::string clean = util::Trim(tag);
        if (clean.empty()) continue;
        bool exists = std::any_of(destination.begin(), destination.end(),
                                  [&](const std::string& old) { return TagsEqual(old, clean); });
        if (!exists) destination.push_back(clean);
    }
}

}  // namespace

App::App() : store_(util::ExeDirectory()) {}

App::~App() {
    if (aiThread_.joinable()) aiThread_.join();
    if (testThread_.joinable()) testThread_.join();
    if (imageThread_.joinable()) imageThread_.join();
    if (editFont_) DeleteObject(editFont_);
    if (whiteBrush_) DeleteObject(whiteBrush_);
    if (darkFieldBrush_) DeleteObject(darkFieldBrush_);
    if (canvasBrush_) DeleteObject(canvasBrush_);
}

bool App::Create(HINSTANCE instance, int showCommand) {
    instance_ = instance;
    HDC screen = GetDC(nullptr);
    if (screen) {
        dpi_ = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
    }
    scale_ = static_cast<float>(dpi_) / 96.0f;
    if (!theme_.Initialize(scale_)) {
        MessageBoxW(nullptr, L"无法初始化界面字体。", L"数研组卷", MB_ICONERROR);
        return false;
    }
    std::string warning;
    if (!store_.Initialize(&warning)) {
        MessageBoxW(nullptr, W(warning).c_str(), L"无法初始化本地数据", MB_ICONERROR);
        return false;
    }
    if (!warning.empty()) startupWarning_ = W(warning);
    darkMode_ = store_.Config().darkMode;
    theme_.SetDark(darkMode_);
    col::Apply(darkMode_);
    CreateFontsAndBrushes();

    smallIcon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16,
                                               LR_DEFAULTCOLOR));
    largeIcon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(101), IMAGE_ICON, 32, 32,
                                               LR_DEFAULTCOLOR));
    if (!smallIcon_) smallIcon_ = LoadIconW(nullptr, IDI_APPLICATION);
    if (!largeIcon_) largeIcon_ = LoadIconW(nullptr, IDI_APPLICATION);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &App::StaticWndProc;
    wc.hInstance = instance_;
    wc.hIcon = largeIcon_;
    wc.hIconSm = smallIcon_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"无法注册应用窗口。", L"数研组卷", MB_ICONERROR);
        return false;
    }

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int workWidth = workArea.right - workArea.left;
    const int workHeight = workArea.bottom - workArea.top;
    int desiredWidth = static_cast<int>(std::lround(1460.0f * scale_));
    int desiredHeight = static_cast<int>(std::lround(900.0f * scale_));
    desiredWidth = std::min(desiredWidth, workWidth - 60);
    desiredHeight = std::min(desiredHeight, workHeight - 60);
    desiredWidth = std::max(desiredWidth, static_cast<int>(std::lround(1160.0f * scale_)));
    desiredHeight = std::max(desiredHeight, static_cast<int>(std::lround(720.0f * scale_)));
    desiredWidth = std::min(desiredWidth, workWidth);
    desiredHeight = std::min(desiredHeight, workHeight);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    RECT windowRect{0, 0, desiredWidth, desiredHeight};
    AdjustWindowRectEx(&windowRect, style, FALSE, 0);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int x = workArea.left + (workWidth - windowWidth) / 2;
    int y = workArea.top + (workHeight - windowHeight) / 2;

    hwnd_ = CreateWindowExW(0, kWindowClass, L"数研组卷", style, x, y, windowWidth, windowHeight,
                            nullptr, nullptr, instance_, this);
    if (!hwnd_) {
        MessageBoxW(nullptr, L"无法创建应用窗口。", L"数研组卷", MB_ICONERROR);
        return false;
    }
    SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon_));
    SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon_));

    CreateEditControls();
    ui::ApplyWindowTheme(hwnd_, darkMode_);
    ApplyNativeTheme();
    ReadSettingsIntoFields();
    ReadPaperDefaults();
    page_ = Page::Bank;
    RebuildFiltered();
    LayoutEditControls();
    ShowWindow(hwnd_, showCommand);
    UpdateWindow(hwnd_);
    if (!startupWarning_.empty()) {
        ShowToast(startupWarning_, true);
    }
    return true;
}

int App::Run() {
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK App::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!app && message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = reinterpret_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (app) {
        return app->HandleMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!hwnd_ && message != WM_NCCREATE) {
        hwnd_ = hwnd;
    }
    switch (message) {
        case WM_CREATE:
            return 0;
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            LayoutEditControls();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            RECT minClient{0, 0, static_cast<LONG>(std::lround(1160.0f * scale_)),
                           static_cast<LONG>(std::lround(720.0f * scale_))};
            AdjustWindowRectEx(&minClient, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE, 0);
            info->ptMinTrackSize.x = minClient.right - minClient.left;
            info->ptMinTrackSize.y = minClient.bottom - minClient.top;
            return 0;
        }
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(GET_X_LPARAM(lParam)) / scale_;
            float y = static_cast<float>(GET_Y_LPARAM(lParam)) / scale_;
            OnMouseMove(x, y);
            return 0;
        }
        case WM_MOUSELEAVE:
            trackingMouse_ = false;
            OnMouseLeave();
            return 0;
        case WM_LBUTTONDOWN: {
            float x = static_cast<float>(GET_X_LPARAM(lParam)) / scale_;
            float y = static_cast<float>(GET_Y_LPARAM(lParam)) / scale_;
            OnMouseDown(x, y);
            return 0;
        }
        case WM_LBUTTONUP: {
            float x = static_cast<float>(GET_X_LPARAM(lParam)) / scale_;
            float y = static_cast<float>(GET_Y_LPARAM(lParam)) / scale_;
            OnMouseUp(x, y);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &point);
            OnMouseWheel(static_cast<float>(point.x) / scale_,
                         static_cast<float>(point.y) / scale_,
                         GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int notification = HIWORD(wParam);
            if (notification == EN_CHANGE) {
                if (id == ID_SEARCH) {
                    SyncSearchFromEdit();
                } else if (id == ID_API_BASE || id == ID_API_MODEL || id == ID_API_KEY) {
                    if (!testRunning_) {
                        testHasResult_ = false;
                        testStatus_.clear();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (id == ID_PAPER_TITLE || id == ID_PAPER_MINUTES || id == ID_PAPER_NOTES) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            } else if (notification == EN_SETFOCUS || notification == EN_KILLFOCUS) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            if (darkMode_) {
                SetTextColor(dc, RGB(232, 236, 245));
                SetBkColor(dc, RGB(21, 27, 38));
                return reinterpret_cast<LRESULT>(darkFieldBrush_);
            }
            SetTextColor(dc, RGB(31, 38, 54));
            SetBkColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(whiteBrush_);
        }
        case WM_TIMER:
            if (wParam == kToastTimer) {
                HideToast();
            } else if (wParam == 9) {
                if (imageExtracting_) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else {
                    KillTimer(hwnd, 9);
                }
            }
            return 0;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_KEYDOWN: {
            bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (control && wParam == 'N' && page_ == Page::Bank) {
                OpenQuestionEditor({});
                return 0;
            }
            if (control && wParam == 'F') {
                SwitchPage(Page::Bank);
                SetFocus(searchEdit_);
                SendMessageW(searchEdit_, EM_SETSEL, 0, -1);
                return 0;
            }
            if (control && wParam >= '1' && wParam <= '4') {
                int index = static_cast<int>(wParam - '1');
                Page pages[] = {Page::Bank, Page::Tags, Page::Ai, Page::Paper};
                SwitchPage(pages[index]);
                return 0;
            }
            return 0;
        }
        case kMsgAiUpdate:
            HandleAiUpdate(wParam, lParam);
            return 0;
        case kMsgAiFinished:
            HandleAiFinished(wParam, lParam);
            return 0;
        case kMsgTestFinished:
            HandleTestFinished(wParam, lParam);
            return 0;
        case kMsgImageFinished:
            HandleImageFinished(wParam, lParam);
            return 0;
        case WM_CLOSE: {
            if (aiRunning_ || testRunning_ || imageExtracting_) {
                int answer = MessageBoxW(hwnd, L"AI 请求仍在进行中，退出会丢弃尚未完成的结果。仍然退出吗？",
                                         L"数研组卷", MB_YESNO | MB_ICONQUESTION);
                if (answer != IDYES) return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
            if (aiThread_.joinable()) aiThread_.detach();
            if (testThread_.joinable()) testThread_.detach();
            if (imageThread_.joinable()) imageThread_.detach();
            KillTimer(hwnd, kToastTimer);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void App::CreateFontsAndBrushes() {
    whiteBrush_ = CreateSolidBrush(RGB(255, 255, 255));
    darkFieldBrush_ = CreateSolidBrush(RGB(21, 27, 38));
    canvasBrush_ = CreateSolidBrush(RGB(245, 246, 250));
    int fontHeight = -static_cast<int>(std::lround(14.0f * scale_));
    editFont_ = CreateFontW(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
}

void App::ApplyNativeTheme() {
    HWND edits[] = {searchEdit_, apiBaseEdit_, apiModelEdit_, apiKeyEdit_, paperTitleEdit_,
                    paperMinutesEdit_, paperNotesEdit_};
    for (HWND edit : edits) {
        if (!edit) continue;
        SetWindowTheme(edit, darkMode_ ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        RedrawWindow(edit, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);
    }
}

void App::ApplyTheme() {
    theme_.SetDark(darkMode_);
    col::Apply(darkMode_);
    if (hwnd_) {
        ui::ApplyWindowTheme(hwnd_, darkMode_);
        ApplyNativeTheme();
    }
}

void App::SetDarkMode(bool dark) {
    if (darkMode_ == dark) return;
    darkMode_ = dark;
    ApplyTheme();
    store_.Config().darkMode = dark;
    std::string error;
    if (!store_.SaveSettings(&error)) {
        ShowToast(L"夜间模式已切换，但偏好保存失败：" + W(error), true);
    } else {
        ShowToast(dark ? L"已切换到夜间模式。" : L"已切换到日间模式。");
    }
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void App::CreateEditControls() {
    auto createSingle = [&](int id, const wchar_t* cue, bool password) -> HWND {
        DWORD style = WS_CHILD | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP;
        if (password) style |= ES_PASSWORD;
        HWND edit = CreateWindowExW(0, L"EDIT", L"", style, 0, 0, 10, 10, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_,
                                    nullptr);
        if (!edit) return nullptr;
        SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_), TRUE);
        SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                     MAKELPARAM(2, 2));
        SendMessageW(edit, EM_SETLIMITTEXT, 12000, 0);
        if (cue) SendMessageW(edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cue));
        if (password) SendMessageW(edit, EM_SETPASSWORDCHAR, 0x25CF, 0);
        ShowWindow(edit, SW_HIDE);
        return edit;
    };
    auto createMultiline = [&](int id, bool scrollbar) -> HWND {
        DWORD style = WS_CHILD | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL |
                      WS_TABSTOP;
        if (scrollbar) style |= WS_VSCROLL;
        HWND edit = CreateWindowExW(0, L"EDIT", L"", style, 0, 0, 10, 10, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_,
                                    nullptr);
        if (!edit) return nullptr;
        SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_), TRUE);
        SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                     MAKELPARAM(4, 4));
        SendMessageW(edit, EM_SETLIMITTEXT, 40000, 0);
        ShowWindow(edit, SW_HIDE);
        return edit;
    };

    searchEdit_ = createSingle(ID_SEARCH, L"搜索题干、公式或知识点…", false);
    apiBaseEdit_ = createSingle(ID_API_BASE, L"https://api.deepseek.com", false);
    apiModelEdit_ = createSingle(ID_API_MODEL, L"例如 deepseek-chat", false);
    apiKeyEdit_ = createSingle(ID_API_KEY, L"sk-...（本地模型可留空）", true);
    paperTitleEdit_ = createSingle(ID_PAPER_TITLE, L"输入试卷标题", false);
    paperMinutesEdit_ = createSingle(ID_PAPER_MINUTES, L"120", false);
    paperNotesEdit_ = createMultiline(ID_PAPER_NOTES, true);
}

void App::PositionSingleLineEdit(HWND edit, const ui::Rect& field, float horizontalInset) {
    if (!edit) return;
    float editHeight = std::min(28.0f, std::max(20.0f, field.h - 8.0f));
    ui::Rect rect{field.x + horizontalInset, field.y + (field.h - editHeight) * 0.5f,
                  std::max(10.0f, field.w - horizontalInset * 2.0f), editHeight};
    SetWindowPos(edit, nullptr, static_cast<int>(std::lround(rect.x * scale_)),
                 static_cast<int>(std::lround(rect.y * scale_)),
                 static_cast<int>(std::lround(rect.w * scale_)),
                 static_cast<int>(std::lround(rect.h * scale_)),
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void App::PositionMultilineEdit(HWND edit, const ui::Rect& field, float inset) {
    if (!edit) return;
    ui::Rect rect{field.x + inset, field.y + inset, std::max(10.0f, field.w - inset * 2.0f),
                  std::max(20.0f, field.h - inset * 2.0f)};
    SetWindowPos(edit, nullptr, static_cast<int>(std::lround(rect.x * scale_)),
                 static_cast<int>(std::lround(rect.y * scale_)),
                 static_cast<int>(std::lround(rect.w * scale_)),
                 static_cast<int>(std::lround(rect.h * scale_)),
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void App::LayoutEditControls() {
    if (!hwnd_) return;
    RECT client{};
    GetClientRect(hwnd_, &client);
    if (client.right <= 0 || client.bottom <= 0) return;
    const float width = static_cast<float>(client.right) / scale_;
    const float height = static_cast<float>(client.bottom) / scale_;
    const BankRects bank = ComputeBankRects(width, height, !selectedIds_.empty());
    const AiRects ai = ComputeAiRects(width, height);

    HWND focused = GetFocus();

    auto setVisible = [&](HWND edit, bool show) {
        if (!edit) return;
        if (show) {
            ShowWindow(edit, SW_SHOWNA);
        } else {
            ShowWindow(edit, SW_HIDE);
        }
    };

    const bool showSearch = page_ == Page::Bank;
    searchFieldRect_ = bank.search;
    if (showSearch) {
        PositionSingleLineEdit(searchEdit_,
                               ui::Rect{bank.search.x + 34.0f, bank.search.y + 4.0f,
                                        bank.search.w - 44.0f, bank.search.h - 8.0f},
                               2.0f);
    }
    setVisible(searchEdit_, showSearch);

    const bool showAi = page_ == Page::Ai;
    if (showAi) {
        apiBaseFieldRect_ = ai.baseField;
        apiModelFieldRect_ = ai.modelField;
        apiKeyFieldRect_ = ai.keyField;
        PositionSingleLineEdit(apiBaseEdit_, ai.baseField, 13.0f);
        PositionSingleLineEdit(apiModelEdit_, ai.modelField, 13.0f);
        PositionSingleLineEdit(apiKeyEdit_,
                               ui::Rect{ai.keyField.x, ai.keyField.y, ai.keyField.w - 68.0f,
                                        ai.keyField.h},
                               13.0f);
    }
    setVisible(apiBaseEdit_, showAi);
    setVisible(apiModelEdit_, showAi);
    setVisible(apiKeyEdit_, showAi);

    const bool showPaper = page_ == Page::Paper && !selectedIds_.empty();
    if (showPaper) {
        const PaperRects paper = ComputePaperRects(width, height);
        paperTitleFieldRect_ = paper.titleField;
        paperMinutesFieldRect_ = paper.minutesField;
        paperNotesFieldRect_ = paper.notesField;
        PositionSingleLineEdit(paperTitleEdit_, paper.titleField, 13.0f);
        PositionSingleLineEdit(paperMinutesEdit_, paper.minutesField, 13.0f);
        PositionMultilineEdit(paperNotesEdit_, paper.notesField, 8.0f);
    }
    setVisible(paperTitleEdit_, showPaper);
    setVisible(paperMinutesEdit_, showPaper);
    setVisible(paperNotesEdit_, showPaper);

    if (focused && !IsWindowVisible(focused) && focused != hwnd_) {
        SetFocus(hwnd_);
    }
    if (focused == searchEdit_ && !showSearch) SetFocus(hwnd_);
}

void App::ReadSettingsIntoFields() {
    const Settings& settings = store_.Config();
    SetControlText(apiBaseEdit_, W(settings.apiBase));
    SetControlText(apiModelEdit_, W(settings.model));
    SetControlText(apiKeyEdit_, W(settings.apiKey));
    apiKeyVisible_ = false;
    if (apiKeyEdit_) {
        SendMessageW(apiKeyEdit_, EM_SETPASSWORDCHAR, 0x25CF, 0);
    }
    initialFieldsLoaded_ = true;
}

void App::ReadPaperDefaults() {
    std::wstring year = util::FormatLocalTime(L"%Y");
    if (year.empty()) year = L"2026";
    SetControlText(paperTitleEdit_, year + L" 届高三数学模拟试卷");
    SetControlText(paperMinutesEdit_, L"120");
    SetControlText(paperNotesEdit_,
                   L"1．答题前，请将自己的姓名、班级填写在试卷指定位置。\n"
                   L"2．回答选择题时，选出每小题答案后，用铅笔把答题卡上对应题目的答案标号涂黑。\n"
                   L"3．考试结束后，将本试卷和答题卡一并交回。");
    paperFieldsReady_ = true;
}

void App::EnsurePaperFields() {
    if (!paperFieldsReady_) ReadPaperDefaults();
}

void App::SwitchPage(Page page) {
    if (page_ == page) return;
    if (page_ == Page::Ai && initialFieldsLoaded_) {
        SaveAiSettings(false);
    }
    page_ = page;
    if (page == Page::Paper) EnsurePaperFields();
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::RebuildFiltered() {
    visibleIds_.clear();
    for (const auto& question : store_.Questions()) {
        if (filter_.Matches(question)) {
            visibleIds_.push_back(question.id);
        }
    }
}

void App::SyncSearchFromEdit() {
    if (!searchEdit_) return;
    filter_.search = util::ToUtf8(GetControlText(searchEdit_));
    RebuildFiltered();
    bankScroll_.offset = 0.0f;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ClearFilter() {
    filter_.Reset();
    if (searchEdit_) {
        std::wstring current = GetControlText(searchEdit_);
        if (!current.empty()) {
            SetControlText(searchEdit_, L"");
        }
    }
    bankScroll_.offset = 0.0f;
    filterTagScroll_.offset = 0.0f;
    RebuildFiltered();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ToggleTagFilter(const std::string& tag) {
    if (filter_.tags.count(tag)) {
        filter_.tags.erase(tag);
    } else {
        filter_.tags.insert(tag);
    }
    filterTagScroll_.offset = 0.0f;
    bankScroll_.offset = 0.0f;
    RebuildFiltered();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ApplyTagFromLibrary(const std::string& tag) {
    filter_.Reset();
    filter_.tags.insert(tag);
    if (searchEdit_) {
        SetControlText(searchEdit_, L"");
    }
    SwitchPage(Page::Bank);
    RebuildFiltered();
    bankScroll_.offset = 0.0f;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

App::GroupCounts App::CurrentSelectionCounts() const {
    GroupCounts counts;
    for (const auto& id : selectedIds_) {
        const Question* question = store_.FindQuestion(id);
        if (!question) continue;
        counts.points += PointsForType(question->type);
        switch (question->type) {
            case QuestionType::Choice: ++counts.choice; break;
            case QuestionType::Fill: ++counts.fill; break;
            case QuestionType::Solution: ++counts.solution; break;
        }
    }
    return counts;
}

void App::SaveQuestionFromEditor(const Question& question, const std::string& originalId) {
    Question saved = question;
    saved.stem = NormalizeMathText(util::Trim(saved.stem));
    for (auto& option : saved.options) {
        option = NormalizeMathText(util::Trim(option));
    }
    saved.answer = NormalizeMathText(util::Trim(saved.answer));
    saved.analysis = NormalizeMathText(util::Trim(saved.analysis));
    if (saved.stem.empty()) {
        ShowToast(L"题干不能为空。", true);
        return;
    }
    if (saved.id.empty()) saved.id = util::NewId("q");
    if (saved.created.empty()) saved.created = util::NowIso8601();
    if (saved.type == QuestionType::Choice) {
        while (saved.options.size() < 4) saved.options.emplace_back();
        if (saved.options.size() > 4) saved.options.resize(4);
    }
    saved.tagged = saved.HasAnyTag();

    if (originalId.empty()) {
        store_.Questions().push_back(saved);
        selectedIds_.insert(saved.id);
    } else {
        auto& questions = store_.Questions();
        auto it = std::find_if(questions.begin(), questions.end(),
                               [&](const Question& item) { return item.id == originalId; });
        if (it == questions.end()) {
            ShowToast(L"没有找到要修改的题目。", true);
            return;
        }
        *it = saved;
    }
    store_.MergeQuestionTags(saved);
    std::string error;
    bool ok = store_.SaveQuestions(&error);
    ok = store_.SaveTags(&error) && ok;
    RebuildFiltered();
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
    if (!ok) {
        ShowToast(W(error), true);
    } else {
        ShowToast(originalId.empty() ? L"题目已添加，并加入组卷篮。" : L"题目已保存。");
    }
}

void App::OpenQuestionEditor(const std::string& questionId) {
    if (imageExtracting_) {
        ShowToast(L"图片识别完成后会打开编辑器，请稍候。", true);
        return;
    }
    Question existing;
    const Question* existingPtr = nullptr;
    const std::string* originalId = nullptr;
    if (!questionId.empty()) {
        const Question* found = store_.FindQuestion(questionId);
        if (!found) {
            ShowToast(L"没有找到这道题。", true);
            return;
        }
        existing = *found;
        existingPtr = &existing;
        originalId = &questionId;
    }
    QuestionEditorResult result =
        ShowQuestionEditor(hwnd_, theme_, store_.Tags(), existingPtr);
    if (result.action == QuestionEditorAction::Save) {
        SaveQuestionFromEditor(result.question, originalId ? *originalId : std::string{});
    } else if (result.action == QuestionEditorAction::Delete && originalId) {
        DeleteQuestion(*originalId);
    }
}

void App::DeleteQuestion(const std::string& questionId) {
    auto& questions = store_.Questions();
    auto it = std::find_if(questions.begin(), questions.end(),
                           [&](const Question& item) { return item.id == questionId; });
    if (it == questions.end()) return;
    questions.erase(it);
    selectedIds_.erase(questionId);
    std::string error;
    bool ok = store_.SaveQuestions(&error);
    RebuildFiltered();
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
    ShowToast(ok ? L"题目已删除。" : W(error), !ok);
}

void App::AddHit(const ui::Rect& rect, std::wstring action, std::wstring payload) {
    HitTarget target;
    target.rect = rect;
    target.action = std::move(action);
    target.payload = std::move(payload);
    hits_.push_back(std::move(target));
}

std::wstring App::MakeKey(const std::wstring& action, const std::wstring& payload) const {
    return HitKey(action, payload);
}

bool App::IsHovered(const std::wstring& action, const std::wstring& payload) const {
    return hoverKey_ == MakeKey(action, payload);
}

const App::HitTarget* App::HitTest(float x, float y) const {
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (it->rect.Contains(x, y)) return &*it;
    }
    return nullptr;
}

void App::OnMouseMove(float x, float y) {
    if (!trackingMouse_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        TrackMouseEvent(&track);
        trackingMouse_ = true;
    }
    if (draggingScroll_) {
        float travel = std::max(1.0f, draggingScroll_->track.h - draggingScroll_->thumb.h);
        float delta = (y - dragStartY_) / travel * draggingScroll_->maxOffset;
        draggingScroll_->offset = std::clamp(dragStartOffset_ + delta, 0.0f,
                                             draggingScroll_->maxOffset);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    std::wstring newHover;
    const HitTarget* hit = HitTest(x, y);
    if (hit) {
        newHover = MakeKey(hit->action, hit->payload);
        SetCursor(LoadCursorW(nullptr, IDC_HAND));
    } else {
        bool overScroll = false;
        std::vector<ScrollState*> scrolls;
        if (page_ == Page::Bank) {
            scrolls = {&bankScroll_, &filterTagScroll_};
        } else if (page_ == Page::Tags) {
            scrolls = {&formulaScroll_, &conceptScroll_};
        }
        for (ScrollState* scroll : scrolls) {
            if (scroll->maxOffset > 0.0f &&
                (scroll->thumb.Contains(x, y) || scroll->track.Contains(x, y))) {
                overScroll = true;
                break;
            }
        }
        SetCursor(LoadCursorW(nullptr, overScroll ? IDC_HAND : IDC_ARROW));
    }
    if (newHover != hoverKey_) {
        hoverKey_ = std::move(newHover);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void App::OnMouseLeave() {
    if (!hoverKey_.empty()) {
        hoverKey_.clear();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
}

void App::OnMouseDown(float x, float y) {
    if (!draggingScroll_) {
        std::vector<ScrollState*> scrolls;
        if (page_ == Page::Bank) {
            scrolls = {&bankScroll_, &filterTagScroll_};
        } else if (page_ == Page::Tags) {
            scrolls = {&formulaScroll_, &conceptScroll_};
        }
        for (ScrollState* scroll : scrolls) {
            if (scroll->maxOffset <= 0.0f) continue;
            if (scroll->thumb.Contains(x, y)) {
                draggingScroll_ = scroll;
                dragStartY_ = y;
                dragStartOffset_ = scroll->offset;
                SetCapture(hwnd_);
                return;
            }
            if (scroll->track.Contains(x, y)) {
                if (y < scroll->thumb.y) {
                    scroll->offset -= scroll->viewport.h * 0.85f;
                } else {
                    scroll->offset += scroll->viewport.h * 0.85f;
                }
                scroll->offset = std::clamp(scroll->offset, 0.0f, scroll->maxOffset);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
        }
    }
    const HitTarget* hit = HitTest(x, y);
    pressedKey_ = hit ? MakeKey(hit->action, hit->payload) : std::wstring{};
    if (hit) {
        if (hit->action != L"focus") {
            SetFocus(hwnd_);
        }
        SetCapture(hwnd_);
    }
}

void App::OnMouseUp(float x, float y) {
    if (draggingScroll_) {
        draggingScroll_ = nullptr;
        ReleaseCapture();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (GetCapture() == hwnd_) ReleaseCapture();
    if (pressedKey_.empty()) return;
    const HitTarget* hit = HitTest(x, y);
    if (hit && MakeKey(hit->action, hit->payload) == pressedKey_) {
        HitTarget target = *hit;
        pressedKey_.clear();
        Activate(target);
        return;
    }
    pressedKey_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::OnMouseWheel(float x, float y, int delta) {
    ScrollState* scroll = nullptr;
    RECT client{};
    GetClientRect(hwnd_, &client);
    const float width = static_cast<float>(client.right) / scale_;
    const float height = static_cast<float>(client.bottom) / scale_;
    if (page_ == Page::Bank) {
        const BankRects bank = ComputeBankRects(width, height, !selectedIds_.empty());
        if (bank.filter.Contains(x, y)) {
            scroll = &filterTagScroll_;
        } else if (bank.listViewport.Contains(x, y)) {
            scroll = &bankScroll_;
        }
    } else if (page_ == Page::Tags) {
        const TagsRects tags = ComputeTagsRects(width, height);
        if (tags.formulaCard.Contains(x, y)) {
            scroll = &formulaScroll_;
        } else if (tags.conceptCard.Contains(x, y)) {
            scroll = &conceptScroll_;
        }
    }
    if (!scroll) return;
    float step = 62.0f * (delta > 0 ? -1.0f : 1.0f);
    if (std::abs(delta) > 120) step *= std::min(3.0f, std::abs(delta) / 120.0f);
    scroll->offset = std::clamp(scroll->offset + step, 0.0f, scroll->maxOffset);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ClampScroll(ScrollState& scroll) {
    scroll.offset = std::clamp(scroll.offset, 0.0f, std::max(0.0f, scroll.maxOffset));
}

void App::ShowToast(const std::wstring& text, bool error) {
    toastText_ = text;
    toastError_ = error;
    toastActive_ = true;
    KillTimer(hwnd_, kToastTimer);
    SetTimer(hwnd_, kToastTimer, error ? 5200 : 3400, nullptr);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::HideToast() {
    KillTimer(hwnd_, kToastTimer);
    toastActive_ = false;
    toastText_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::OpenPath(const std::filesystem::path& path) {
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        ShowToast(L"无法打开文件，请检查系统是否安装了可打开 .docx 的程序。", true);
    }
}

void App::OpenContainingFolder(const std::filesystem::path& path) {
    std::wstring arguments = L"/select,\"" + path.wstring() + L"\"";
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", L"explorer.exe", arguments.c_str(), nullptr,
                                     SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        ShowToast(L"无法打开文件所在文件夹。", true);
    }
}

void App::OnPaint() {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    if (!dc) return;
    RECT client{};
    GetClientRect(hwnd_, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) {
        EndPaint(hwnd_, &paint);
        return;
    }
    Gdiplus::Bitmap buffer(width, height, PixelFormat32bppRGB);
    Gdiplus::Graphics graphics(&buffer);
    graphics.Clear(col::Background);
    ui::Canvas canvas(graphics, theme_);
    hits_.clear();

    float logicalWidth = static_cast<float>(width) / scale_;
    float logicalHeight = static_cast<float>(height) / scale_;
    PaintSidebar(canvas, logicalWidth, logicalHeight);
    PaintHeader(canvas, logicalWidth, logicalHeight);
    switch (page_) {
        case Page::Bank: PaintBank(canvas, logicalWidth, logicalHeight); break;
        case Page::Tags: PaintTags(canvas, logicalWidth, logicalHeight); break;
        case Page::Ai: PaintAi(canvas, logicalWidth, logicalHeight); break;
        case Page::Paper: PaintPaper(canvas, logicalWidth, logicalHeight); break;
    }
    PaintToast(canvas, logicalWidth, logicalHeight);

    Gdiplus::Graphics target(dc);
    target.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    target.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    target.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    target.DrawImage(&buffer, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height,
                     Gdiplus::UnitPixel);
    EndPaint(hwnd_, &paint);
}

void App::PaintSidebar(ui::Canvas& canvas, float width, float height) {
    (void)width;
    canvas.FillRect(ui::Rect{0, 0, kSidebarWidth, height}, col::Card);
    canvas.Line(kSidebarWidth - 0.5f, 0, kSidebarWidth - 0.5f, height, col::Border, 1.0f);

    ui::Rect logoRect{22.0f, 26.0f, 40.0f, 40.0f};
    canvas.FillRect(logoRect, col::Accent, 12.0f);
    canvas.Text(L"∑", logoRect, 24.0f, col::OnAccent, true, Gdiplus::StringAlignmentCenter,
                Gdiplus::StringAlignmentCenter, false);
    canvas.Text(L"数研组卷", ui::Rect{72.0f, 25.0f, 132.0f, 25.0f}, 17.5f, col::Ink, true);
    canvas.Text(L"MATH PAPER STUDIO", ui::Rect{73.0f, 50.0f, 138.0f, 15.0f}, 8.5f, col::Faint,
                false);

    canvas.Text(L"工作台", ui::Rect{26.0f, 105.0f, 100.0f, 16.0f}, 9.5f, col::Faint, true);
    const wchar_t* labels[] = {L"题库管理", L"标签容器", L"AI 接口", L"试卷合成"};
    const wchar_t* keys[] = {L"bank", L"tags", L"ai", L"paper"};
    const int active = static_cast<int>(page_);
    for (int i = 0; i < 4; ++i) {
        ui::Rect item{14.0f, 128.0f + static_cast<float>(i) * 56.0f, 196.0f, 46.0f};
        bool selected = i == active;
        bool hovered = IsHovered(L"nav", keys[i]);
        if (selected) {
            canvas.FillRect(item, col::AccentSoft2, 12.0f);
            canvas.FillRect(ui::Rect{item.x, item.y + 13.0f, 3.0f, 20.0f}, col::Accent, 1.5f);
        } else if (hovered) {
            canvas.FillRect(item, col::Hover, 12.0f);
        }
        Gdiplus::Color iconColor = selected ? col::Accent : col::Muted;
        DrawNavIcon(canvas, i, ui::Rect{30.0f, item.y + 14.0f, 18.0f, 18.0f}, iconColor);
        canvas.Text(labels[i], ui::Rect{58.0f, item.y, 140.0f, item.h}, 13.5f,
                    selected ? col::Accent : col::InkSoft, selected);
        AddHit(item, L"nav", keys[i]);
    }

    const float modeRowY = height - 190.0f;
    ui::Rect modeRow{14.0f, modeRowY, 196.0f, 44.0f};
    const bool modeHover = IsHovered(L"dark_mode", L"");
    const Gdiplus::Color modeRowBackground = modeHover ? col::Hover : col::Card;
    if (modeHover) canvas.FillRect(modeRow, col::Hover, 10.0f);
    const Gdiplus::Color modeColor = darkMode_ ? col::Accent : col::Muted;
    canvas.Circle(32.0f, modeRowY + 17.0f, 6.5f, modeColor);
    canvas.Circle(35.5f, modeRowY + 14.0f, 5.8f, modeRowBackground);
    canvas.Text(L"夜间模式", ui::Rect{50.0f, modeRowY + 5.0f, 100.0f, 18.0f}, 11.5f,
                darkMode_ ? col::Accent : col::InkSoft, false);
    canvas.Text(L"深色外观", ui::Rect{50.0f, modeRowY + 23.0f, 100.0f, 14.0f}, 8.8f,
                col::Faint, false);
    ui::Rect switchTrack{158.0f, modeRowY + 13.0f, 36.0f, 18.0f};
    canvas.FillRect(switchTrack, darkMode_ ? col::Accent : col::DisabledFill, 9.0f);
    canvas.StrokeRect(switchTrack, darkMode_ ? col::Accent : col::BorderStrong, 9.0f, 1.0f);
    const float knobX = darkMode_ ? switchTrack.right() - 9.0f : switchTrack.x + 9.0f;
    canvas.Circle(knobX, switchTrack.y + 9.0f, 6.5f, col::OnAccent);
    AddHit(modeRow, L"dark_mode");

    const float bottomLineY = height - 124.0f;
    canvas.Line(20.0f, bottomLineY, kSidebarWidth - 20.0f, bottomLineY, col::Border, 1.0f);
    canvas.Circle(24.0f, height - 99.0f, 3.0f, col::Green);
    canvas.Text(L"本地数据 · 自动保存", ui::Rect{33.0f, height - 109.0f, 172.0f, 20.0f}, 10.5f,
                col::Muted, false);
    ui::Rect folderHit{18.0f, height - 86.0f, 188.0f, 32.0f};
    bool folderHover = IsHovered(L"open_data", L"");
    if (folderHover) canvas.FillRect(folderHit, col::Hover, 8.0f);
    canvas.Text(L"打开数据目录  ↗", ui::Rect{23.0f, height - 82.0f, 170.0f, 24.0f}, 11.5f,
                col::Accent, true);
    AddHit(folderHit, L"open_data");
    canvas.Text(L"v1.0  ·  C++ / Win32", ui::Rect{23.0f, height - 38.0f, 170.0f, 16.0f}, 9.0f,
                col::Faint, false);
}

void App::DrawNavIcon(ui::Canvas& canvas, int index, const ui::Rect& rect,
                      const Gdiplus::Color& color) {
    float x = rect.x;
    float y = rect.y;
    if (index == 0) {
        canvas.StrokeRect(ui::Rect{x + 2.0f, y + 1.0f, 14.0f, 16.0f}, color, 3.0f, 1.5f);
        canvas.Line(x + 5.5f, y + 6.0f, x + 12.5f, y + 6.0f, color, 1.4f);
        canvas.Line(x + 5.5f, y + 9.5f, x + 12.5f, y + 9.5f, color, 1.4f);
        canvas.Line(x + 5.5f, y + 13.0f, x + 10.0f, y + 13.0f, color, 1.4f);
    } else if (index == 1) {
        canvas.Line(x + 3.0f, y + 7.0f, x + 8.0f, y + 2.0f, color, 1.5f);
        canvas.Line(x + 8.0f, y + 2.0f, x + 16.0f, y + 2.0f, color, 1.5f);
        canvas.Line(x + 16.0f, y + 2.0f, x + 16.0f, y + 10.0f, color, 1.5f);
        canvas.Line(x + 16.0f, y + 10.0f, x + 11.0f, y + 15.0f, color, 1.5f);
        canvas.Line(x + 11.0f, y + 15.0f, x + 7.5f, y + 11.5f, color, 1.5f);
        canvas.CircleOutline(x + 11.5f, y + 6.5f, 2.0f, color, 1.3f);
    } else if (index == 2) {
        canvas.Line(x + 9.0f, y + 1.0f, x + 9.0f, y + 17.0f, color, 1.5f);
        canvas.Line(x + 1.0f, y + 9.0f, x + 17.0f, y + 9.0f, color, 1.5f);
        canvas.Line(x + 3.5f, y + 3.5f, x + 6.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 14.5f, y + 3.5f, x + 12.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 3.5f, y + 14.5f, x + 6.0f, y + 12.0f, color, 1.2f);
        canvas.Line(x + 14.5f, y + 14.5f, x + 12.0f, y + 12.0f, color, 1.2f);
        canvas.Circle(x + 9.0f, y + 9.0f, 1.3f, color);
    } else {
        canvas.StrokeRect(ui::Rect{x + 2.0f, y + 1.0f, 14.0f, 16.0f}, color, 3.0f, 1.5f);
        canvas.Line(x + 9.0f, y + 1.5f, x + 9.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 9.0f, y + 6.0f, x + 15.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 5.0f, y + 10.0f, x + 12.0f, y + 10.0f, color, 1.2f);
        canvas.Line(x + 5.0f, y + 13.0f, x + 10.0f, y + 13.0f, color, 1.2f);
    }
}

void App::DrawSearchIcon(ui::Canvas& canvas, float x, float y, float size,
                         const Gdiplus::Color& color) {
    float centerX = x + size * 0.43f;
    float centerY = y + size * 0.43f;
    float radius = size * 0.29f;
    canvas.CircleOutline(centerX, centerY, radius, color, 1.6f);
    canvas.Line(centerX + radius * 0.72f, centerY + radius * 0.72f, x + size - 1.0f,
                y + size - 1.0f, color, 1.8f);
}

void App::DrawSparkIcon(ui::Canvas& canvas, const ui::Rect& rect,
                        const Gdiplus::Color& color) {
    float cx = rect.x + rect.w * 0.5f;
    float cy = rect.y + rect.h * 0.5f;
    canvas.Triangle(cx, rect.y, cx + rect.w * 0.15f, cy - rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(cx, rect.y, cx - rect.w * 0.15f, cy - rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(cx + rect.w, cy, cx + rect.w * 0.85f, cy + rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(cx + rect.w, cy, cx + rect.w * 0.85f, cy - rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(cx, rect.bottom(), cx + rect.w * 0.15f, cy + rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(cx, rect.bottom(), cx - rect.w * 0.15f, cy + rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(rect.x, cy, cx - rect.w * 0.15f, cy + rect.h * 0.15f, cx, cy, color);
    canvas.Triangle(rect.x, cy, cx - rect.w * 0.15f, cy - rect.h * 0.15f, cx, cy, color);
}

void App::DrawDocumentIcon(ui::Canvas& canvas, const ui::Rect& rect,
                           const Gdiplus::Color& color) {
    canvas.StrokeRect(ui::Rect{rect.x + 1.0f, rect.y, rect.w - 2.0f, rect.h}, color, 2.0f, 1.3f);
    canvas.Line(rect.x + rect.w - 5.0f, rect.y + 1.0f, rect.x + rect.w - 5.0f,
                rect.y + 5.0f, color, 1.1f);
    canvas.Line(rect.x + rect.w - 5.0f, rect.y + 5.0f, rect.x + rect.w - 1.0f,
                rect.y + 5.0f, color, 1.1f);
    canvas.Line(rect.x + 4.0f, rect.y + 8.0f, rect.x + rect.w - 5.0f, rect.y + 8.0f, color, 1.1f);
    canvas.Line(rect.x + 4.0f, rect.y + 11.0f, rect.x + rect.w - 5.0f, rect.y + 11.0f, color, 1.1f);
}

void App::DrawFieldBackground(ui::Canvas& canvas, const ui::Rect& rect, HWND edit) {
    bool focused = edit && GetFocus() == edit;
    canvas.FillRect(rect, col::Card, 10.0f);
    canvas.StrokeRect(rect, focused ? col::Accent : col::BorderStrong, 10.0f,
                      focused ? 1.6f : 1.0f);
}

void App::PaintHeader(ui::Canvas& canvas, float width, float height) {
    std::wstring title;
    std::wstring subtitle;
    switch (page_) {
        case Page::Bank:
            title = L"数学题库";
            subtitle = L"筛选题目、AI 标记公式与知识点，再组成可打印试卷";
            break;
        case Page::Tags:
            title = L"标签容器";
            subtitle = L"公式与知识点分栏归档，点击任意标签即可反向筛选题目";
            break;
        case Page::Ai:
            title = L"AI 接口";
            subtitle = L"接入任意 OpenAI 兼容模型，自动为题目识别公式与知识点标签";
            break;
        case Page::Paper:
            title = L"试卷合成";
            subtitle = L"按标准数学试卷版式排版，导出可直接打印的 Word 文档";
            break;
    }
    float headerTextWidth = std::max(300.0f, width - 620.0f);
    if (page_ == Page::Bank) {
        const BankRects bank = ComputeBankRects(width, height, !selectedIds_.empty());
        headerTextWidth = std::max(220.0f, bank.aiButton.x - kPageX - 24.0f);
    }
    canvas.Text(title, ui::Rect{kPageX, 22.0f,
                                std::min(std::max(220.0f, width - 850.0f), headerTextWidth),
                                34.0f},
                26.0f, col::Ink, true);
    canvas.Text(subtitle, ui::Rect{kPageX, 62.0f, headerTextWidth, 20.0f}, 12.0f, col::Muted,
                false);

    if (page_ == Page::Bank) {
        const BankRects rects = ComputeBankRects(width, height, !selectedIds_.empty());
        DrawFieldBackground(canvas, rects.search, searchEdit_);
        DrawSearchIcon(canvas, rects.search.x + 14.0f, rects.search.y + 11.0f, 18.0f, col::Faint);
        AddHit(rects.search, L"focus", L"search");

        std::wstring aiLabel;
        bool aiEnabled = !aiRunning_ && !imageExtracting_ && !selectedIds_.empty();
        if (aiRunning_) {
            aiLabel = L"AI 标注中 " + IntText(aiDone_) + L"/" + IntText(aiTotal_);
        } else {
            aiLabel = L"AI 智能标注";
            if (!selectedIds_.empty()) aiLabel += L" (" + IntText(static_cast<int>(selectedIds_.size())) + L")";
        }
        DrawButton(canvas, rects.aiButton, aiLabel, L"ai_tag", L"", aiEnabled ? 5 : 2, aiEnabled,
                   12.5f);
        bool imageEnabled = !imageExtracting_ && !aiRunning_ && !testRunning_;
        DrawButton(canvas, rects.imageButton, imageExtracting_ ? L"识别图片中…" : L"图片识题",
                   L"image_question", L"", imageExtracting_ ? 2 : 1, imageEnabled, 12.5f);
        DrawButton(canvas, rects.addButton, L"＋ 新增题目", L"new_question", L"", 0,
                   !imageExtracting_, 12.5f);
    } else if (page_ == Page::Tags) {
        const TagsRects rects = ComputeTagsRects(width, height);
        DrawButton(canvas, rects.headerButton, L"去题库筛选", L"nav", L"bank", 1, true, 12.5f);
    } else if (page_ == Page::Ai) {
        ui::Rect badge{width - kRightMargin - 138.0f, 40.0f, 138.0f, 34.0f};
        canvas.FillRect(badge, col::GreenSoft, 17.0f);
        canvas.Circle(badge.x + 18.0f, badge.y + 17.0f, 3.5f, col::Green);
        canvas.Text(L"OpenAI 兼容", ui::Rect{badge.x + 28.0f, badge.y, badge.w - 34.0f, badge.h},
                    11.0f, col::Green, true);
    } else {
        ui::Rect badge{width - kRightMargin - 118.0f, 40.0f, 118.0f, 34.0f};
        canvas.FillRect(badge, col::AccentSoft, 17.0f);
        canvas.Text(L"已选 " + IntText(static_cast<int>(selectedIds_.size())) + L" 题",
                    ui::Rect{badge.x, badge.y, badge.w, badge.h}, 11.0f, col::Accent, true,
                    Gdiplus::StringAlignmentCenter);
    }
}

void App::DrawCheckboxRow(ui::Canvas& canvas, const ui::Rect& row, const std::wstring& label,
                          int count, bool checked, const std::wstring& action,
                          const std::wstring& payload, bool enabled) {
    bool hovered = enabled && IsHovered(action, payload);
    if (hovered) {
        canvas.FillRect(row, col::Hover, 7.0f);
    }
    ui::Rect box{row.x, row.y + (row.h - 16.0f) * 0.5f, 16.0f, 16.0f};
    canvas.Checkbox(box, checked, hovered, !enabled);
    canvas.Text(label, ui::Rect{row.x + 24.0f, row.y, row.w - 54.0f, row.h}, 12.0f,
                enabled ? col::InkSoft : col::Faint, false);
    if (count >= 0) {
        canvas.Text(IntText(count), ui::Rect{row.right() - 34.0f, row.y, 26.0f, row.h}, 10.5f,
                    col::Faint, false, Gdiplus::StringAlignmentFar);
    }
    if (enabled) {
        AddHit(row, action, payload);
    }
}

bool App::DrawButton(ui::Canvas& canvas, const ui::Rect& rect, const std::wstring& text,
                     const std::wstring& action, const std::wstring& payload, int style,
                     bool enabled, float fontSize) {
    std::wstring key = MakeKey(action, payload);
    bool hovered = enabled && hoverKey_ == key;
    bool pressed = enabled && pressedKey_ == key;
    Gdiplus::Color fill;
    Gdiplus::Color border(0, 0, 0, 0);
    Gdiplus::Color textColor = col::OnAccent;
    float borderWidth = 1.0f;
    if (!enabled) {
        fill = style == 2 || style == 6 ? Gdiplus::Color(0, 0, 0, 0) : col::DisabledFill;
        textColor = col::DisabledText;
        border = col::DisabledBorder;
    } else if (style == 0) {
        fill = (hovered || pressed) ? col::AccentDark : col::Accent;
        textColor = col::OnAccent;
    } else if (style == 1 || style == 2) {
        if (hovered) {
            fill = col::Hover;
            border = col::BorderStrong;
        } else {
            fill = col::Card;
            border = col::BorderStrong;
        }
        textColor = col::InkSoft;
        if (style == 2) {
            if (hovered) {
                fill = col::Hover;
                border = Gdiplus::Color(0, 0, 0, 0);
            } else {
                fill = Gdiplus::Color(0, 0, 0, 0);
                border = Gdiplus::Color(0, 0, 0, 0);
            }
        }
    } else if (style == 3) {
        fill = hovered ? col::LightButtonHover : col::OnAccent;
        textColor = col::OnLight;
    } else if (style == 4) {
        fill = hovered ? col::RedHover : col::Red;
        textColor = col::OnAccent;
    } else if (style == 5) {
        fill = hovered ? col::AccentSoftHover : col::AccentSoft;
        textColor = col::Accent;
    } else if (style == 6) {
        fill = hovered ? Gdiplus::Color(45, 255, 255, 255) : Gdiplus::Color(0, 0, 0, 0);
        textColor = hovered ? col::OnAccent : col::OnNavyMuted;
    } else {
        fill = col::Card;
        textColor = col::Ink;
    }
    if (pressed && style != 2 && style != 6) fill = ui::Alpha(col::Ink, 22);
    canvas.FillRect(rect, fill, 10.0f);
    if (border.GetA() > 0 && borderWidth > 0.0f) {
        canvas.StrokeRect(rect, border, 10.0f, borderWidth);
    }
    canvas.Text(text, rect, fontSize, textColor, style == 0 || style == 3,
                Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter, false);
    if (enabled && !action.empty()) AddHit(rect, action, payload);
    return true;
}

void App::DrawScrollbar(ui::Canvas& canvas, ScrollState& scroll, const ui::Rect& viewport,
                        float contentHeight) {
    scroll.viewport = viewport;
    scroll.track = ui::Rect{};
    scroll.thumb = ui::Rect{};
    if (contentHeight <= viewport.h + 1.0f) {
        scroll.maxOffset = 0.0f;
        scroll.offset = 0.0f;
        return;
    }
    scroll.maxOffset = contentHeight - viewport.h;
    scroll.offset = std::clamp(scroll.offset, 0.0f, scroll.maxOffset);
    float trackX = viewport.right() - 5.0f;
    float trackY = viewport.y + 2.0f;
    float trackH = viewport.h - 4.0f;
    scroll.track = ui::Rect{trackX - 2.0f, trackY, 9.0f, trackH};
    float thumbH = std::max(34.0f, viewport.h * viewport.h / contentHeight);
    thumbH = std::min(thumbH, trackH);
    float thumbY = trackY + (trackH - thumbH) * (scroll.offset / scroll.maxOffset);
    scroll.thumb = ui::Rect{trackX, thumbY, 5.0f, thumbH};
    canvas.FillRect(ui::Rect{trackX, trackY, 5.0f, trackH}, col::ScrollTrack, 2.5f);
    canvas.FillRect(scroll.thumb, col::ScrollThumb, 2.5f);
}

void App::DrawTagPill(ui::Canvas& canvas, const std::wstring& text, float x, float y,
                      const Gdiplus::Color& background, const Gdiplus::Color& foreground,
                      float maxWidth, float* measuredWidth) {
    if (text.empty()) {
        if (measuredWidth) *measuredWidth = 0.0f;
        return;
    }
    Gdiplus::SizeF size = canvas.Measure(text, 10.2f, false, false);
    float width = std::min(maxWidth, size.Width + 16.0f);
    width = std::max(width, 28.0f);
    ui::Rect pill{x, y, width, 20.0f};
    canvas.FillRect(pill, background, 10.0f);
    canvas.TextEllipsis(text, ui::Rect{pill.x + 8.0f, pill.y, pill.w - 16.0f, pill.h}, 10.2f,
                        foreground, false);
    if (measuredWidth) *measuredWidth = width;
}

void App::DrawSelectionBar(ui::Canvas& canvas, const ui::Rect& rect) {
    canvas.FillRect(rect, col::Navy, 16.0f);
    GroupCounts counts = CurrentSelectionCounts();
    int count = static_cast<int>(selectedIds_.size());
    canvas.Text(L"已选 " + IntText(count) + L" 道题", ui::Rect{rect.x + 22.0f, rect.y + 11.0f,
                                                              260.0f, 24.0f},
                14.5f, col::OnAccent, true);
    std::wstring summary = L"选择题 " + IntText(counts.choice) + L" · 填空题 " +
                           IntText(counts.fill) + L" · 解答题 " + IntText(counts.solution) +
                           L"　|　预计 " + IntText(counts.points) + L" 分";
    canvas.Text(summary, ui::Rect{rect.x + 22.0f, rect.y + 36.0f, rect.w - 280.0f, 20.0f},
                11.3f, col::OnNavyMuted, false);
    ui::Rect generate{rect.right() - 18.0f - 118.0f, rect.y + 13.0f, 118.0f, 38.0f};
    ui::Rect clear{generate.x - 8.0f - 54.0f, rect.y + 13.0f, 54.0f, 38.0f};
    DrawButton(canvas, clear, L"清空", L"clear_selection", L"", 6, true, 12.0f);
    DrawButton(canvas, generate, L"前往组卷 →", L"go_paper", L"", 3, true, 12.5f);
}

void App::PaintBank(ui::Canvas& canvas, float width, float height) {
    const bool hasSelection = !selectedIds_.empty();
    const BankRects rects = ComputeBankRects(width, height, hasSelection);
    const ui::Rect filterCard = rects.filter;
    const ui::Rect filterInner{filterCard.x + 18.0f, filterCard.y + 18.0f,
                               filterCard.w - 36.0f, filterCard.h - 36.0f};

    canvas.DrawShadow(filterCard, 16.0f);
    canvas.FillRect(filterCard, col::Card, 16.0f);
    canvas.StrokeRect(filterCard, col::Border, 16.0f, 1.0f);

    int difficultyCounts[3] = {0, 0, 0};
    int typeCounts[3] = {0, 0, 0};
    int untaggedCount = 0;
    for (const auto& question : store_.Questions()) {
        ++difficultyCounts[static_cast<int>(question.difficulty)];
        ++typeCounts[static_cast<int>(question.type)];
        if (!question.tagged) ++untaggedCount;
    }

    canvas.Text(L"筛选条件", ui::Rect{filterInner.x, filterInner.y + 2.0f, 150.0f, 22.0f},
                15.5f, col::Ink, true);
    bool resetHovered = IsHovered(L"reset_filter", L"");
    canvas.Text(L"重置", ui::Rect{filterInner.right() - 42.0f, filterInner.y + 5.0f, 42.0f, 18.0f},
                11.0f, filter_.IsDefault() ? col::Faint : (resetHovered ? col::AccentDark : col::Accent),
                true, Gdiplus::StringAlignmentFar);
    if (!filter_.IsDefault()) {
        AddHit(ui::Rect{filterInner.right() - 48.0f, filterInner.y + 1.0f, 48.0f, 26.0f},
               L"reset_filter");
    }
    std::wstring libraryLine = L"题库 " + IntText(static_cast<int>(store_.Questions().size())) +
                               L" 题  ·  A " + IntText(difficultyCounts[0]) + L"  B " +
                               IntText(difficultyCounts[1]) + L"  C " + IntText(difficultyCounts[2]);
    canvas.Text(libraryLine, ui::Rect{filterInner.x, filterInner.y + 25.0f,
                                      filterInner.w, 16.0f},
                10.2f, col::Faint, false);

    auto filterChip = [&](const ui::Rect& chip, const std::wstring& label, bool selected,
                          const std::wstring& action, const std::wstring& payload) {
        bool hovered = IsHovered(action, payload);
        Gdiplus::Color fill = selected ? col::AccentSoft : col::Card;
        Gdiplus::Color border = selected ? col::AccentBorder : col::Border;
        if (hovered && !selected) {
            fill = col::Hover;
            border = col::BorderStrong;
        }
        canvas.FillRect(chip, fill, 9.0f);
        canvas.StrokeRect(chip, border, 9.0f, selected ? 1.2f : 1.0f);
        canvas.Text(label, chip, 11.8f, selected ? col::Accent : col::InkSoft, selected,
                    Gdiplus::StringAlignmentCenter);
        AddHit(chip, action, payload);
    };

    canvas.Text(L"难度", ui::Rect{filterInner.x, filterInner.y + 61.0f, 80.0f, 16.0f}, 10.5f,
                col::Muted, true);
    const float chipGap = 7.0f;
    const float chipWidth = (filterInner.w - chipGap * 2.0f) / 3.0f;
    const wchar_t* difficultyLabels[] = {L"A 基础", L"B 提升", L"C 挑战"};
    for (int i = 0; i < 3; ++i) {
        ui::Rect chip{filterInner.x + static_cast<float>(i) * (chipWidth + chipGap),
                      filterInner.y + 82.0f, chipWidth, 32.0f};
        bool selected = filter_.difficulties[static_cast<size_t>(i)];
        filterChip(chip, difficultyLabels[i], selected, L"chip_diff", IntText(i));
    }

    canvas.Text(L"题型", ui::Rect{filterInner.x, filterInner.y + 128.0f, 80.0f, 16.0f}, 10.5f,
                col::Muted, true);
    const wchar_t* typeLabels[] = {L"选择题", L"填空题", L"解答题"};
    for (int i = 0; i < 3; ++i) {
        ui::Rect chip{filterInner.x + static_cast<float>(i) * (chipWidth + chipGap),
                      filterInner.y + 148.0f, chipWidth, 32.0f};
        bool selected = filter_.types[static_cast<size_t>(i)];
        filterChip(chip, typeLabels[i], selected, L"chip_type", IntText(i));
    }

    ui::Rect untaggedRow{filterInner.x, filterInner.y + 194.0f, filterInner.w, 26.0f};
    DrawCheckboxRow(canvas, untaggedRow, L"只看待标注", untaggedCount, filter_.onlyUntagged,
                    L"toggle_untagged", L"");
    canvas.Line(filterInner.x, filterInner.y + 232.0f, filterInner.right(),
                filterInner.y + 232.0f, col::Border, 1.0f);

    canvas.Text(L"标签筛选", ui::Rect{filterInner.x, filterInner.y + 246.0f, 120.0f, 18.0f},
                12.0f, col::InkSoft, true);
    ui::Rect matchButton{filterInner.right() - 68.0f, filterInner.y + 244.0f, 68.0f, 24.0f};
    DrawButton(canvas, matchButton, filter_.matchAllTags ? L"全部匹配" : L"任一匹配",
               L"tag_match_mode", L"", 5, true, 10.0f);

    auto sortedTags = [&](const std::vector<std::string>& source, bool formulas) {
        std::vector<std::string> tags = source;
        std::stable_sort(tags.begin(), tags.end(), [&](const std::string& a, const std::string& b) {
            int ua = formulas ? store_.FormulaUsage(a) : store_.ConceptUsage(a);
            int ub = formulas ? store_.FormulaUsage(b) : store_.ConceptUsage(b);
            if (ua != ub) return ua > ub;
            return a < b;
        });
        return tags;
    };
    const std::vector<std::string> formulaTags = sortedTags(store_.Tags().formulas, true);
    const std::vector<std::string> conceptTags = sortedTags(store_.Tags().concepts, false);

    const float tagViewportTop = filterInner.y + 276.0f;
    const float tagViewportBottom = filterCard.bottom() - 16.0f;
    ui::Rect tagViewport{filterInner.x, tagViewportTop, filterInner.w,
                         std::max(60.0f, tagViewportBottom - tagViewportTop)};
    float contentHeight = 0.0f;
    if (!formulaTags.empty()) {
        contentHeight += 24.0f + static_cast<float>(formulaTags.size()) * 25.0f + 8.0f;
    }
    if (!conceptTags.empty()) {
        contentHeight += 24.0f + static_cast<float>(conceptTags.size()) * 25.0f;
    }
    filterTagScroll_.maxOffset = std::max(0.0f, contentHeight - tagViewport.h);
    filterTagScroll_.offset = std::clamp(filterTagScroll_.offset, 0.0f,
                                         filterTagScroll_.maxOffset);

    canvas.PushClip(tagViewport);
    float tagY = tagViewport.y + 2.0f - filterTagScroll_.offset;
    auto drawGroupTitle = [&](const std::wstring& title, int count, const Gdiplus::Color& color) {
        ui::Rect titleRect{filterInner.x, tagY, filterInner.w - 10.0f, 22.0f};
        canvas.Text(title, titleRect, 10.5f, color, true);
        canvas.Text(IntText(count), ui::Rect{titleRect.x, titleRect.y, titleRect.w, titleRect.h},
                    9.5f, col::Faint, false, Gdiplus::StringAlignmentFar);
        tagY += 23.0f;
    };
    auto drawTagRow = [&](const std::string& tag, int count) {
        ui::Rect row{filterInner.x, tagY, filterInner.w - 9.0f, 24.0f};
        bool interactive = row.y >= tagViewport.y - 1.0f && row.bottom() <= tagViewport.bottom() + 1.0f;
        bool hovered = interactive && IsHovered(L"filter_tag", W(tag));
        if (hovered) canvas.FillRect(row, col::Hover, 6.0f);
        ui::Rect box{row.x, row.y + 4.5f, 15.0f, 15.0f};
        bool checked = filter_.tags.count(tag) > 0;
        canvas.Checkbox(box, checked, hovered, false);
        canvas.Text(W(tag), ui::Rect{row.x + 23.0f, row.y, row.w - 60.0f, row.h}, 11.3f,
                    checked ? col::Ink : col::InkSoft, checked);
        canvas.Text(IntText(count), ui::Rect{row.right() - 32.0f, row.y, 28.0f, row.h}, 9.8f,
                    col::Faint, false, Gdiplus::StringAlignmentFar);
        if (interactive) AddHit(row, L"filter_tag", W(tag));
        tagY += 25.0f;
    };
    if (formulaTags.empty() && conceptTags.empty()) {
        canvas.Text(L"AI 标注后，公式与知识点会归档到这里。", ui::Rect{
                    filterInner.x, tagY + 8.0f, filterInner.w - 8.0f, 38.0f}, 11.0f,
                    col::Faint, false);
    } else {
        if (!formulaTags.empty()) {
            drawGroupTitle(L"公式标签", static_cast<int>(formulaTags.size()), col::TagText);
            for (const auto& tag : formulaTags) {
                drawTagRow(tag, store_.FormulaUsage(tag));
            }
            tagY += 8.0f;
        }
        if (!conceptTags.empty()) {
            drawGroupTitle(L"知识点标签", static_cast<int>(conceptTags.size()),
                           col::ConceptTagText);
            for (const auto& tag : conceptTags) {
                drawTagRow(tag, store_.ConceptUsage(tag));
            }
        }
    }
    canvas.PopClip();
    DrawScrollbar(canvas, filterTagScroll_, tagViewport, contentHeight);

    // Results header
    bool allVisibleSelected = !visibleIds_.empty();
    for (const auto& id : visibleIds_) {
        if (!selectedIds_.count(id)) {
            allVisibleSelected = false;
            break;
        }
    }
    canvas.Text(L"筛选结果", ui::Rect{rects.listHeader.x, rects.listHeader.y + 8.0f, 100.0f, 22.0f},
                13.5f, col::Ink, true);
    canvas.Text(L"共 " + IntText(static_cast<int>(visibleIds_.size())) + L" 道",
                ui::Rect{rects.listHeader.x + 72.0f, rects.listHeader.y + 10.0f, 120.0f, 20.0f},
                10.8f, col::Muted, false);
    if (aiRunning_) {
        canvas.Text(L"AI 正在分析 " + IntText(aiDone_) + L"/" + IntText(aiTotal_),
                    ui::Rect{rects.listHeader.x + 168.0f, rects.listHeader.y + 10.0f, 220.0f,
                             20.0f},
                    10.5f, col::Accent, true);
        float progress = aiTotal_ > 0 ? static_cast<float>(aiDone_) / aiTotal_ : 0.0f;
        float barWidth = rects.listHeader.w;
        canvas.FillRect(ui::Rect{rects.listHeader.x, rects.listHeader.bottom() - 1.0f, barWidth,
                                 2.0f}, col::Border, 1.0f);
        canvas.FillRect(ui::Rect{rects.listHeader.x, rects.listHeader.bottom() - 1.0f,
                                 barWidth * std::clamp(progress, 0.0f, 1.0f), 2.0f},
                        col::Accent, 1.0f);
    } else if (imageExtracting_) {
        std::wstring progress = imageBatchCount_ > 1
                                    ? L"AI 正在读取 " + IntText(imageBatchCount_) +
                                          L" 张图片并提取所有题目…"
                                    : L"AI 正在读取图片并提取所有题目…";
        canvas.Text(progress,
                    ui::Rect{rects.listHeader.x + 168.0f, rects.listHeader.y + 10.0f, 360.0f,
                             20.0f},
                    10.5f, col::Accent, true);
        canvas.FillRect(ui::Rect{rects.listHeader.x, rects.listHeader.bottom() - 1.0f,
                                 rects.listHeader.w, 2.0f},
                        col::Border, 1.0f);
        float animated = 0.18f + 0.25f * (1.0f + std::sin(static_cast<float>(GetTickCount64() % 1600) /
                                                         1600.0f * 6.28318f)) *
                                     0.5f;
        canvas.FillRect(ui::Rect{rects.listHeader.x + rects.listHeader.w * 0.18f,
                                 rects.listHeader.bottom() - 1.0f,
                                 rects.listHeader.w * animated, 2.0f},
                        col::Accent, 1.0f);
    }
    {
        Gdiplus::SizeF selectSize = canvas.Measure(L"全选当前结果", 11.0f, false, false);
        float totalWidth = 16.0f + 8.0f + selectSize.Width;
        float startX = rects.listHeader.right() - totalWidth;
        ui::Rect selectRect{startX, rects.listHeader.y + 5.0f, totalWidth, 26.0f};
        bool hovered = !visibleIds_.empty() && IsHovered(L"select_all", L"");
        if (hovered) canvas.FillRect(selectRect, col::Hover, 7.0f);
        canvas.Checkbox(ui::Rect{selectRect.x, selectRect.y + 5.0f, 16.0f, 16.0f},
                        allVisibleSelected, hovered, visibleIds_.empty());
        canvas.Text(L"全选当前结果", ui::Rect{selectRect.x + 24.0f, selectRect.y,
                                              selectRect.w - 24.0f, selectRect.h},
                    11.0f, visibleIds_.empty() ? col::Faint : col::Muted, false);
        if (!visibleIds_.empty()) AddHit(selectRect, L"select_all");
    }

    const float rowHeight = 108.0f;
    const float rowGap = 10.0f;
    float contentBottom = 0.0f;
    if (!visibleIds_.empty()) {
        contentBottom = static_cast<float>(visibleIds_.size()) * (rowHeight + rowGap) - rowGap;
    }
    bankScroll_.maxOffset = std::max(0.0f, contentBottom - rects.listViewport.h);
    bankScroll_.offset = std::clamp(bankScroll_.offset, 0.0f, bankScroll_.maxOffset);

    if (visibleIds_.empty()) {
        float centerY = rects.listViewport.y + rects.listViewport.h * 0.5f - 50.0f;
        ui::Rect iconRect{rects.listViewport.x + rects.listViewport.w * 0.5f - 26.0f,
                          centerY - 34.0f, 52.0f, 52.0f};
        canvas.FillRect(iconRect, col::AccentSoft, 26.0f);
        DrawSearchIcon(canvas, iconRect.x + 16.0f, iconRect.y + 16.0f, 20.0f, col::Accent);
        canvas.Text(filter_.IsDefault() ? L"题库里还没有题目" : L"没有找到匹配的题目",
                    ui::Rect{rects.listViewport.x, centerY + 32.0f, rects.listViewport.w, 24.0f},
                    14.0f, col::Ink, true, Gdiplus::StringAlignmentCenter);
        canvas.Text(filter_.IsDefault() ? L"点击右上角“新增题目”开始建立题库"
                                         : L"调整左侧筛选条件，或重置筛选后再试",
                    ui::Rect{rects.listViewport.x, centerY + 58.0f, rects.listViewport.w, 20.0f},
                    11.5f, col::Muted, false, Gdiplus::StringAlignmentCenter);
        if (!filter_.IsDefault()) {
            ui::Rect resetButton{rects.listViewport.x + rects.listViewport.w * 0.5f - 52.0f,
                                 centerY + 92.0f, 104.0f, 34.0f};
            DrawButton(canvas, resetButton, L"重置筛选", L"reset_filter", L"", 5, true, 11.5f);
        }
    } else {
        const float rowWidth = rects.listViewport.w - 8.0f;
        canvas.PushClip(rects.listViewport);
        for (size_t i = 0; i < visibleIds_.size(); ++i) {
            float rowY = rects.listViewport.y + static_cast<float>(i) * (rowHeight + rowGap) -
                         bankScroll_.offset;
            if (rowY + rowHeight < rects.listViewport.y - 2.0f ||
                rowY > rects.listViewport.bottom() + 2.0f) {
                continue;
            }
            const Question* question = store_.FindQuestion(visibleIds_[i]);
            if (!question) continue;
            bool interactive = rowY >= rects.listViewport.y - 1.0f &&
                               rowY + rowHeight <= rects.listViewport.bottom() + 1.0f;
            ui::Rect row{rects.listViewport.x, rowY, rowWidth, rowHeight};
            DrawQuestionRow(canvas, *question, row, static_cast<int>(i) + 1,
                            selectedIds_.count(question->id) > 0, interactive);
        }
        canvas.PopClip();
        DrawScrollbar(canvas, bankScroll_, rects.listViewport, contentBottom);
    }

    if (hasSelection) {
        DrawSelectionBar(canvas, rects.selectionBar);
    }
}

void App::DrawQuestionRow(ui::Canvas& canvas, const Question& question, const ui::Rect& rect,
                          int displayIndex, bool checked, bool interactive) {
    bool rowHovered = interactive && IsHovered(L"row", W(question.id));
    bool editHovered = interactive && IsHovered(L"edit_question", W(question.id));
    Gdiplus::Color fill = checked ? col::AccentSoft2 : col::Card;
    Gdiplus::Color border = checked ? col::AccentBorder : col::Border;
    if ((rowHovered || editHovered) && !checked) fill = col::CardAlt;
    if (rowHovered && checked) fill = col::AccentRowHover;
    canvas.FillRect(rect, fill, 14.0f);
    canvas.StrokeRect(rect, border, 14.0f, checked ? 1.3f : 1.0f);
    if (interactive) AddHit(rect, L"row", W(question.id));

    ui::Rect checkRect{rect.x + 18.0f, rect.y + 20.0f, 18.0f, 18.0f};
    canvas.Checkbox(checkRect, checked, rowHovered);
    canvas.Text(PaddedNumber(displayIndex),
                ui::Rect{rect.x + 48.0f, rect.y + 17.0f, 26.0f, 20.0f}, 10.2f, col::Faint,
                true);
    float topX = rect.x + 78.0f;
    std::wstring difficultyText = DifficultyPillText(question.difficulty);
    float difficultyWidth = canvas.Measure(difficultyText, 10.5f, true, false).Width + 20.0f;
    canvas.FillRect(ui::Rect{topX, rect.y + 14.0f, difficultyWidth, 22.0f},
                    DifficultyBackground(question.difficulty), 11.0f);
    canvas.Text(difficultyText, ui::Rect{topX, rect.y + 14.0f, difficultyWidth, 22.0f}, 10.5f,
                DifficultyForeground(question.difficulty), true,
                Gdiplus::StringAlignmentCenter);
    topX += difficultyWidth + 6.0f;
    std::wstring typeText = TypeName(question.type);
    float typeWidth = canvas.Measure(typeText, 10.2f, false, false).Width + 18.0f;
    canvas.FillRect(ui::Rect{topX, rect.y + 14.0f, typeWidth, 22.0f}, col::CardAlt, 11.0f);
    canvas.StrokeRect(ui::Rect{topX, rect.y + 14.0f, typeWidth, 22.0f}, col::Border, 11.0f, 1.0f);
    canvas.Text(typeText, ui::Rect{topX, rect.y + 14.0f, typeWidth, 22.0f}, 10.2f, col::Muted,
                false, Gdiplus::StringAlignmentCenter);

    ui::Rect editButton{rect.right() - 64.0f, rect.y + 12.0f, 46.0f, 26.0f};
    std::wstring status = question.tagged ? L"AI 已标注" : L"待标注";
    float statusTextWidth = canvas.Measure(status, 10.2f, false, false).Width;
    float statusStart = editButton.x - 12.0f - statusTextWidth - 12.0f;
    canvas.Circle(statusStart, rect.y + 26.0f, 3.2f,
                  question.tagged ? col::Green : col::Amber);
    canvas.Text(status, ui::Rect{statusStart + 9.0f, rect.y + 16.0f,
                                 statusTextWidth + 2.0f, 20.0f},
                10.2f, question.tagged ? col::Green : col::Amber, false);
    DrawButton(canvas, editButton, L"编辑", interactive ? L"edit_question" : L"",
               W(question.id), 2, true, 10.5f);

    float stemX = rect.x + 48.0f;
    float stemWidth = rect.w - 48.0f - 22.0f;
    canvas.TextBlock(W(question.stem), ui::Rect{stemX, rect.y + 41.0f, stemWidth, 38.0f}, 13.8f,
                     col::Ink, 2, false);

    std::vector<std::string> tags = question.AllTags();
    float tagX = rect.x + 48.0f;
    float tagLimit = rect.right() - 22.0f;
    float tagY = rect.y + 80.0f;
    size_t drawn = 0;
    for (const auto& tag : tags) {
        if (drawn >= 4) break;
        bool isFormula = std::any_of(question.formulas.begin(), question.formulas.end(),
                                     [&](const std::string& item) { return TagsEqual(item, tag); });
        std::wstring label = W(tag);
        Gdiplus::SizeF size = canvas.Measure(label, 10.2f, false, false);
        float width = std::min(180.0f, size.Width + 16.0f);
        if (tagX + width > tagLimit - 50.0f) break;
        DrawTagPill(canvas, label, tagX, tagY, isFormula ? col::AccentSoft : col::BlueSoft,
                    isFormula ? col::TagText : col::ConceptTagText, 180.0f);
        tagX += width + 6.0f;
        ++drawn;
    }
    if (tags.size() > drawn) {
        DrawTagPill(canvas, L"+" + IntText(static_cast<int>(tags.size() - drawn)),
                    tagX, tagY, col::CardAlt, col::Muted, 60.0f);
    }
    if (tags.empty()) {
        canvas.Text(L"暂无标签，编辑题目或使用 AI 智能标注", ui::Rect{tagX, tagY, 360.0f, 20.0f},
                    10.0f, col::Faint, false);
    }
}

void App::PaintTags(ui::Canvas& canvas, float width, float height) {
    const TagsRects rects = ComputeTagsRects(width, height);
    int totalQuestions = static_cast<int>(store_.Questions().size());
    int taggedQuestions = 0;
    for (const auto& question : store_.Questions()) {
        if (question.tagged) ++taggedQuestions;
    }
    int usedFormulas = CountTagUsages(store_.Tags(), store_, true);
    int usedConcepts = CountTagUsages(store_.Tags(), store_, false);

    const float statGap = 14.0f;
    const float statWidth = (rects.stats.w - statGap * 2.0f) / 3.0f;
    struct StatCard {
        std::wstring label;
        std::wstring value;
        std::wstring note;
        Gdiplus::Color accent;
        Gdiplus::Color soft;
        std::wstring glyph;
    } cards[] = {
        {L"公式标签", IntText(static_cast<int>(store_.Tags().formulas.size())),
         IntText(usedFormulas) + L" 个正在使用", col::TagText, col::AccentSoft, L"∑"},
        {L"知识点标签", IntText(static_cast<int>(store_.Tags().concepts.size())),
         IntText(usedConcepts) + L" 个正在使用", col::ConceptTagText, col::BlueSoft, L"知"},
        {L"已标注题目", IntText(taggedQuestions) + L" / " + IntText(totalQuestions),
         totalQuestions > 0 ? L"AI 标注覆盖率 " +
                                  IntText(totalQuestions > 0 ? taggedQuestions * 100 / totalQuestions : 0) +
                                  L"%"
                            : L"题库还是空的",
         col::Green, col::GreenSoft, L"✓"},
    };
    for (int i = 0; i < 3; ++i) {
        ui::Rect card{rects.stats.x + static_cast<float>(i) * (statWidth + statGap), rects.stats.y,
                      statWidth, rects.stats.h};
        canvas.DrawShadow(card, 14.0f);
        canvas.FillRect(card, col::Card, 14.0f);
        canvas.StrokeRect(card, col::Border, 14.0f, 1.0f);
        ui::Rect icon{card.x + 18.0f, card.y + 19.0f, 32.0f, 32.0f};
        canvas.FillRect(icon, cards[i].soft, 9.0f);
        canvas.Text(cards[i].glyph, icon, 15.0f, cards[i].accent, true,
                    Gdiplus::StringAlignmentCenter);
        canvas.Text(cards[i].label, ui::Rect{card.x + 60.0f, card.y + 16.0f, 140.0f, 18.0f},
                    10.5f, col::Muted, false);
        canvas.Text(cards[i].value, ui::Rect{card.x + 60.0f, card.y + 34.0f, card.w - 74.0f,
                                             30.0f},
                    21.0f, col::Ink, true);
        canvas.Text(cards[i].note, ui::Rect{card.x + 18.0f, card.y + 62.0f, card.w - 36.0f,
                                            16.0f},
                    9.6f, col::Faint, false);
        if (i == 2 && totalQuestions > 0) {
            float progress = static_cast<float>(taggedQuestions) / totalQuestions;
            ui::Rect bar{card.x + 18.0f, card.bottom() - 10.0f, card.w - 36.0f, 3.0f};
            canvas.FillRect(bar, col::Track, 1.5f);
            canvas.FillRect(ui::Rect{bar.x, bar.y, bar.w * progress, bar.h}, col::Green, 1.5f);
        }
    }

    struct ColumnData {
        ui::Rect card;
        std::wstring title;
        std::wstring subtitle;
        std::wstring glyph;
        Gdiplus::Color accent;
        Gdiplus::Color soft;
        std::vector<std::string> tags;
        ScrollState* scroll;
        bool formulas;
    };
    std::vector<ColumnData> columns;
    std::vector<std::string> formulas = store_.Tags().formulas;
    std::vector<std::string> concepts = store_.Tags().concepts;
    auto sortByUsage = [&](std::vector<std::string>& tags, bool formulasColumn) {
        std::stable_sort(tags.begin(), tags.end(), [&](const std::string& a, const std::string& b) {
            int ua = formulasColumn ? store_.FormulaUsage(a) : store_.ConceptUsage(a);
            int ub = formulasColumn ? store_.FormulaUsage(b) : store_.ConceptUsage(b);
            if (ua != ub) return ua > ub;
            return a < b;
        });
    };
    sortByUsage(formulas, true);
    sortByUsage(concepts, false);
    columns.push_back(ColumnData{rects.formulaCard, L"公式标签", L"公式、定理与运算名称",
                                 L"∑", col::TagText, col::AccentSoft, std::move(formulas),
                                 &formulaScroll_, true});
    columns.push_back(ColumnData{rects.conceptCard, L"知识点标签", L"知识概念与题型方法",
                                 L"知", col::ConceptTagText, col::BlueSoft, std::move(concepts),
                                 &conceptScroll_, false});

    for (auto& column : columns) {
        canvas.DrawShadow(column.card, 16.0f);
        canvas.FillRect(column.card, col::Card, 16.0f);
        canvas.StrokeRect(column.card, col::Border, 16.0f, 1.0f);
        ui::Rect icon{column.card.x + 20.0f, column.card.y + 19.0f, 36.0f, 36.0f};
        canvas.FillRect(icon, column.soft, 10.0f);
        canvas.Text(column.glyph, icon, 16.0f, column.accent, true,
                    Gdiplus::StringAlignmentCenter);
        canvas.Text(column.title, ui::Rect{column.card.x + 66.0f, column.card.y + 17.0f, 180.0f,
                                           22.0f},
                    15.5f, col::Ink, true);
        canvas.Text(column.subtitle, ui::Rect{column.card.x + 66.0f, column.card.y + 39.0f,
                                              260.0f, 16.0f},
                    10.0f, col::Faint, false);
        canvas.Text(IntText(static_cast<int>(column.tags.size())) + L" 个标签",
                    ui::Rect{column.card.right() - 120.0f, column.card.y + 23.0f, 98.0f, 20.0f},
                    10.5f, col::Muted, false, Gdiplus::StringAlignmentFar);
        canvas.Line(column.card.x + 20.0f, column.card.y + 72.0f,
                    column.card.right() - 20.0f, column.card.y + 72.0f, col::Border, 1.0f);
        ui::Rect viewport{column.card.x + 20.0f, column.card.y + 80.0f,
                          column.card.w - 40.0f, column.card.h - 96.0f};
        if (column.tags.empty()) {
            canvas.Text(L"还没有标签", ui::Rect{viewport.x, viewport.y + 30.0f, viewport.w, 22.0f},
                        13.0f, col::Muted, true, Gdiplus::StringAlignmentCenter);
            canvas.Text(L"去题库勾选题目，点击“AI 智能标注”即可自动归档",
                        ui::Rect{viewport.x + 20.0f, viewport.y + 56.0f, viewport.w - 40.0f, 40.0f},
                        10.5f, col::Faint, false, Gdiplus::StringAlignmentCenter);
            column.scroll->maxOffset = 0.0f;
            continue;
        }
        const float rowHeight = 48.0f;
        float contentHeight = rowHeight * static_cast<float>(column.tags.size());
        column.scroll->maxOffset = std::max(0.0f, contentHeight - viewport.h);
        column.scroll->offset = std::clamp(column.scroll->offset, 0.0f,
                                           column.scroll->maxOffset);
        canvas.PushClip(viewport);
        for (size_t i = 0; i < column.tags.size(); ++i) {
            float rowY = viewport.y + static_cast<float>(i) * rowHeight - column.scroll->offset;
            if (rowY + rowHeight < viewport.y || rowY > viewport.bottom()) continue;
            ui::Rect row{viewport.x, rowY, viewport.w - 8.0f, rowHeight};
            bool interactive = row.y >= viewport.y - 1.0f && row.bottom() <= viewport.bottom() + 1.0f;
            bool hovered = interactive && IsHovered(L"tag_open", W(column.tags[i]));
            if (hovered) canvas.FillRect(row.Inset(0.0f, 2.0f), col::Hover, 8.0f);
            int usage = column.formulas ? store_.FormulaUsage(column.tags[i])
                                        : store_.ConceptUsage(column.tags[i]);
            canvas.Text(W(column.tags[i]), ui::Rect{row.x + 10.0f, row.y, row.w - 140.0f, row.h},
                        13.0f, col::InkSoft, false);
            canvas.Text(usage > 0 ? IntText(usage) + L" 道题" : L"暂无题目",
                        ui::Rect{row.right() - 120.0f, row.y, 100.0f, row.h}, 10.2f,
                        usage > 0 ? col::Muted : col::Faint, false,
                        Gdiplus::StringAlignmentFar);
            canvas.Triangle(row.right() - 15.0f, row.y + 19.0f, row.right() - 9.0f,
                            row.y + 24.0f, row.right() - 15.0f, row.y + 29.0f, col::Faint);
            canvas.Line(row.x + 10.0f, row.bottom() - 1.0f, row.right() - 10.0f,
                        row.bottom() - 1.0f, col::Border, 1.0f);
            if (interactive) AddHit(row, L"tag_open", W(column.tags[i]));
        }
        canvas.PopClip();
        DrawScrollbar(canvas, *column.scroll, viewport, contentHeight);
    }

    if (store_.Questions().empty()) {
        canvas.Text(L"标签会随着题目与 AI 标注自动增长", ui::Rect{rects.formulaCard.x,
                                                                   rects.formulaCard.bottom() + 2.0f,
                                                                   rects.stats.w, 20.0f},
                    10.0f, col::Faint, false, Gdiplus::StringAlignmentCenter);
    }
}

void App::PaintAi(ui::Canvas& canvas, float width, float height) {
    const AiRects rects = ComputeAiRects(width, height);
    canvas.DrawShadow(rects.leftCard, 16.0f);
    canvas.FillRect(rects.leftCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.leftCard, col::Border, 16.0f, 1.0f);

    const float innerX = rects.leftCard.x + 28.0f;
    const float innerW = rects.leftCard.w - 56.0f;
    canvas.FillRect(ui::Rect{innerX + 1.0f, rects.leftCard.y + 23.0f, 32.0f, 32.0f},
                    col::AccentSoft, 9.0f);
    DrawSparkIcon(canvas, ui::Rect{innerX + 9.0f, rects.leftCard.y + 31.0f, 16.0f, 16.0f},
                  col::Accent);
    canvas.Text(L"接口配置", ui::Rect{innerX + 44.0f, rects.leftCard.y + 22.0f, 220.0f, 25.0f},
                17.0f, col::Ink, true);
    canvas.Text(L"使用你自己的 API，密钥只保存在这台电脑",
                ui::Rect{innerX + 44.0f, rects.leftCard.y + 47.0f, innerW - 50.0f, 18.0f}, 10.5f,
                col::Faint, false);

    canvas.Text(L"快速填充", ui::Rect{innerX, rects.leftCard.y + 76.0f, 100.0f, 16.0f}, 9.8f,
                col::Muted, true);
    std::string baseKey = util::ToLowerAscii(GetControlUtf8(apiBaseEdit_));
    bool deepseekActive = baseKey.find("deepseek") != std::string::npos;
    bool openaiActive = baseKey.find("openai.com") != std::string::npos;
    DrawButton(canvas, rects.presetDeepSeek, L"DeepSeek", L"provider", L"deepseek",
               deepseekActive ? 5 : 1, true, 11.0f);
    DrawButton(canvas, rects.presetOpenAI, L"OpenAI", L"provider", L"openai",
               openaiActive ? 5 : 1, true, 11.0f);
    DrawButton(canvas, rects.presetCustom, L"自定义", L"provider", L"custom", 1, true, 11.0f);

    canvas.Text(L"API 地址", ui::Rect{rects.baseField.x, rects.baseField.y - 22.0f, 200.0f, 16.0f},
                11.0f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.baseField, apiBaseEdit_);
    AddHit(rects.baseField, L"focus", L"api_base");
    canvas.Text(L"模型名称", ui::Rect{rects.modelField.x, rects.modelField.y - 22.0f, 200.0f,
                                      16.0f},
                11.0f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.modelField, apiModelEdit_);
    AddHit(rects.modelField, L"focus", L"api_model");
    canvas.Text(L"API 密钥", ui::Rect{rects.keyField.x, rects.keyField.y - 22.0f, 200.0f, 16.0f},
                11.0f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.keyField, apiKeyEdit_);
    AddHit(rects.keyField, L"focus", L"api_key");
    DrawButton(canvas, rects.keyToggle, apiKeyVisible_ ? L"隐藏" : L"显示",
               L"toggle_key_visible", L"", 5, true, 10.0f);

    canvas.TextWrapped(
        L"兼容 OpenAI Chat Completions。图片识题需视觉模型，支持多图、多题，AI 会生成答案和解析。"
        L"本地模型可留空密钥。",
        ui::Rect{innerX, rects.keyField.bottom() + 11.0f, innerW, 36.0f}, 10.2f, col::Faint,
        false, Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentNear);

    DrawButton(canvas, rects.saveButton, L"保存配置", L"save_ai", L"", 0, true, 12.0f);
    DrawButton(canvas, rects.testButton, testRunning_ ? L"连接中…" : L"测试连接", L"test_ai",
               L"", 1, !testRunning_, 12.0f);

    ui::Rect status = rects.status;
    Gdiplus::Color statusFill = col::CardAlt;
    Gdiplus::Color statusBorder = col::Border;
    Gdiplus::Color dotColor = col::Faint;
    std::wstring statusTitle = L"接口状态";
    std::wstring statusBody = L"保存配置后可先测试连接。成功后再回到题库使用 AI 智能标注。";
    if (testRunning_) {
        statusFill = col::BlueSoft;
        statusBorder = col::StatusInfoBorder;
        dotColor = col::Blue;
        statusTitle = L"正在测试连接…";
        statusBody = L"正在向配置的接口发送一条最小请求，不会修改题库。";
    } else if (testHasResult_) {
        statusFill = testOk_ ? col::GreenSoft : col::RedSoft;
        statusBorder = testOk_ ? col::StatusOkBorder : col::StatusFailBorder;
        dotColor = testOk_ ? col::Green : col::Red;
        statusTitle = testOk_ ? L"接口连接成功" : L"接口连接失败";
        statusBody = testStatus_;
    }
    canvas.FillRect(status, statusFill, 12.0f);
    canvas.StrokeRect(status, statusBorder, 12.0f, 1.0f);
    canvas.Circle(status.x + 20.0f, status.y + 23.0f, 3.5f, dotColor);
    canvas.Text(statusTitle, ui::Rect{status.x + 32.0f, status.y + 10.0f, status.w - 44.0f,
                                      22.0f},
                11.5f, col::InkSoft, true);
    if (!statusBody.empty()) {
        canvas.TextWrapped(statusBody, ui::Rect{status.x + 16.0f, status.y + 34.0f,
                                                status.w - 32.0f, status.h - 42.0f},
                           10.2f, col::Muted, false, Gdiplus::StringAlignmentNear,
                           Gdiplus::StringAlignmentNear);
    }

    const float helpTop = status.bottom() + 12.0f;
    if (rects.leftCard.bottom() - helpTop - 24.0f > 44.0f) {
        ui::Rect help{innerX, helpTop, innerW, rects.leftCard.bottom() - helpTop - 24.0f};
        canvas.FillRect(help, col::CardAlt, 10.0f);
        canvas.StrokeRect(help, col::Border, 10.0f, 1.0f);
        canvas.Text(L"常见接口示例", ui::Rect{help.x + 14.0f, help.y + 7.0f, help.w - 28.0f,
                                              16.0f},
                    9.3f, col::InkSoft, true);
        struct Example {
            const wchar_t* label;
            const wchar_t* value;
        } examples[] = {
            {L"DeepSeek", L"api.deepseek.com · deepseek-chat"},
            {L"OpenAI", L"api.openai.com/v1 · gpt-4o-mini"},
            {L"Ollama", L"127.0.0.1:11434/v1 · 本地模型名"},
        };
        float exampleY = help.y + 26.0f;
        for (const auto& example : examples) {
            canvas.Text(example.label, ui::Rect{help.x + 14.0f, exampleY, 68.0f, 14.0f}, 8.5f,
                        col::Muted, true);
            canvas.TextEllipsis(example.value,
                                ui::Rect{help.x + 84.0f, exampleY, help.w - 98.0f, 14.0f},
                                8.5f, col::Faint, false);
            exampleY += 14.0f;
        }
    }

    canvas.DrawShadow(rects.workflowCard, 16.0f);
    canvas.FillRect(rects.workflowCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.workflowCard, col::Border, 16.0f, 1.0f);
    canvas.Text(L"标注工作流", ui::Rect{rects.workflowCard.x + 24.0f,
                                        rects.workflowCard.y + 20.0f, 220.0f, 24.0f},
                15.5f, col::Ink, true);
    canvas.Text(L"从题目到可筛选标签", ui::Rect{rects.workflowCard.x + 24.0f,
                                                rects.workflowCard.y + 44.0f, 240.0f, 18.0f},
                10.2f, col::Faint, false);
    canvas.Line(rects.workflowCard.x + 24.0f, rects.workflowCard.y + 72.0f,
                rects.workflowCard.right() - 24.0f, rects.workflowCard.y + 72.0f, col::Border,
                1.0f);

    struct Step {
        std::wstring number;
        std::wstring title;
        std::wstring description;
        Gdiplus::Color color;
    } steps[] = {
        {L"01", L"选择或上传", L"勾选题目做标注，或点题库右上角“图片识题”上传一张/多张照片。", col::Accent},
        {L"02", L"模型理解", L"识别图中所有题目，提取公式、知识点并自动生成答案解析。", col::Blue},
        {L"03", L"归档标签", L"分别写入公式与知识点容器，供筛选组卷。", col::Green},
    };
    float stepY = rects.workflowCard.y + 88.0f;
    float stepGap = std::max(54.0f, (rects.workflowCard.h - 158.0f) / 3.0f);
    for (int i = 0; i < 3; ++i) {
        ui::Rect numberRect{rects.workflowCard.x + 24.0f, stepY, 28.0f, 28.0f};
        canvas.FillRect(numberRect, col::CardAlt, 8.0f);
        canvas.StrokeRect(numberRect, col::Border, 8.0f, 1.0f);
        canvas.Text(steps[i].number, numberRect, 9.0f, steps[i].color, true,
                    Gdiplus::StringAlignmentCenter);
        canvas.Text(steps[i].title, ui::Rect{rects.workflowCard.x + 64.0f, stepY - 1.0f,
                                              rects.workflowCard.w - 88.0f, 20.0f},
                    12.0f, col::InkSoft, true);
        canvas.TextWrapped(steps[i].description,
                           ui::Rect{rects.workflowCard.x + 64.0f, stepY + 20.0f,
                                    rects.workflowCard.w - 88.0f, 36.0f},
                           9.8f, col::Muted, false, Gdiplus::StringAlignmentNear,
                           Gdiplus::StringAlignmentNear);
        if (i < 2) {
            canvas.Line(rects.workflowCard.x + 38.0f, stepY + 28.0f,
                        rects.workflowCard.x + 38.0f, stepY + stepGap - 2.0f,
                        col::Border, 1.2f);
        }
        stepY += stepGap;
    }
    ui::Rect compatPill{rects.workflowCard.x + 24.0f,
                        rects.workflowCard.bottom() - 35.0f,
                        rects.workflowCard.w - 48.0f, 24.0f};
    canvas.FillRect(compatPill, col::AccentSoft2, 7.0f);
    canvas.Text(L"OPENAI CHAT COMPLETIONS 兼容", compatPill, 8.5f, col::TagText, true,
                Gdiplus::StringAlignmentCenter);

    canvas.DrawShadow(rects.securityCard, 16.0f);
    canvas.FillRect(rects.securityCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.securityCard, col::Border, 16.0f, 1.0f);
    canvas.Text(L"数据和隐私", ui::Rect{rects.securityCard.x + 24.0f,
                                       rects.securityCard.y + 20.0f, 220.0f, 22.0f},
                14.5f, col::Ink, true);
    const float securityTextX = rects.securityCard.x + 24.0f;
    float securityY = rects.securityCard.y + 52.0f;
    const float securityWidth = rects.securityCard.w - 48.0f;
    auto drawBullet = [&](const std::wstring& text, float height) {
        canvas.Circle(securityTextX + 3.0f, securityY + 6.0f, 2.2f, col::Accent);
        canvas.TextWrapped(text, ui::Rect{securityTextX + 14.0f, securityY,
                                          securityWidth - 14.0f, height},
                           9.8f, col::Muted, false, Gdiplus::StringAlignmentNear,
                           Gdiplus::StringAlignmentNear);
        securityY += height + 6.0f;
    };
    drawBullet(L"API 密钥使用 Windows DPAPI 加密，仅保存在本机 data/settings.json。", 34.0f);
    drawBullet(L"题目与标签只保存在本地；调用 AI 时，题目内容会发送到你配置的接口。", 34.0f);
    if (securityY + 22.0f < rects.securityCard.bottom() - 10.0f) {
        ui::Rect localPill{securityTextX, securityY,
                           std::min(securityWidth, 128.0f), 22.0f};
        canvas.FillRect(localPill, col::GreenSoft, 6.0f);
        canvas.Text(L"本地优先", localPill, 9.0f, col::Green, true,
                    Gdiplus::StringAlignmentCenter);
    }
}

void App::PaintPaper(ui::Canvas& canvas, float width, float height) {
    const PaperRects rects = ComputePaperRects(width, height);
    if (selectedIds_.empty()) {
        FrameRects frame = ComputeFrame(width, height);
        ui::Rect card{frame.x, frame.y, frame.w, std::min(frame.h, 430.0f)};
        canvas.DrawShadow(card, 16.0f);
        canvas.FillRect(card, col::Card, 16.0f);
        canvas.StrokeRect(card, col::Border, 16.0f, 1.0f);
        ui::Rect icon{card.x + card.w * 0.5f - 32.0f, card.y + 64.0f, 64.0f, 64.0f};
        canvas.FillRect(icon, col::AccentSoft, 20.0f);
        DrawDocumentIcon(canvas, ui::Rect{icon.x + 20.0f, icon.y + 17.0f, 24.0f, 30.0f},
                         col::Accent);
        canvas.Text(L"组卷篮还是空的", ui::Rect{card.x, icon.bottom() + 24.0f, card.w, 28.0f},
                    19.0f, col::Ink, true, Gdiplus::StringAlignmentCenter);
        canvas.Text(L"先在题库中勾选题目，再回来选择标题、考试时间与输出选项。",
                    ui::Rect{card.x + 30.0f, icon.bottom() + 58.0f, card.w - 60.0f, 22.0f}, 11.5f,
                    col::Muted, false, Gdiplus::StringAlignmentCenter);
        ui::Rect goButton{card.x + card.w * 0.5f - 72.0f, icon.bottom() + 102.0f, 144.0f, 40.0f};
        DrawButton(canvas, goButton, L"去题库选题", L"nav", L"bank", 0, true, 12.5f);
        return;
    }

    const GroupCounts counts = CurrentSelectionCounts();
    const int count = counts.choice + counts.fill + counts.solution;
    const float statGap = 14.0f;
    const float statWidth = (rects.stats.w - statGap * 2.0f) / 3.0f;
    struct Stat {
        std::wstring label;
        std::wstring value;
        std::wstring hint;
    } stats[] = {
        {L"已选题目", IntText(count) + L" 道", L"来自当前题库"},
        {L"预计总分", IntText(counts.points) + L" 分", L"按 5 / 5 / 12 分计"},
        {L"题型构成", L"选 " + IntText(counts.choice) + L" · 填 " + IntText(counts.fill) +
                              L" · 解 " + IntText(counts.solution),
         L"按题型自动分组"},
    };
    for (int i = 0; i < 3; ++i) {
        ui::Rect card{rects.stats.x + static_cast<float>(i) * (statWidth + statGap),
                      rects.stats.y, statWidth, rects.stats.h};
        canvas.FillRect(card, col::Card, 14.0f);
        canvas.StrokeRect(card, col::Border, 14.0f, 1.0f);
        canvas.Text(stats[i].label, ui::Rect{card.x + 18.0f, card.y + 13.0f, card.w - 36.0f,
                                             16.0f},
                    10.0f, col::Muted, false);
        canvas.Text(stats[i].value, ui::Rect{card.x + 18.0f, card.y + 33.0f, card.w - 36.0f,
                                             30.0f},
                    i == 2 ? 16.0f : 22.0f, col::Ink, true);
        canvas.Text(stats[i].hint, ui::Rect{card.x + 18.0f, card.y + 64.0f, card.w - 36.0f,
                                            14.0f},
                    9.2f, col::Faint, false);
    }

    canvas.DrawShadow(rects.formCard, 16.0f);
    canvas.FillRect(rects.formCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.formCard, col::Border, 16.0f, 1.0f);
    const float formX = rects.formCard.x + 26.0f;
    const float innerW = rects.formCard.w - 52.0f;
    canvas.Text(L"试卷信息", ui::Rect{formX, rects.formCard.y + 20.0f, 220.0f, 24.0f}, 16.0f,
                col::Ink, true);
    ui::Rect wordBadge{rects.formCard.right() - 26.0f - 98.0f, rects.formCard.y + 20.0f, 98.0f,
                       24.0f};
    canvas.FillRect(wordBadge, col::AccentSoft, 7.0f);
    canvas.Text(L"A4 · Word", wordBadge, 9.0f, col::Accent, true,
                Gdiplus::StringAlignmentCenter);

    const float labelOffset = rects.compact ? 22.0f : 24.0f;
    canvas.Text(L"试卷标题", ui::Rect{formX, rects.titleField.y - labelOffset, 120.0f, 16.0f},
                10.8f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.titleField, paperTitleEdit_);
    AddHit(rects.titleField, L"focus", L"paper_title");

    canvas.Text(L"考试时间", ui::Rect{formX, rects.minutesField.y - labelOffset, 120.0f, 16.0f},
                10.8f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.minutesField, paperMinutesEdit_);
    AddHit(rects.minutesField, L"focus", L"paper_minutes");
    canvas.Text(L"分钟", ui::Rect{rects.minutesField.right() + 10.0f, rects.minutesField.y,
                                  36.0f, rects.minutesField.h},
                11.0f, col::Muted, false);
    canvas.Text(L"满分 " + IntText(counts.points) + L" 分  ·  符合 A4 打印版式",
                ui::Rect{rects.minutesField.right() + 64.0f, rects.minutesField.y,
                         innerW - 170.0f, rects.minutesField.h},
                10.5f, col::Faint, false);

    canvas.Text(L"注意事项", ui::Rect{formX, rects.notesField.y - labelOffset, 160.0f, 16.0f},
                10.8f, col::InkSoft, true);
    canvas.Text(L"每行一条，将显示在试卷首页",
                ui::Rect{formX, rects.notesField.y - labelOffset, innerW, 16.0f}, 9.2f,
                col::Faint, false, Gdiplus::StringAlignmentFar);
    DrawFieldBackground(canvas, rects.notesField, paperNotesEdit_);
    AddHit(rects.notesField, L"focus", L"paper_notes");

    float outputY = rects.notesField.bottom() + (rects.compact ? 16.0f : 22.0f);
    canvas.Text(L"输出设置", ui::Rect{formX, outputY, 160.0f, 18.0f}, 11.5f, col::InkSoft, true);
    outputY += rects.compact ? 23.0f : 25.0f;
    struct OptionData {
        const wchar_t* label;
        const wchar_t* payload;
        bool value;
        bool enabled;
    } options[] = {
        {L"参考答案", L"answers", includeAnswers_, true},
        {L"详细解析", L"analysis", includeAnalysis_, includeAnswers_},
        {L"解答题留白", L"space", includeAnswerSpace_, true},
    };
    const float optionWidth = innerW / 3.0f;
    for (int i = 0; i < 3; ++i) {
        ui::Rect row{formX + static_cast<float>(i) * optionWidth, outputY, optionWidth - 10.0f,
                     28.0f};
        bool hovered = options[i].enabled && IsHovered(L"paper_opt", options[i].payload);
        if (hovered) canvas.FillRect(row, col::Hover, 7.0f);
        canvas.Checkbox(ui::Rect{row.x, row.y + 5.0f, 17.0f, 17.0f}, options[i].value,
                         hovered, !options[i].enabled);
        canvas.Text(options[i].label, ui::Rect{row.x + 26.0f, row.y, row.w - 26.0f, row.h},
                    11.0f, options[i].enabled ? col::InkSoft : col::Faint, false);
        if (options[i].enabled) AddHit(row, L"paper_opt", options[i].payload);
    }

    const float specY = outputY + (rects.compact ? 32.0f : 44.0f);
    float specLineY = specY;
    if (!rects.compact) {
        canvas.Text(L"版式规格", ui::Rect{formX, specY, 160.0f, 18.0f}, 11.5f, col::InkSoft,
                    true);
        std::wstring specs[] = {
            L"标准 A4 纵向 · 页边距 2.0 cm · 页脚自动页码",
            L"标题黑体、正文宋体 · 选择题选项双栏排列",
            L"按题型分组，组内按 A→C 难度排序 · 答案单独起页",
        };
        specLineY = specY + 24.0f;
        for (const auto& line : specs) {
            canvas.Circle(formX + 3.0f, specLineY + 7.0f, 2.0f, col::BorderStrong);
            canvas.Text(line, ui::Rect{formX + 13.0f, specLineY, innerW - 13.0f, 18.0f}, 9.8f,
                        col::Muted, false);
            specLineY += 19.0f;
        }
    } else if (specY + 16.0f < rects.generateButton.y - 12.0f) {
        canvas.Text(L"A4 纵向 · 宋体/黑体 · 自动页码 · 答案另起页",
                    ui::Rect{formX, specY, innerW, 16.0f}, 9.2f, col::Faint, false);
        specLineY = specY + 18.0f;
    }

    if (!rects.compact && !lastExportPath_.empty() &&
        rects.generateButton.y - 42.0f > specLineY) {
        ui::Rect exportRow{formX, rects.generateButton.y - 40.0f, innerW, 28.0f};
        canvas.FillRect(exportRow, col::GreenSoft, 8.0f);
        canvas.Circle(exportRow.x + 13.0f, exportRow.y + 14.0f, 3.0f, col::Green);
        std::wstring filename = lastExportPath_.filename().wstring();
        canvas.TextEllipsis(L"上次导出：" + filename,
                            ui::Rect{exportRow.x + 22.0f, exportRow.y, exportRow.w - 150.0f,
                                      exportRow.h},
                            9.8f, col::Green, false);
        ui::Rect openRect{exportRow.right() - 118.0f, exportRow.y, 46.0f, exportRow.h};
        ui::Rect folderRect{exportRow.right() - 66.0f, exportRow.y, 58.0f, exportRow.h};
        canvas.Text(L"打开", openRect, 9.8f, col::Green, true,
                    Gdiplus::StringAlignmentCenter);
        canvas.Text(L"文件夹", folderRect, 9.8f, col::Green, true,
                    Gdiplus::StringAlignmentCenter);
        AddHit(openRect, L"open_export");
        AddHit(folderRect, L"open_export_folder");
    }

    DrawButton(canvas, rects.generateButton, L"导出可打印 Word 试卷", L"generate_paper", L"", 0,
               true, 13.5f);

    canvas.DrawShadow(rects.previewCard, 16.0f);
    canvas.FillRect(rects.previewCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.previewCard, col::Border, 16.0f, 1.0f);
    canvas.Text(L"版式预览", ui::Rect{rects.previewCard.x + 24.0f,
                                      rects.previewCard.y + 20.0f, 180.0f, 23.0f},
                15.5f, col::Ink, true);
    canvas.Text(L"最终文件为 A4 版式 Word 文档", ui::Rect{rects.previewCard.x + 24.0f,
                                                        rects.previewCard.y + 44.0f, 260.0f, 17.0f},
                9.8f, col::Faint, false);
    ui::Rect printBadge{rects.previewCard.right() - 105.0f, rects.previewCard.y + 20.0f, 82.0f,
                        22.0f};
    canvas.FillRect(printBadge, col::GreenSoft, 6.0f);
    canvas.Text(L"A4 可打印", printBadge, 8.8f, col::Green, true,
                Gdiplus::StringAlignmentCenter);
    DrawMiniPage(canvas, rects.paper);
    if (rects.paper.bottom() + 26.0f < rects.previewCard.bottom()) {
        canvas.Text(includeAnswers_ ? L"包含参考答案与解析，答案部分将自动另起一页"
                                     : L"仅生成试题，不附带答案页",
                    ui::Rect{rects.previewCard.x + 24.0f, rects.paper.bottom() + 14.0f,
                             rects.previewCard.w - 48.0f, 18.0f},
                    9.4f, col::Faint, false, Gdiplus::StringAlignmentCenter);
    }
}

void App::DrawMiniPage(ui::Canvas& canvas, const ui::Rect& rect) {
    if (rect.w < 40.0f || rect.h < 60.0f) return;
    // The miniature always represents white printable paper, including in dark mode.
    const Gdiplus::Color paper = ui::Color(255, 255, 255);
    const Gdiplus::Color paperInk = ui::Color(22, 29, 45);
    const Gdiplus::Color paperSoft = ui::Color(70, 80, 100);
    const Gdiplus::Color paperMuted = ui::Color(110, 118, 134);
    const Gdiplus::Color paperFaint = ui::Color(145, 152, 167);
    const Gdiplus::Color paperLine = ui::Color(215, 219, 228);
    const Gdiplus::Color paperLineSoft = ui::Color(230, 233, 240);
    const Gdiplus::Color paperBorder = ui::Color(221, 224, 232);
    canvas.DrawShadow(rect, 3.0f);
    canvas.FillRect(rect, paper, 2.0f);
    canvas.StrokeRect(rect, paperBorder, 2.0f, 1.0f);
    const float left = rect.x + rect.w * 0.09f;
    const float right = rect.right() - rect.w * 0.09f;
    const float contentWidth = right - left;
    float y = rect.y + rect.h * 0.07f;
    std::wstring title = GetControlText(paperTitleEdit_);
    if (title.empty()) title = L"数学试卷";
    canvas.TextEllipsis(title, ui::Rect{left, y, contentWidth, 16.0f}, 9.6f, paperInk, true,
                        Gdiplus::StringAlignmentCenter);
    y += 17.0f;
    int minutes = 120;
    std::wstring minutesText = GetControlText(paperMinutesEdit_);
    if (!minutesText.empty()) {
        try {
            minutes = std::stoi(minutesText);
        } catch (...) {
            minutes = 120;
        }
    }
    minutes = std::clamp(minutes, 1, 600);
    GroupCounts counts = CurrentSelectionCounts();
    canvas.Text(L"考试时间：" + IntText(minutes) + L" 分钟　满分：" + IntText(counts.points) +
                    L" 分",
                ui::Rect{left, y, contentWidth, 12.0f}, 6.8f, paperMuted, false,
                Gdiplus::StringAlignmentCenter);
    y += 12.0f;
    canvas.Line(left, y, right, y, paperBorder, 0.7f);
    y += rect.h * 0.035f;

    std::wstring names[] = {L"一、选择题", L"二、填空题", L"三、解答题"};
    int sectionCounts[] = {counts.choice, counts.fill, counts.solution};
    int sectionIndex = 0;
    for (int i = 0; i < 3; ++i) {
        if (sectionCounts[i] == 0) continue;
        if (y > rect.bottom() - 40.0f) break;
        canvas.Text(names[sectionIndex], ui::Rect{left, y, contentWidth, 12.0f}, 7.0f,
                    paperSoft, true);
        y += 12.0f;
        int rows = std::min(3, sectionCounts[i]);
        for (int r = 0; r < rows; ++r) {
            if (y > rect.bottom() - 28.0f) break;
            canvas.Text(IntText(r + 1) + L"．", ui::Rect{left, y, 14.0f, 10.0f}, 6.2f,
                        paperMuted, false);
            canvas.FillRect(ui::Rect{left + 14.0f, y + 4.0f, contentWidth * (r == 1 ? 0.65f : 0.8f),
                                     1.0f},
                            paperLine, 0.0f);
            if (i == 0) {
                // Draw a minimal hint of the option row.
                canvas.FillRect(ui::Rect{left + 20.0f, y + 8.0f, contentWidth * 0.35f, 0.8f},
                                paperLineSoft, 0.0f);
                y += 10.0f;
            } else {
                y += 10.0f;
            }
        }
        if (sectionCounts[i] > 3) {
            if (y <= rect.bottom() - 26.0f) {
                canvas.Text(L"… 共 " + IntText(sectionCounts[i]) + L" 题",
                            ui::Rect{left + 14.0f, y, contentWidth - 14.0f, 10.0f}, 6.0f,
                            paperFaint, false);
                y += 10.0f;
            }
        }
        ++sectionIndex;
        y += 4.0f;
    }
    if (includeAnswers_) {
        canvas.FillRect(ui::Rect{left, rect.bottom() - 34.0f, contentWidth, 0.7f},
                        paperLine, 0.0f);
        canvas.Text(L"参考答案与解析", ui::Rect{left, rect.bottom() - 29.0f, contentWidth, 10.0f},
                    6.2f, paperMuted, false, Gdiplus::StringAlignmentCenter);
    }
    canvas.Text(L"第 1 页", ui::Rect{left, rect.bottom() - 15.0f, contentWidth, 9.0f}, 5.8f,
                paperFaint, false, Gdiplus::StringAlignmentCenter);
}

void App::Activate(const HitTarget& target) {
    const std::wstring& action = target.action;
    const std::wstring& payload = target.payload;
    if (action == L"nav") {
        if (payload == L"bank") SwitchPage(Page::Bank);
        else if (payload == L"tags") SwitchPage(Page::Tags);
        else if (payload == L"ai") SwitchPage(Page::Ai);
        else if (payload == L"paper") SwitchPage(Page::Paper);
        return;
    }
    if (action == L"focus") {
        if (payload == L"search") {
            SetFocus(searchEdit_);
        } else if (payload == L"api_base") {
            SetFocus(apiBaseEdit_);
        } else if (payload == L"api_model") {
            SetFocus(apiModelEdit_);
        } else if (payload == L"api_key") {
            SetFocus(apiKeyEdit_);
        } else if (payload == L"paper_title") {
            SetFocus(paperTitleEdit_);
        } else if (payload == L"paper_minutes") {
            SetFocus(paperMinutesEdit_);
        } else if (payload == L"paper_notes") {
            SetFocus(paperNotesEdit_);
        }
        return;
    }
    if (action == L"dark_mode") {
        SetDarkMode(!darkMode_);
        return;
    }
    if (action == L"open_data") {
        HINSTANCE result = ShellExecuteW(hwnd_, L"open", store_.DataDir().c_str(), nullptr, nullptr,
                                         SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32) ShowToast(L"无法打开数据目录。", true);
        return;
    }
    if (action == L"new_question") {
        OpenQuestionEditor({});
        return;
    }
    if (action == L"edit_question") {
        OpenQuestionEditor(util::ToUtf8(payload));
        return;
    }
    if (action == L"row") {
        std::string id = util::ToUtf8(payload);
        if (selectedIds_.count(id)) selectedIds_.erase(id);
        else selectedIds_.insert(id);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"select_all") {
        bool all = !visibleIds_.empty();
        for (const auto& id : visibleIds_) {
            if (!selectedIds_.count(id)) {
                all = false;
                break;
            }
        }
        for (const auto& id : visibleIds_) {
            if (all) selectedIds_.erase(id);
            else selectedIds_.insert(id);
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"chip_diff") {
        int index = std::clamp(std::stoi(payload), 0, 2);
        filter_.difficulties[static_cast<size_t>(index)] =
            !filter_.difficulties[static_cast<size_t>(index)];
        bankScroll_.offset = 0.0f;
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"chip_type") {
        int index = std::clamp(std::stoi(payload), 0, 2);
        filter_.types[static_cast<size_t>(index)] = !filter_.types[static_cast<size_t>(index)];
        bankScroll_.offset = 0.0f;
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"toggle_untagged") {
        filter_.onlyUntagged = !filter_.onlyUntagged;
        bankScroll_.offset = 0.0f;
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"tag_match_mode") {
        filter_.matchAllTags = !filter_.matchAllTags;
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"filter_tag") {
        ToggleTagFilter(util::ToUtf8(payload));
        return;
    }
    if (action == L"tag_open") {
        ApplyTagFromLibrary(util::ToUtf8(payload));
        return;
    }
    if (action == L"reset_filter") {
        ClearFilter();
        return;
    }
    if (action == L"clear_selection") {
        selectedIds_.clear();
        if (page_ == Page::Paper) LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"go_paper") {
        SwitchPage(Page::Paper);
        return;
    }
    if (action == L"ai_tag") {
        StartAiTagging();
        return;
    }
    if (action == L"image_question") {
        StartImageQuestionImport();
        return;
    }
    if (action == L"provider") {
        OnProviderPreset(util::ToUtf8(payload));
        return;
    }
    if (action == L"toggle_key_visible") {
        ToggleApiKeyVisibility();
        return;
    }
    if (action == L"save_ai") {
        SaveAiSettings(true);
        return;
    }
    if (action == L"test_ai") {
        StartConnectionTest();
        return;
    }
    if (action == L"paper_opt") {
        if (payload == L"answers") {
            includeAnswers_ = !includeAnswers_;
            if (!includeAnswers_) includeAnalysis_ = false;
        } else if (payload == L"analysis") {
            if (includeAnswers_) includeAnalysis_ = !includeAnalysis_;
        } else if (payload == L"space") {
            includeAnswerSpace_ = !includeAnswerSpace_;
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"generate_paper") {
        GeneratePaper();
        return;
    }
    if (action == L"open_export") {
        if (!lastExportPath_.empty()) OpenPath(lastExportPath_);
        return;
    }
    if (action == L"open_export_folder") {
        if (!lastExportPath_.empty()) OpenContainingFolder(lastExportPath_);
        return;
    }
}

void App::OnProviderPreset(const std::string& provider) {
    if (provider == "deepseek") {
        SetControlText(apiBaseEdit_, L"https://api.deepseek.com");
        SetControlText(apiModelEdit_, L"deepseek-chat");
    } else if (provider == "openai") {
        SetControlText(apiBaseEdit_, L"https://api.openai.com/v1");
        SetControlText(apiModelEdit_, L"gpt-4o-mini");
    } else {
        SetFocus(apiBaseEdit_);
        SendMessageW(apiBaseEdit_, EM_SETSEL, 0, -1);
        return;
    }
    testHasResult_ = false;
    testStatus_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ToggleApiKeyVisibility() {
    apiKeyVisible_ = !apiKeyVisible_;
    if (apiKeyEdit_) {
        SendMessageW(apiKeyEdit_, EM_SETPASSWORDCHAR, apiKeyVisible_ ? 0 : 0x25CF, 0);
        InvalidateRect(apiKeyEdit_, nullptr, TRUE);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool App::ReadSettingsFromFields(Settings& settings, bool showErrors) {
    std::string base = util::Trim(util::ToUtf8(GetControlText(apiBaseEdit_)));
    std::string model = util::Trim(util::ToUtf8(GetControlText(apiModelEdit_)));
    std::string key = util::Trim(util::ToUtf8(GetControlText(apiKeyEdit_)));
    if (base.empty()) {
        if (showErrors) {
            ShowToast(L"请填写 API 地址。", true);
            SetFocus(apiBaseEdit_);
        }
        return false;
    }
    if (base.rfind("http://", 0) != 0 && base.rfind("https://", 0) != 0) {
        std::string lower = util::ToLowerAscii(base);
        bool local = lower.find("localhost") != std::string::npos ||
                     lower.find("127.0.0.1") != std::string::npos ||
                     lower.find("0.0.0.0") != std::string::npos;
        base = (local ? "http://" : "https://") + base;
    }
    while (!base.empty() && base.back() == '/') base.pop_back();
    if (model.empty()) {
        if (showErrors) {
            ShowToast(L"请填写模型名称。", true);
            SetFocus(apiModelEdit_);
        }
        return false;
    }
    settings.apiBase = base;
    settings.model = model;
    settings.apiKey = key;
    return true;
}

void App::SaveAiSettings(bool notify) {
    Settings settings = store_.Config();
    if (!ReadSettingsFromFields(settings, notify)) {
        return;
    }
    store_.Config() = settings;
    std::string error;
    if (!store_.SaveSettings(&error)) {
        ShowToast(W(error), true);
        return;
    }
    if (notify) {
        ShowToast(L"AI 配置已保存，密钥已加密保存在本机。");
    }
}

void App::StartConnectionTest() {
    if (testRunning_ || aiRunning_ || imageExtracting_) return;
    Settings settings = store_.Config();
    if (!ReadSettingsFromFields(settings, true)) return;
    testRunning_ = true;
    testHasResult_ = false;
    testStatus_ = L"正在连接 AI 服务…";
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    testThread_ = std::thread([target, settings]() {
        AiTestResult result = TestAiConnection(settings);
        auto* update = new TestUpdate{result.ok, result.message};
        if (!PostMessageW(target, kMsgTestFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::HandleTestFinished(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    std::unique_ptr<TestUpdate> update(reinterpret_cast<TestUpdate*>(lParam));
    if (testThread_.joinable()) testThread_.join();
    testRunning_ = false;
    if (update) {
        testHasResult_ = true;
        testOk_ = update->ok;
        testStatus_ = W(update->message);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::StartAiTagging() {
    if (aiRunning_) return;
    if (imageExtracting_) {
        ShowToast(L"图片识别正在进行，请稍候再使用 AI 标注。", true);
        return;
    }
    if (selectedIds_.empty()) {
        ShowToast(L"请先在题库中勾选题目。", true);
        return;
    }
    Settings settings = store_.Config();
    if (util::Trim(settings.apiBase).empty()) {
        settings.apiBase = "https://api.deepseek.com";
    }
    if (util::Trim(settings.model).empty()) {
        SwitchPage(Page::Ai);
        ShowToast(L"请先在 AI 接口中填写模型名称。", true);
        SetFocus(apiModelEdit_);
        return;
    }
    std::vector<Question> batch;
    for (const auto& question : store_.Questions()) {
        if (selectedIds_.count(question.id)) batch.push_back(question);
    }
    if (batch.empty()) {
        ShowToast(L"所选题目已经不在题库中。", true);
        return;
    }
    TagLibrary library = store_.Tags();
    aiTotal_ = static_cast<int>(batch.size());
    aiDone_ = 0;
    aiSuccess_ = 0;
    aiFailed_ = 0;
    aiLastError_.clear();
    aiRunning_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    aiThread_ = std::thread([target, batch = std::move(batch), settings,
                             library = std::move(library)]() mutable {
        for (const auto& question : batch) {
            AiTagResult result = AnalyzeQuestionWithAi(question, settings, library);
            auto* update = new AiUpdate;
            update->questionId = question.id;
            update->ok = result.ok;
            update->formulas = std::move(result.formulas);
            update->concepts = std::move(result.concepts);
            update->error = result.error;
            if (!PostMessageW(target, kMsgAiUpdate, 0, reinterpret_cast<LPARAM>(update))) {
                delete update;
                break;
            }
        }
        PostMessageW(target, kMsgAiFinished, 0, 0);
    });
}

void App::HandleAiUpdate(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    std::unique_ptr<AiUpdate> update(reinterpret_cast<AiUpdate*>(lParam));
    if (!update) return;
    ++aiDone_;
    if (update->ok) {
        Question* question = store_.FindQuestion(update->questionId);
        if (question) {
            MergeUniqueTags(question->formulas, update->formulas);
            MergeUniqueTags(question->concepts, update->concepts);
            question->tagged = question->HasAnyTag();
            store_.MergeQuestionTags(*question);
            ++aiSuccess_;
        }
    } else {
        ++aiFailed_;
        if (aiLastError_.empty()) aiLastError_ = W(update->error);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::HandleAiFinished(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    (void)lParam;
    if (aiThread_.joinable()) aiThread_.join();
    aiRunning_ = false;
    std::string error;
    bool saved = store_.SaveQuestions(&error);
    saved = store_.SaveTags(&error) && saved;
    RebuildFiltered();
    if (!saved) {
        ShowToast(W(error), true);
    }
    if (aiFailed_ > 0) {
        std::wstring message = L"AI 标注完成\n\n成功：" + IntText(aiSuccess_) +
                               L" 道　失败：" + IntText(aiFailed_) + L" 道";
        if (!aiLastError_.empty()) {
            message += L"\n\n";
            if (aiLastError_.size() > 500) {
                message += aiLastError_.substr(0, 500) + L"…";
            } else {
                message += aiLastError_;
            }
        }
        MessageBoxW(hwnd_, message.c_str(), L"AI 标注结果", MB_ICONWARNING);
        InvalidateRect(hwnd_, nullptr, FALSE);
    } else {
        ShowToast(L"AI 标注完成：成功标注 " + IntText(aiSuccess_) + L" 道题。");
    }
    LayoutEditControls();
}

void App::StartImageQuestionImport() {
    if (imageExtracting_ || aiRunning_ || testRunning_) return;
    Settings settings = store_.Config();
    if (util::Trim(settings.apiBase).empty() || util::Trim(settings.model).empty()) {
        SwitchPage(Page::Ai);
        ShowToast(L"请先在 AI 接口中填写 API 地址和模型名称。", true);
        SetFocus(apiModelEdit_);
        return;
    }

    wchar_t fileBuffer[32768] = {};
    std::wstring initialDir = util::PicturesDirectory().wstring();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter =
        L"图片文件 (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0"
        L"所有文件 (*.*)\0*.*\0\0";
    dialog.lpstrFile = fileBuffer;
    dialog.nMaxFile = static_cast<DWORD>(std::size(fileBuffer));
    dialog.lpstrInitialDir = initialDir.c_str();
    dialog.lpstrTitle = L"选择题目图片（可多选，AI 将提取所有题目并生成答案解析）";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR |
                   OFN_ALLOWMULTISELECT;
    if (!GetOpenFileNameW(&dialog)) {
        return;
    }

    std::vector<std::filesystem::path> images;
    std::wstring first(fileBuffer);
    if (first.empty()) return;
    const wchar_t* extra = fileBuffer + first.size() + 1;
    if (*extra == L'\0') {
        images.emplace_back(first);
    } else {
        std::filesystem::path directory(first);
        for (const wchar_t* name = extra; *name; name += wcslen(name) + 1) {
            images.push_back(directory / name);
        }
    }
    if (images.empty()) {
        ShowToast(L"没有选择可识别的图片。", true);
        return;
    }
    if (images.size() > 16) {
        MessageBoxW(hwnd_, L"一次最多选择 16 张图片，请分批识别。", L"图片识题",
                    MB_ICONINFORMATION);
        return;
    }

    if (imageThread_.joinable()) imageThread_.join();
    imageExtracting_ = true;
    imageBatchCount_ = static_cast<int>(images.size());
    SetTimer(hwnd_, 9, 260, nullptr);
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    TagLibrary library = store_.Tags();
    imageThread_ = std::thread([target, images = std::move(images), settings,
                                library = std::move(library)]() mutable {
        auto* update = new ImageUpdate;
        std::string errors;
        for (size_t index = 0; index < images.size(); ++index) {
            const auto& path = images[index];
            std::string prefix = "第 " + std::to_string(index + 1) + " 张图片";
            std::string filename = util::ToUtf8(path.filename().wstring());
            if (!filename.empty()) prefix += "（" + filename + "）";
            EncodedImageData encoded;
            std::string imageError;
            if (!EncodeImageForAi(path, encoded, &imageError)) {
                errors += prefix + "：" + imageError + "\n";
                continue;
            }
            AiImageQuestionResult result =
                AnalyzeQuestionImageWithAi(encoded.dataUrl, settings, library);
            if (!result.ok) {
                errors += prefix + "：" + result.error + "\n";
                continue;
            }
            for (auto& question : result.questions) {
                update->questions.push_back(std::move(question));
            }
        }
        update->ok = !update->questions.empty();
        update->error = std::move(errors);
        if (!PostMessageW(target, kMsgImageFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::HandleImageFinished(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    std::unique_ptr<ImageUpdate> update(reinterpret_cast<ImageUpdate*>(lParam));
    if (imageThread_.joinable()) imageThread_.join();
    imageExtracting_ = false;
    const int batchCount = imageBatchCount_;
    imageBatchCount_ = 0;
    KillTimer(hwnd_, 9);
    InvalidateRect(hwnd_, nullptr, FALSE);
    if (!update) return;
    if (!update->ok) {
        std::wstring message = L"图片识题失败：\n\n" + W(update->error);
        message += L"\n\n提示：图片识题需要使用支持图片输入（Vision）的模型。";
        MessageBoxW(hwnd_, message.c_str(), L"图片识题", MB_ICONWARNING);
        return;
    }

    const int questionCount = static_cast<int>(update->questions.size());
    int missingAnswer = 0;
    for (const auto& question : update->questions) {
        if (util::Trim(question.answer).empty() || util::Trim(question.analysis).empty()) {
            ++missingAnswer;
        }
    }
    std::wstring confirm = L"AI 从 " + IntText(batchCount) + L" 张图片中识别出 " +
                           IntText(questionCount) + L" 道题，并已为每道题生成答案与解析。\n\n" +
                           L"是否全部加入题库？\n加入后可在题库顶部逐题检查、编辑或组卷。";
    if (missingAnswer > 0) {
        confirm += L"\n\n注意：其中 " + IntText(missingAnswer) +
                   L" 道题的答案或解析为空，导入后请补全。";
    }
    if (!util::Trim(update->error).empty()) {
        confirm += L"\n\n有部分图片识别失败，已成功识别的题目仍可导入。";
    }
    int answer = MessageBoxW(hwnd_, confirm.c_str(), L"图片识题结果",
                             MB_YESNO | MB_ICONQUESTION);
    if (answer != IDYES) {
        ShowToast(L"已取消导入。");
        return;
    }

    std::set<std::string> knownIds;
    for (const auto& question : store_.Questions()) knownIds.insert(question.id);
    std::vector<Question> imported;
    imported.reserve(update->questions.size());
    for (auto& question : update->questions) {
        if (question.id.empty() || knownIds.count(question.id)) {
            question.id = util::NewId("q");
        }
        knownIds.insert(question.id);
        if (question.created.empty()) question.created = util::NowIso8601();
        question.tagged = question.HasAnyTag();
        store_.MergeQuestionTags(question);
        imported.push_back(std::move(question));
    }
    auto& questions = store_.Questions();
    questions.insert(questions.begin(), std::make_move_iterator(imported.begin()),
                     std::make_move_iterator(imported.end()));

    filter_.Reset();
    if (searchEdit_) SetControlText(searchEdit_, L"");
    bankScroll_.offset = 0.0f;
    filterTagScroll_.offset = 0.0f;
    RebuildFiltered();
    SwitchPage(Page::Bank);
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);

    std::string saveError;
    bool saved = store_.SaveQuestions(&saveError);
    saved = store_.SaveTags(&saveError) && saved;
    if (!saved) {
        ShowToast(W(saveError), true);
    } else {
        ShowToast(L"已导入 " + IntText(questionCount) + L" 道题，答案与解析已写入题库顶部。");
    }
    if (!util::Trim(update->error).empty()) {
        std::wstring detail = W(update->error);
        if (detail.size() > 900) detail = detail.substr(0, 900) + L"…";
        MessageBoxW(hwnd_, (L"部分图片识别失败：\n\n" + detail).c_str(), L"图片识题",
                    MB_ICONWARNING);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::GeneratePaper() {
    std::vector<Question> paperQuestions;
    for (const auto& question : store_.Questions()) {
        if (selectedIds_.count(question.id)) paperQuestions.push_back(question);
    }
    if (paperQuestions.empty()) {
        ShowToast(L"请先在题库中勾选题目。", true);
        return;
    }
    std::wstring title = GetControlText(paperTitleEdit_);
    if (util::Trim(title).empty()) title = L"高三数学模拟试卷";
    int minutes = 120;
    {
        std::wstring minutesText = GetControlText(paperMinutesEdit_);
        wchar_t* end = nullptr;
        long parsed = wcstol(minutesText.c_str(), &end, 10);
        if (end && end != minutesText.c_str()) {
            minutes = util::ClampInt(static_cast<int>(parsed), 1, 600);
        }
    }
    std::wstring notes = GetControlText(paperNotesEdit_);
    PaperOptions options;
    options.title = util::ToUtf8(title);
    options.durationMinutes = minutes;
    options.includeAnswers = includeAnswers_;
    options.includeAnalysis = includeAnalysis_ && includeAnswers_;
    options.includeAnswerSpace = includeAnswerSpace_;
    options.notes = util::ToUtf8(notes);

    std::wstring defaultName = SafeFileStem(title) + L"_" +
                               util::FormatLocalTime(L"%Y%m%d_%H%M") + L".docx";
    wchar_t filePath[1024] = {};
    wcsncpy(filePath, defaultName.c_str(), std::size(filePath) - 1);
    filePath[std::size(filePath) - 1] = L'\0';
    std::wstring initialDir = util::DesktopDirectory().wstring();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = L"Word 文档 (*.docx)\0*.docx\0所有文件 (*.*)\0*.*\0\0";
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = static_cast<DWORD>(std::size(filePath));
    dialog.lpstrInitialDir = initialDir.c_str();
    dialog.lpstrTitle = L"导出 Word 试卷";
    dialog.lpstrDefExt = L"docx";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) {
        return;
    }
    std::filesystem::path outputPath(filePath);
    DocxBuildResult result = GenerateExamDocx(paperQuestions, options, outputPath);
    if (!result.ok) {
        std::wstring message = L"生成试卷失败：\n\n" + W(result.error);
        MessageBoxW(hwnd_, message.c_str(), L"导出失败", MB_ICONERROR);
        return;
    }
    lastExportPath_ = outputPath;
    InvalidateRect(hwnd_, nullptr, FALSE);
    ShowToast(L"试卷已生成：" + outputPath.filename().wstring() + L"（" +
              IntText(result.questionCount) + L" 题，满分 " + IntText(result.totalPoints) +
              L" 分）");
}

void App::PaintToast(ui::Canvas& canvas, float width, float height) {
    if (!toastActive_ || toastText_.empty()) return;
    std::wstring message = toastText_;
    for (wchar_t& c : message) {
        if (c == L'\r' || c == L'\n') c = L' ';
    }
    float maxTextWidth = std::min(760.0f, width - 140.0f);
    Gdiplus::SizeF measured = canvas.Measure(message, 12.0f, false, false);
    float toastWidth = std::min(maxTextWidth + 52.0f, measured.Width + 52.0f);
    toastWidth = std::max(210.0f, toastWidth);
    float toastHeight = 42.0f;
    float toastY = height - 74.0f;
    if (page_ == Page::Bank && !selectedIds_.empty()) toastY = height - 124.0f;
    ui::Rect toast{(width - toastWidth) * 0.5f, toastY, toastWidth, toastHeight};
    canvas.DrawShadow(toast, 12.0f);
    canvas.FillRect(toast, toastError_ ? col::ToastErrorBg : col::ToastBg, 12.0f);
    canvas.Circle(toast.x + 19.0f, toast.y + toastHeight * 0.5f, 3.5f,
                  toastError_ ? col::ToastErrorText : col::ToastOkDot);
    canvas.TextEllipsis(message, ui::Rect{toast.x + 32.0f, toast.y, toast.w - 46.0f, toast.h},
                        12.0f, col::ToastText, false);
}
