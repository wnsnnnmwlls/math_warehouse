#include "app.h"

#include "ai.h"
#include "dedup.h"
#include "docx.h"
#include "editor.h"
#include "image_data.h"
#include "math_draw.h"
#include "keyboard.h"
#include "math_text.h"
#include "text_file.h"
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
#include <cstring>
#include <cwchar>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr UINT kMsgAiUpdate = WM_APP + 11;
constexpr UINT kMsgAiFinished = WM_APP + 12;
constexpr UINT kMsgTestFinished = WM_APP + 13;
constexpr UINT kMsgImageFinished = WM_APP + 14;
constexpr UINT kMsgExamFinished = WM_APP + 15;
constexpr UINT kMsgToolFinished = WM_APP + 16;
constexpr UINT_PTR kToastTimer = 1;
constexpr UINT_PTR kExamTimer = 10;
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

std::string ClampPreviewText(const std::string& text, size_t maxBytes) {
    std::string value = util::Trim(text);
    if (value.size() <= maxBytes) return value;
    return value.substr(0, maxBytes) + "…";
}

struct ModelChoice {
    std::wstring name;
    std::wstring base;
};

std::wstring BaseForModel(const std::wstring& name) {
    if (name.rfind(L"deepseek-", 0) == 0) return L"https://api.deepseek.com";
    if (name.rfind(L"gpt-", 0) == 0 || name.rfind(L"o1", 0) == 0 ||
        name.rfind(L"o3", 0) == 0 || name.rfind(L"o4", 0) == 0) {
        return L"https://api.openai.com/v1";
    }
    if (name.rfind(L"qwen", 0) == 0) {
        return L"https://dashscope.aliyuncs.com/compatible-mode/v1";
    }
    if (name.rfind(L"glm-", 0) == 0) return L"https://open.bigmodel.cn/api/paas/v4";
    return {};
}

bool LooksLikeVisionModel(const std::wstring& model) {
    const std::wstring lowered = util::ToLower(model);
    auto contains = [&](const wchar_t* token) {
        return lowered.find(token) != std::wstring::npos;
    };
    return contains(L"vl") || contains(L"vision") || contains(L"gpt-4o") ||
           contains(L"gpt-4.1") || contains(L"glm-4v") || contains(L"claude") ||
           contains(L"gemini");
}

std::vector<ModelChoice> BuildModelChoices(const std::wstring& current,
                                           const std::wstring& currentBase) {
    std::vector<ModelChoice> choices = {
        {L"deepseek-chat", L"https://api.deepseek.com"},
        {L"deepseek-flash", L"https://api.deepseek.com"},
        {L"deepseek-reasoner", L"https://api.deepseek.com"},
        {L"gpt-4o-mini", L"https://api.openai.com/v1"},
        {L"gpt-4o", L"https://api.openai.com/v1"},
        {L"qwen-vl-max", L"https://dashscope.aliyuncs.com/compatible-mode/v1"},
        {L"qwen-vl-plus", L"https://dashscope.aliyuncs.com/compatible-mode/v1"},
        {L"glm-4v-plus", L"https://open.bigmodel.cn/api/paas/v4"},
        {L"glm-4-flash", L"https://open.bigmodel.cn/api/paas/v4"},
    };
    if (!current.empty() &&
        std::none_of(choices.begin(), choices.end(),
                     [&](const ModelChoice& choice) { return choice.name == current; })) {
        choices.insert(choices.begin(),
                       ModelChoice{current, currentBase.empty() ? BaseForModel(current)
                                                               : currentBase});
        if (choices.size() > 9) choices.pop_back();
    }
    return choices;
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

struct ToolsRects {
    ui::Rect tabs;
    ui::Rect functionTab;
    ui::Rect geometryTab;
    ui::Rect inputCard;
    ui::Rect functionField;
    ui::Rect keyboardButton;
    ui::Rect drawButton;
    ui::Rect aiButton;
    ui::Rect plotCard;
    ui::Rect aiCard;
    ui::Rect geometryInputCard;
    ui::Rect geometryField;
    ui::Rect geometryKeyboardButton;
    ui::Rect geometryGenerateButton;
    ui::Rect geometryCanvas;
    ui::Rect geometryAiCard;
};

ToolsRects ComputeToolsRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    ToolsRects rects;
    rects.tabs = ui::Rect{frame.x, frame.y, frame.w, 36.0f};
    rects.functionTab = ui::Rect{frame.x, frame.y, 112.0f, 34.0f};
    rects.geometryTab = ui::Rect{frame.x + 120.0f, frame.y, 112.0f, 34.0f};
    const float top = frame.y + 48.0f;
    rects.inputCard = ui::Rect{frame.x, top, frame.w, 92.0f};
    float fieldWidth = std::min(430.0f, frame.w * 0.44f);
    rects.functionField = ui::Rect{frame.x + 20.0f, top + 32.0f, fieldWidth, 40.0f};
    rects.keyboardButton =
        ui::Rect{rects.functionField.right() + 8.0f, top + 32.0f, 88.0f, 40.0f};
    rects.drawButton = ui::Rect{rects.keyboardButton.right() + 8.0f, top + 32.0f, 78.0f, 40.0f};
    rects.aiButton = ui::Rect{rects.drawButton.right() + 8.0f, top + 32.0f, 146.0f, 40.0f};
    const float contentTop = top + 104.0f;
    const float contentHeight = std::max(220.0f, height - kBottomMargin - contentTop);
    const float plotWidth = std::max(360.0f, frame.w * 0.63f);
    rects.plotCard = ui::Rect{frame.x, contentTop, plotWidth, contentHeight};
    rects.aiCard = ui::Rect{frame.x + plotWidth + 18.0f, contentTop,
                            frame.w - plotWidth - 18.0f, contentHeight};
    rects.geometryInputCard = ui::Rect{frame.x, top, frame.w, 108.0f};
    float geoFieldWidth = std::min(540.0f, frame.w * 0.56f);
    rects.geometryField = ui::Rect{frame.x + 20.0f, top + 32.0f, geoFieldWidth, 58.0f};
    rects.geometryKeyboardButton =
        ui::Rect{rects.geometryField.right() + 8.0f, top + 32.0f, 88.0f, 32.0f};
    rects.geometryGenerateButton =
        ui::Rect{rects.geometryField.right() + 8.0f, top + 68.0f, 160.0f, 32.0f};
    const float geoTop = top + 120.0f;
    const float geoHeight = std::max(220.0f, height - kBottomMargin - geoTop);
    const float canvasWidth = std::max(400.0f, frame.w * 0.64f);
    rects.geometryCanvas = ui::Rect{frame.x, geoTop, canvasWidth, geoHeight};
    rects.geometryAiCard = ui::Rect{frame.x + canvasWidth + 18.0f, geoTop,
                                    frame.w - canvasWidth - 18.0f, geoHeight};
    return rects;
}

struct ExamSetupRects {
    ui::Rect leftCard;
    ui::Rect rightCard;
    ui::Rect chatViewport;
    ui::Rect chatField;
    ui::Rect sendButton;
    ui::Rect generateButton;
    ui::Rect preview;
    ui::Rect startButton;
};

ExamSetupRects ComputeExamSetupRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    ExamSetupRects rects;
    float leftWidth = std::max(480.0f, frame.w * 0.57f);
    rects.leftCard = ui::Rect{frame.x, frame.y, leftWidth, frame.h};
    rects.rightCard = ui::Rect{frame.x + leftWidth + 18.0f, frame.y,
                               frame.w - leftWidth - 18.0f, frame.h};
    rects.chatViewport = ui::Rect{rects.leftCard.x + 24.0f, rects.leftCard.y + 72.0f,
                                  rects.leftCard.w - 48.0f, rects.leftCard.h - 250.0f};
    rects.chatField = ui::Rect{rects.leftCard.x + 24.0f,
                               rects.leftCard.bottom() - 148.0f,
                               rects.leftCard.w - 48.0f, 76.0f};
    rects.sendButton = ui::Rect{rects.chatField.right() - 98.0f,
                                rects.chatField.bottom() + 10.0f, 98.0f, 34.0f};
    rects.generateButton = ui::Rect{rects.leftCard.x + 24.0f,
                                    rects.chatField.bottom() + 10.0f, 180.0f, 34.0f};
    rects.preview = ui::Rect{rects.rightCard.x + 20.0f, rects.rightCard.y + 72.0f,
                             rects.rightCard.w - 40.0f, rects.rightCard.h - 190.0f};
    rects.startButton = ui::Rect{rects.rightCard.x + 20.0f,
                                 rects.rightCard.bottom() - 88.0f,
                                 rects.rightCard.w - 40.0f, 46.0f};
    return rects;
}

struct ExamRunnerRects {
    ui::Rect card;
    ui::Rect answerField;
    ui::Rect prevButton;
    ui::Rect nextButton;
    ui::Rect submitButton;
};

ExamRunnerRects ComputeExamRunnerRects(float width, float height) {
    FrameRects frame = ComputeFrame(width, height);
    ExamRunnerRects rects;
    rects.card = ui::Rect{frame.x, frame.y, frame.w, frame.h};
    rects.answerField = ui::Rect{frame.x + 36.0f, frame.y + 330.0f, frame.w - 72.0f, 120.0f};
    rects.submitButton = ui::Rect{frame.x + frame.w - 36.0f - 112.0f,
                                  frame.y + frame.h - 64.0f, 112.0f, 40.0f};
    rects.nextButton = ui::Rect{rects.submitButton.x - 10.0f - 92.0f,
                                frame.y + frame.h - 64.0f, 92.0f, 40.0f};
    rects.prevButton = ui::Rect{rects.nextButton.x - 8.0f - 92.0f,
                                frame.y + frame.h - 64.0f, 92.0f, 40.0f};
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
    Grade grade = Grade::Unknown;
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

struct ExamAiUpdate {
    bool generate = false;
    bool ok = false;
    std::string text;
    std::string error;
};

struct ToolAiUpdate {
    int kind = 0;  // 0 function, 1 geometry
    bool ok = false;
    std::string text;
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
    if (examThread_.joinable()) examThread_.join();
    if (toolThread_.joinable()) toolThread_.join();
    if (editFont_) DeleteObject(editFont_);
    if (whiteBrush_) DeleteObject(whiteBrush_);
    if (darkFieldBrush_) DeleteObject(darkFieldBrush_);
    if (canvasBrush_) DeleteObject(canvasBrush_);
    if (backDc_) {
        if (backOldBitmap_) SelectObject(backDc_, backOldBitmap_);
        DeleteDC(backDc_);
        backDc_ = nullptr;
    }
    if (backBitmap_) {
        DeleteObject(backBitmap_);
        backBitmap_ = nullptr;
    }
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
    ParseFunction();
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
                } else if (id == ID_FUNCTION_INPUT) {
                    ParseFunction();
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (id == ID_GEOMETRY_INPUT || id == ID_EXAM_CHAT ||
                           id == ID_EXAM_ANSWER) {
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
            } else if (wParam == kExamTimer) {
                if (examActive_ && !examSubmitted_) {
                    --examRemainingSeconds_;
                    if (examRemainingSeconds_ <= 0) {
                        examRemainingSeconds_ = 0;
                        SubmitExam();
                        ShowToast(L"考试时间到，已自动交卷。");
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else {
                    KillTimer(hwnd, kExamTimer);
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
            if (control && wParam >= '1' && wParam <= '6') {
                int index = static_cast<int>(wParam - '1');
                Page pages[] = {Page::Bank, Page::Tags, Page::Ai,
                                Page::Exam, Page::Tools, Page::Paper};
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
        case kMsgExamFinished:
            HandleExamFinished(wParam, lParam);
            return 0;
        case kMsgToolFinished:
            HandleToolFinished(wParam, lParam);
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
                    paperMinutesEdit_, paperNotesEdit_, functionEdit_, geometryEdit_,
                    examChatEdit_, examAnswerEdit_};
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
    functionEdit_ = createSingle(ID_FUNCTION_INPUT, L"例如 sin(x)、x^2-4、sqrt(x+1)", false);
    geometryEdit_ = createMultiline(ID_GEOMETRY_INPUT, false);
    examChatEdit_ = createMultiline(ID_EXAM_CHAT, false);
    examAnswerEdit_ = createMultiline(ID_EXAM_ANSWER, false);
    SetControlText(functionEdit_, L"sin(x)");
    SetControlText(geometryEdit_, L"三棱锥 P-ABC 中，PA⊥平面 ABC，AB⊥AC，PA=AB=AC=2。");
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
        PositionSingleLineEdit(apiKeyEdit_,
                               ui::Rect{ai.keyField.x, ai.keyField.y, ai.keyField.w - 68.0f,
                                        ai.keyField.h},
                               13.0f);
    }
    setVisible(apiBaseEdit_, showAi);
    setVisible(apiModelEdit_, false);
    setVisible(apiKeyEdit_, showAi && !modelMenuOpen_);

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

    const ToolsRects tools = ComputeToolsRects(width, height);
    const bool showFunction = page_ == Page::Tools && toolsTab_ == 0;
    if (showFunction) {
        functionFieldRect_ = tools.functionField;
        PositionSingleLineEdit(functionEdit_, tools.functionField, 14.0f);
    }
    setVisible(functionEdit_, showFunction);
    const bool showGeometry = page_ == Page::Tools && toolsTab_ == 1;
    if (showGeometry) {
        geometryFieldRect_ = tools.geometryField;
        PositionMultilineEdit(geometryEdit_, tools.geometryField, 8.0f);
    }
    setVisible(geometryEdit_, showGeometry);

    const bool showExamChat = page_ == Page::Exam && !examActive_ && !examSubmitted_;
    if (showExamChat) {
        const ExamSetupRects exam = ComputeExamSetupRects(width, height);
        examChatFieldRect_ = exam.chatField;
        PositionMultilineEdit(examChatEdit_, exam.chatField, 8.0f);
    }
    setVisible(examChatEdit_, showExamChat);
    bool showExamAnswer = false;
    if (page_ == Page::Exam && examActive_ && !examSubmitted_ && !examQuestions_.empty()) {
        const Question& current = examQuestions_[static_cast<size_t>(
            std::clamp(examIndex_, 0, static_cast<int>(examQuestions_.size()) - 1))];
        showExamAnswer = current.type != QuestionType::Choice;
        if (showExamAnswer) {
            const ExamRunnerRects exam = ComputeExamRunnerRects(width, height);
            examAnswerFieldRect_ = exam.answerField;
            PositionMultilineEdit(examAnswerEdit_, exam.answerField, 8.0f);
        }
    }
    setVisible(examAnswerEdit_, showExamAnswer);

    if (focused && !IsWindowVisible(focused) && focused != hwnd_) {
        SetFocus(hwnd_);
    }
    if ((focused == searchEdit_ && !showSearch) ||
        (focused == functionEdit_ && !showFunction) ||
        (focused == geometryEdit_ && !showGeometry) ||
        (focused == examChatEdit_ && !showExamChat) ||
        (focused == examAnswerEdit_ && !showExamAnswer)) {
        SetFocus(hwnd_);
    }
}

void App::ReadSettingsIntoFields() {
    const Settings& settings = store_.Config();
    SetControlText(apiBaseEdit_, W(settings.apiBase));
    SetControlText(apiModelEdit_, W(settings.model));
    SetControlText(apiKeyEdit_, W(settings.apiKey));
    modelMenuOpen_ = false;
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
    modelMenuOpen_ = false;
    CloseMathKeyboard();
    if (page == Page::Paper) EnsurePaperFields();
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::OpenMathKeyboard(HWND target) {
    if (target && IsWindow(target)) ShowMathKeyboard(hwnd_, target, theme_);
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
    saved.stem = InsertImplicitMultiplication(NormalizeMathText(util::Trim(saved.stem)));
    for (auto& option : saved.options) {
        option = InsertImplicitMultiplication(NormalizeMathText(util::Trim(option)));
    }
    saved.answer = InsertImplicitMultiplication(NormalizeMathText(util::Trim(saved.answer)));
    saved.analysis = InsertImplicitMultiplication(NormalizeMathText(util::Trim(saved.analysis)));
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
    if (!backBuffer_ || backBufferWidth_ != width || backBufferHeight_ != height) {
        backBuffer_ = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppRGB);
        if (backDc_) {
            if (backOldBitmap_) SelectObject(backDc_, backOldBitmap_);
            DeleteDC(backDc_);
            backDc_ = nullptr;
        }
        if (backBitmap_) {
            DeleteObject(backBitmap_);
            backBitmap_ = nullptr;
        }
        backBits_ = nullptr;
        BITMAPINFO bitmapInfo{};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = width;
        bitmapInfo.bmiHeader.biHeight = -height;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        backBitmap_ = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &backBits_, nullptr, 0);
        if (!backBitmap_) {
            EndPaint(hwnd_, &paint);
            return;
        }
        backDc_ = CreateCompatibleDC(dc);
        if (!backDc_) {
            DeleteObject(backBitmap_);
            backBitmap_ = nullptr;
            EndPaint(hwnd_, &paint);
            return;
        }
        backOldBitmap_ = SelectObject(backDc_, backBitmap_);
        backBufferWidth_ = width;
        backBufferHeight_ = height;
    }

    {
        Gdiplus::Graphics graphics(backBuffer_.get());
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
            case Page::Exam: PaintExam(canvas, logicalWidth, logicalHeight); break;
            case Page::Tools: PaintTools(canvas, logicalWidth, logicalHeight); break;
            case Page::Paper: PaintPaper(canvas, logicalWidth, logicalHeight); break;
        }
        PaintToast(canvas, logicalWidth, logicalHeight);

        Gdiplus::BitmapData data{};
        Gdiplus::Rect all(0, 0, width, height);
        if (backBits_ && backBuffer_->LockBits(&all, Gdiplus::ImageLockModeRead,
                                                PixelFormat32bppRGB, &data) == Gdiplus::Ok) {
            const size_t rowBytes = static_cast<size_t>(width) * 4;
            for (int y = 0; y < height; ++y) {
                std::memcpy(static_cast<unsigned char*>(backBits_) + rowBytes * y,
                            static_cast<const unsigned char*>(data.Scan0) + data.Stride * y,
                            rowBytes);
            }
            backBuffer_->UnlockBits(&data);
        }
        BitBlt(dc, 0, 0, width, height, backDc_, 0, 0, SRCCOPY);
    }
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
    const wchar_t* labels[] = {L"题库管理", L"标签容器", L"AI 接口",
                               L"AI 考试", L"函数工具", L"试卷合成"};
    const wchar_t* keys[] = {L"bank", L"tags", L"ai", L"exam", L"tools", L"paper"};
    const int active = static_cast<int>(page_);
    for (int i = 0; i < 6; ++i) {
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
    } else if (index == 3) {
        canvas.StrokeRect(ui::Rect{x + 2.0f, y + 1.0f, 14.0f, 16.0f}, color, 3.0f, 1.5f);
        canvas.Line(x + 9.0f, y + 1.5f, x + 9.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 9.0f, y + 6.0f, x + 15.0f, y + 6.0f, color, 1.2f);
        canvas.Line(x + 5.0f, y + 10.0f, x + 12.0f, y + 10.0f, color, 1.2f);
        canvas.Line(x + 5.0f, y + 13.0f, x + 10.0f, y + 13.0f, color, 1.2f);
    } else if (index == 4) {
        canvas.StrokeRect(ui::Rect{x + 2.0f, y + 1.0f, 14.0f, 16.0f}, color, 3.0f, 1.5f);
        canvas.Line(x + 5.0f, y + 6.0f, x + 9.0f, y + 10.0f, color, 1.4f);
        canvas.Line(x + 9.0f, y + 10.0f, x + 14.0f, y + 4.0f, color, 1.4f);
        canvas.Line(x + 5.0f, y + 13.0f, x + 12.0f, y + 13.0f, color, 1.2f);
    } else {
        canvas.Line(x + 1.0f, y + 15.0f, x + 17.0f, y + 15.0f, color, 1.2f);
        canvas.Line(x + 2.0f, y + 2.0f, x + 2.0f, y + 16.0f, color, 1.2f);
        canvas.Line(x + 3.0f, y + 11.0f, x + 6.0f, y + 7.0f, color, 1.5f);
        canvas.Line(x + 6.0f, y + 7.0f, x + 9.0f, y + 9.0f, color, 1.5f);
        canvas.Line(x + 9.0f, y + 9.0f, x + 14.0f, y + 3.0f, color, 1.5f);
        canvas.Line(x + 14.0f, y + 3.0f, x + 16.0f, y + 5.0f, color, 1.2f);
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
        case Page::Exam:
            title = L"AI 考试";
            subtitle = L"描述薄弱点，由 AI 诊断并从题库生成针对性练习卷；开始后计时作答";
            break;
        case Page::Tools:
            title = L"函数工具";
            subtitle = L"输入函数自动绘图、标记零点，并让 AI 分析定义域、值域与单调性";
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
        DrawButton(canvas, rects.imageButton, imageExtracting_ ? L"识别文件中…" : L"输入文件识题",
                   L"image_question", L"", imageExtracting_ ? 2 : 1, imageEnabled, 12.0f);
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
    } else if (page_ == Page::Paper) {
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
    int favoriteCount = 0;
    std::unordered_map<std::string, int> formulaTagUsage;
    std::unordered_map<std::string, int> conceptTagUsage;
    for (const auto& question : store_.Questions()) {
        ++difficultyCounts[static_cast<int>(question.difficulty)];
        ++typeCounts[static_cast<int>(question.type)];
        if (!question.tagged) ++untaggedCount;
        if (question.favorite) ++favoriteCount;
        for (const auto& tag : question.formulas) ++formulaTagUsage[TagKey(tag)];
        for (const auto& tag : question.concepts) ++conceptTagUsage[TagKey(tag)];
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
    bool tidyHovered = IsHovered(L"tidy_questions", L"");
    canvas.Text(L"整理", ui::Rect{filterInner.right() - 134.0f, filterInner.y + 5.0f, 36.0f, 18.0f},
                11.0f, tidyHovered ? col::AccentDark : col::Muted, true,
                Gdiplus::StringAlignmentFar);
    AddHit(ui::Rect{filterInner.right() - 138.0f, filterInner.y + 1.0f, 40.0f, 26.0f},
           L"tidy_questions");
    bool dedupHovered = IsHovered(L"dedup_questions", L"");
    canvas.Text(L"去重", ui::Rect{filterInner.right() - 88.0f, filterInner.y + 5.0f, 36.0f, 18.0f},
                11.0f, dedupHovered ? col::AccentDark : col::Muted, true,
                Gdiplus::StringAlignmentFar);
    AddHit(ui::Rect{filterInner.right() - 92.0f, filterInner.y + 1.0f, 40.0f, 26.0f},
           L"dedup_questions");
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

    canvas.Text(L"年级", ui::Rect{filterInner.x, filterInner.y + 194.0f, 80.0f, 16.0f}, 10.5f,
                col::Muted, true);
    const wchar_t* gradeLabels[] = {L"全部", L"高一", L"高二", L"高三"};
    const float gradeGap = 5.0f;
    const float gradeWidth = (filterInner.w - gradeGap * 3.0f) / 4.0f;
    for (int i = 0; i < 4; ++i) {
        ui::Rect chip{filterInner.x + static_cast<float>(i) * (gradeWidth + gradeGap),
                      filterInner.y + 213.0f, gradeWidth, 28.0f};
        bool selected = i == 0 ? filter_.grade < 0 : filter_.grade == i;
        std::wstring gradePayload = i == 0 ? L"all" : std::wstring(L"g") + std::to_wstring(i);
        filterChip(chip, gradeLabels[i], selected, L"chip_grade", gradePayload);
    }

    const float halfGap = 7.0f;
    const float halfWidth = (filterInner.w - halfGap) / 2.0f;
    ui::Rect favoriteChip{filterInner.x, filterInner.y + 250.0f, halfWidth, 27.0f};
    filterChip(favoriteChip,
               favoriteCount > 0 ? L"★ 收藏 " + IntText(favoriteCount) : L"★ 收藏",
               filter_.onlyFavorites, L"toggle_favorites", L"");
    ui::Rect untaggedChip{filterInner.x + halfWidth + halfGap, filterInner.y + 250.0f,
                          halfWidth, 27.0f};
    filterChip(untaggedChip, L"待标注 " + IntText(untaggedCount), filter_.onlyUntagged,
               L"toggle_untagged", L"");
    canvas.Line(filterInner.x, filterInner.y + 288.0f, filterInner.right(),
                filterInner.y + 288.0f, col::Border, 1.0f);

    canvas.Text(L"标签筛选", ui::Rect{filterInner.x, filterInner.y + 301.0f, 120.0f, 18.0f},
                12.0f, col::InkSoft, true);
    ui::Rect matchButton{filterInner.right() - 68.0f, filterInner.y + 299.0f, 68.0f, 24.0f};
    DrawButton(canvas, matchButton, filter_.matchAllTags ? L"全部匹配" : L"任一匹配",
               L"tag_match_mode", L"", 5, true, 10.0f);

    auto sortedTags = [&](const std::vector<std::string>& source, bool formulas) {
        std::vector<std::string> tags = source;
        const auto& usage = formulas ? formulaTagUsage : conceptTagUsage;
        std::stable_sort(tags.begin(), tags.end(), [&](const std::string& a, const std::string& b) {
            auto usageOf = [&](const std::string& tag) {
                auto it = usage.find(TagKey(tag));
                return it == usage.end() ? 0 : it->second;
            };
            const int ua = usageOf(a);
            const int ub = usageOf(b);
            if (ua != ub) return ua > ub;
            return a < b;
        });
        return tags;
    };
    const std::vector<std::string> formulaTags = sortedTags(store_.Tags().formulas, true);
    const std::vector<std::string> conceptTags = sortedTags(store_.Tags().concepts, false);

    const float tagViewportTop = filterInner.y + 330.0f;
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
        const bool visible = row.bottom() >= tagViewport.y && row.y <= tagViewport.bottom();
        const bool interactive = row.y >= tagViewport.y - 1.0f &&
                                 row.bottom() <= tagViewport.bottom() + 1.0f;
        if (visible) {
            bool hovered = interactive && IsHovered(L"filter_tag", W(tag));
            if (hovered) canvas.FillRect(row, col::Hover, 6.0f);
            ui::Rect box{row.x, row.y + 4.5f, 15.0f, 15.0f};
            bool checked = filter_.tags.count(tag) > 0;
            canvas.Checkbox(box, checked, hovered, false);
            canvas.Text(W(tag), ui::Rect{row.x + 23.0f, row.y, row.w - 60.0f, row.h}, 11.3f,
                        checked ? col::Ink : col::InkSoft, checked);
            canvas.Text(IntText(count), ui::Rect{row.right() - 32.0f, row.y, 28.0f, row.h},
                        9.8f, col::Faint, false, Gdiplus::StringAlignmentFar);
        }
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
                auto it = formulaTagUsage.find(TagKey(tag));
                drawTagRow(tag, it == formulaTagUsage.end() ? 0 : it->second);
            }
            tagY += 8.0f;
        }
        if (!conceptTags.empty()) {
            drawGroupTitle(L"知识点标签", static_cast<int>(conceptTags.size()),
                           col::ConceptTagText);
            for (const auto& tag : conceptTags) {
                auto it = conceptTagUsage.find(TagKey(tag));
                drawTagRow(tag, it == conceptTagUsage.end() ? 0 : it->second);
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
                                          L" 个文件并提取所有题目…"
                                    : L"AI 正在读取文件并提取所有题目…";
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
    topX += typeWidth + 6.0f;
    std::wstring gradeText = GradeName(question.grade);
    float gradeWidth = canvas.Measure(gradeText, 10.0f, true, false).Width + 18.0f;
    const bool hasGrade = question.grade != Grade::Unknown;
    canvas.FillRect(ui::Rect{topX, rect.y + 14.0f, gradeWidth, 22.0f},
                    hasGrade ? col::GreenSoft : col::CardAlt, 11.0f);
    canvas.Text(gradeText, ui::Rect{topX, rect.y + 14.0f, gradeWidth, 22.0f}, 10.0f,
                hasGrade ? col::Green : col::Faint, hasGrade,
                Gdiplus::StringAlignmentCenter);

    ui::Rect editButton{rect.right() - 64.0f, rect.y + 12.0f, 46.0f, 26.0f};
    ui::Rect starButton{editButton.x - 32.0f, rect.y + 12.0f, 26.0f, 26.0f};
    bool starHovered = interactive && IsHovered(L"toggle_favorite_question", W(question.id));
    if (starHovered) canvas.FillRect(starButton, col::AmberSoft, 7.0f);
    canvas.Text(question.favorite ? L"★" : L"☆", starButton, 15.0f,
                question.favorite ? col::Amber : col::Faint, false,
                Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
    if (interactive) AddHit(starButton, L"toggle_favorite_question", W(question.id));
    std::wstring status = question.tagged ? L"AI 已标注" : L"待标注";
    float statusTextWidth = canvas.Measure(status, 10.2f, false, false).Width;
    float statusStart = starButton.x - 10.0f - statusTextWidth - 10.0f;
    canvas.Circle(statusStart, rect.y + 26.0f, 3.2f,
                  question.tagged ? col::Green : col::Amber);
    canvas.Text(status, ui::Rect{statusStart + 9.0f, rect.y + 16.0f,
                                 statusTextWidth + 2.0f, 20.0f},
                10.2f, question.tagged ? col::Green : col::Amber, false);
    DrawButton(canvas, editButton, L"编辑", interactive ? L"edit_question" : L"",
               W(question.id), 2, true, 10.5f);

    float stemX = rect.x + 48.0f;
    float stemWidth = rect.w - 48.0f - 22.0f;
    if (!mathdraw::DrawLine(canvas, W(question.stem),
                            ui::Rect{stemX, rect.y + 41.0f, stemWidth, 38.0f}, 13.8f,
                            col::Ink, false)) {
        canvas.TextBlock(W(question.stem), ui::Rect{stemX, rect.y + 41.0f, stemWidth, 38.0f},
                         13.8f, col::Ink, 2, false);
    }

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
    std::unordered_map<std::string, int> formulaTagUsage;
    std::unordered_map<std::string, int> conceptTagUsage;
    for (const auto& question : store_.Questions()) {
        if (question.tagged) ++taggedQuestions;
        for (const auto& tag : question.formulas) ++formulaTagUsage[TagKey(tag)];
        for (const auto& tag : question.concepts) ++conceptTagUsage[TagKey(tag)];
    }
    int usedFormulas = 0;
    int usedConcepts = 0;
    for (const auto& tag : store_.Tags().formulas) {
        auto it = formulaTagUsage.find(TagKey(tag));
        if (it != formulaTagUsage.end() && it->second > 0) ++usedFormulas;
    }
    for (const auto& tag : store_.Tags().concepts) {
        auto it = conceptTagUsage.find(TagKey(tag));
        if (it != conceptTagUsage.end() && it->second > 0) ++usedConcepts;
    }

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
        const auto& usage = formulasColumn ? formulaTagUsage : conceptTagUsage;
        std::stable_sort(tags.begin(), tags.end(), [&](const std::string& a, const std::string& b) {
            auto usageOf = [&](const std::string& tag) {
                auto it = usage.find(TagKey(tag));
                return it == usage.end() ? 0 : it->second;
            };
            const int ua = usageOf(a);
            const int ub = usageOf(b);
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
            const auto& usageMap = column.formulas ? formulaTagUsage : conceptTagUsage;
            auto usageIt = usageMap.find(TagKey(column.tags[i]));
            int usage = usageIt == usageMap.end() ? 0 : usageIt->second;
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
    {
        const std::wstring currentModel = GetControlText(apiModelEdit_);
        const bool modelHovered = IsHovered(L"model_menu");
        canvas.FillRect(rects.modelField, col::Card, 10.0f);
        canvas.StrokeRect(rects.modelField,
                          modelMenuOpen_ || modelHovered ? col::Accent : col::BorderStrong,
                          10.0f, modelMenuOpen_ ? 1.6f : 1.0f);
        canvas.Text(currentModel.empty() ? L"请选择模型" : currentModel,
                    ui::Rect{rects.modelField.x + 14.0f, rects.modelField.y,
                             rects.modelField.w - 44.0f, rects.modelField.h},
                    12.0f, currentModel.empty() ? col::Faint : col::InkSoft, false);
        canvas.Triangle(rects.modelField.right() - 22.0f, rects.modelField.y + 18.0f,
                        rects.modelField.right() - 12.0f, rects.modelField.y + 18.0f,
                        rects.modelField.right() - 17.0f, rects.modelField.y + 25.0f,
                        modelMenuOpen_ ? col::Accent : col::Muted);
        AddHit(rects.modelField, L"model_menu");
    }
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
        {L"01", L"选择或上传", L"勾选题目做标注，或点题库右上角“输入文件识题”上传图片/文档。", col::Accent},
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

    if (modelMenuOpen_) {
        AddHit(ui::Rect{0, 0, width, height}, L"close_model_menu");
        AddHit(rects.modelField, L"model_menu");
        std::wstring currentModel = GetControlText(apiModelEdit_);
        std::wstring currentBase = GetControlText(apiBaseEdit_);
        std::vector<ModelChoice> choices = BuildModelChoices(currentModel, currentBase);
        const float itemH = 26.0f;
        const float pad = 5.0f;
        const float totalHeight = pad * 2.0f + itemH * static_cast<float>(choices.size());
        float menuTop = rects.modelField.bottom() + 4.0f;
        if (menuTop + totalHeight > rects.leftCard.bottom() - 8.0f) {
            menuTop = rects.modelField.y - totalHeight - 4.0f;
        }
        ui::Rect menu{rects.modelField.x, menuTop, rects.modelField.w, totalHeight};
        canvas.DrawShadow(menu, 10.0f);
        canvas.FillRect(menu, col::Card, 10.0f);
        canvas.StrokeRect(menu, col::BorderStrong, 10.0f, 1.0f);
        float rowY = menu.y + pad;
        for (const auto& choice : choices) {
            ui::Rect row{menu.x + 5.0f, rowY, menu.w - 10.0f, itemH - 2.0f};
            bool selected = choice.name == currentModel;
            bool hovered = IsHovered(L"select_model", choice.name);
            if (selected) canvas.FillRect(row, col::AccentSoft, 6.0f);
            else if (hovered) canvas.FillRect(row, col::Hover, 6.0f);
            if (selected) {
                canvas.Text(L"✓", ui::Rect{row.x + 8.0f, row.y, 16.0f, row.h}, 11.0f,
                            col::Accent, true, Gdiplus::StringAlignmentCenter);
            }
            canvas.Text(choice.name,
                        ui::Rect{row.x + 30.0f, row.y, row.w - 38.0f, row.h}, 11.5f,
                        selected ? col::Accent : col::InkSoft, selected);
            AddHit(row, L"select_model", choice.name);
            rowY += itemH;
        }
    }
}

std::wstring ClockText(int seconds) {
    seconds = std::max(0, seconds);
    return (seconds / 60 < 10 ? L"0" : L"") + std::to_wstring(seconds / 60) + L":" +
           (seconds % 60 < 10 ? L"0" : L"") + std::to_wstring(seconds % 60);
}

std::string AnswerKey(const std::string& text) {
    std::string key = util::ToLowerAscii(util::Trim(text));
    std::string normalized;
    for (unsigned char c : key) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',' || c == ';' ||
            c == ':') {
            continue;
        }
        normalized.push_back(static_cast<char>(c));
    }
    return normalized;
}

std::string ExtractGroundedJson(const std::string& text) {
    std::string source = text;
    size_t first = source.find('{');
    size_t last = source.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last <= first) return {};
    return source.substr(first, last - first + 1);
}

void App::SendExamMessage() {
    if (examChatBusy_ || examGenerating_) return;
    std::wstring message = util::Trim(GetControlText(examChatEdit_));
    if (message.empty()) return;
    Settings settings = store_.Config();
    if (util::Trim(settings.model).empty()) {
        ShowToast(L"请先在 AI 接口中配置模型。", true);
        return;
    }
    examMessages_.push_back(ExamMessage{true, message});
    SetControlText(examChatEdit_, L"");
    examChatBusy_ = true;
    std::ostringstream conversation;
    for (size_t i = examMessages_.size() > 8 ? examMessages_.size() - 8 : 0;
         i < examMessages_.size(); ++i) {
        conversation << (examMessages_[i].user ? "学生：" : "AI：")
                     << util::ToUtf8(examMessages_[i].text) << "\n";
    }
    std::string prompt =
        "以下是我和学生的对话：\n" + conversation.str() +
        "\n请先用 3-5 句话判断学生的薄弱点，并追问 1-2 个问题以便组卷。"
        "只输出中文正文，不要 Markdown。";
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    examThread_ = std::thread([target, settings, prompt]() {
        AiChatResult result = AskAiChat(
            settings, "你是一名有经验的高中数学教研员，正在通过对话诊断学生薄弱点。",
            prompt, 900, 120000);
        auto* update = new ExamAiUpdate{false, result.ok, result.text, result.error};
        if (!PostMessageW(target, kMsgExamFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::GenerateExamPaper() {
    if (examChatBusy_ || examGenerating_) return;
    if (examMessages_.empty()) {
        ShowToast(L"请先描述自己的薄弱点，让 AI 了解情况。", true);
        SetFocus(examChatEdit_);
        return;
    }
    Settings settings = store_.Config();
    if (util::Trim(settings.model).empty()) {
        ShowToast(L"请先在 AI 接口中配置模型。", true);
        return;
    }
    std::ostringstream conversation;
    for (const auto& message : examMessages_) {
        conversation << (message.user ? "学生：" : "AI：") << util::ToUtf8(message.text) << "\n";
    }
    std::ostringstream catalog;
    for (const auto& question : store_.Questions()) {
        if (catalog.tellp() > 14000) break;
        catalog << "[" << question.id << "] " << util::ToUtf8(GradeName(question.grade))
                << " " << util::ToUtf8(DifficultyName(question.difficulty)) << " "
                << util::ToUtf8(TypeName(question.type)) << " | ";
        for (const auto& tag : question.formulas) catalog << tag << "、";
        for (const auto& tag : question.concepts) catalog << tag << "、";
        catalog << " | " << ClampPreviewText(question.stem, 110) << "\n";
    }
    std::string prompt =
        "学生与 AI 的对话：\n" + conversation.str() +
        "\n题库目录（id 年级 难度 题型 | 标签 | 题干）：\n" + catalog.str() +
        "\n请从题库中选择 5-8 道最适合该学生的题，覆盖其薄弱点，并保持难度梯度。"
        "只输出 JSON：{\"title\":\"针对性练习标题\",\"minutes\":30,\"ids\":[\"题目id\"]}。"
        "id 必须来自目录，不要编造。";
    examGenerating_ = true;
    examStatus_ = L"AI 正在根据薄弱点组卷…";
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    examThread_ = std::thread([target, settings, prompt]() {
        AiChatResult result = AskAiChat(
            settings, "你是一名高中数学教研员，只返回用户要求的 JSON。", prompt, 900,
            120000);
        auto* update = new ExamAiUpdate{true, result.ok, result.text, result.error};
        if (!PostMessageW(target, kMsgExamFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::HandleExamFinished(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    std::unique_ptr<ExamAiUpdate> update(reinterpret_cast<ExamAiUpdate*>(lParam));
    if (examThread_.joinable()) examThread_.join();
    if (!update) return;
    if (!update->generate) {
        examChatBusy_ = false;
        examMessages_.push_back(ExamMessage{false, update->ok ? W(update->text)
                                                              : L"AI 请求失败：" + W(update->error)});
    } else {
        examGenerating_ = false;
        std::vector<Question> selected;
        std::wstring title = L"薄弱点专项练习";
        int minutes = 30;
        std::string json = ExtractGroundedJson(update->text);
        jsonx::Value root = json.empty() ? jsonx::Value{} : jsonx::Value::Parse(json);
        if (root.IsObject()) {
            std::wstring parsedTitle = W(root["title"].AsString());
            if (!parsedTitle.empty()) title = parsedTitle;
            minutes = util::ClampInt(root["minutes"].AsInt(30), 5, 180);
            const jsonx::Value& ids = root["ids"];
            if (ids.IsArray()) {
                for (const auto& item : ids.Items()) {
                    if (!item.IsString()) continue;
                    Question* question = store_.FindQuestion(item.AsString());
                    if (!question) continue;
                    if (std::any_of(selected.begin(), selected.end(),
                                    [&](const Question& old) { return old.id == question->id; })) {
                        continue;
                    }
                    selected.push_back(*question);
                    if (selected.size() >= 10) break;
                }
            }
        }
        if (selected.size() < 3) {
            std::string weakness;
            for (const auto& message : examMessages_) {
                if (message.user) weakness += util::ToLowerAscii(util::ToUtf8(message.text)) + "\n";
            }
            struct Scored {
                int score = 0;
                const Question* question = nullptr;
            };
            std::vector<Scored> scored;
            for (const auto& question : store_.Questions()) {
                int score = 0;
                for (const auto& tag : question.AllTags()) {
                    if (!util::Trim(tag).empty() &&
                        weakness.find(util::ToLowerAscii(tag)) != std::string::npos) {
                        score += 6;
                    }
                }
                std::string grade = util::ToLowerAscii(util::ToUtf8(GradeName(question.grade)));
                if (!grade.empty() && weakness.find(grade) != std::string::npos) score += 3;
                if (question.difficulty == Difficulty::B) ++score;
                scored.push_back(Scored{score, &question});
            }
            std::stable_sort(scored.begin(), scored.end(),
                             [](const Scored& a, const Scored& b) { return a.score > b.score; });
            for (const auto& entry : scored) {
                if (selected.size() >= 7) break;
                if (std::any_of(selected.begin(), selected.end(),
                                [&](const Question& old) { return old.id == entry.question->id; })) {
                    continue;
                }
                selected.push_back(*entry.question);
            }
            title = L"薄弱点专项练习";
            minutes = 30;
            examStatus_ = update->ok ? L"AI 选择不足，已按标签补充本地组卷。"
                                     : L"AI 组卷失败，已按薄弱点生成默认练习卷。";
        } else {
            examStatus_ = L"AI 已生成针对性练习卷。";
        }
        if (selected.empty()) {
            examStatus_ = L"题库为空，无法生成练习卷。";
        } else {
            examGenerated_ = std::move(selected);
            examGeneratedTitle_ = std::move(title);
            examGeneratedMinutes_ = minutes;
        }
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::StartExam() {
    if (examGenerated_.empty()) return;
    examQuestions_ = examGenerated_;
    examTitle_ = examGeneratedTitle_;
    examMinutes_ = examGeneratedMinutes_;
    examRemainingSeconds_ = examMinutes_ * 60;
    examActive_ = true;
    examSubmitted_ = false;
    examIndex_ = 0;
    examScore_ = 0;
    examCorrect_ = 0;
    examTotalPoints_ = 0;
    examChoices_.assign(examQuestions_.size(), -1);
    examAnswers_.assign(examQuestions_.size(), {});
    SetTimer(hwnd_, kExamTimer, 1000, nullptr);
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::SaveCurrentExamAnswer() {
    if (!examActive_ || examQuestions_.empty()) return;
    if (examIndex_ < 0 || examIndex_ >= static_cast<int>(examQuestions_.size())) return;
    if (examQuestions_[examIndex_].type != QuestionType::Choice) {
        examAnswers_[static_cast<size_t>(examIndex_)] =
            util::Trim(GetControlText(examAnswerEdit_).empty()
                           ? ""
                           : util::ToUtf8(GetControlText(examAnswerEdit_)));
    }
}

void App::LoadExamAnswerToEdit() {
    if (!examAnswerEdit_ || examQuestions_.empty()) return;
    if (examIndex_ < 0 || examIndex_ >= static_cast<int>(examQuestions_.size())) return;
    SetControlText(examAnswerEdit_,
                   W(examAnswers_[static_cast<size_t>(examIndex_)]));
}

void App::SubmitExam() {
    if (!examActive_) return;
    SaveCurrentExamAnswer();
    examActive_ = false;
    examSubmitted_ = true;
    KillTimer(hwnd_, kExamTimer);
    examScore_ = 0;
    examCorrect_ = 0;
    examTotalPoints_ = 0;
    for (size_t i = 0; i < examQuestions_.size(); ++i) {
        const Question& question = examQuestions_[i];
        int points = PointsForType(question.type);
        examTotalPoints_ += points;
        bool correct = false;
        if (question.type == QuestionType::Choice) {
            int index = examChoices_[i];
            if (index >= 0 && index < static_cast<int>(question.options.size())) {
                char selected = static_cast<char>('A' + index);
                std::string answer = AnswerKey(question.answer);
                correct = !answer.empty() && answer[0] == selected;
            }
        } else if (question.type == QuestionType::Fill) {
            correct = AnswerKey(question.answer) == AnswerKey(examAnswers_[i]);
        }
        if (correct) {
            ++examCorrect_;
            examScore_ += points;
        }
    }
    examReviewIndex_ = 0;
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ResetExam() {
    examActive_ = false;
    examSubmitted_ = false;
    KillTimer(hwnd_, kExamTimer);
    examQuestions_.clear();
    examAnswers_.clear();
    examChoices_.clear();
    examScore_ = 0;
    examCorrect_ = 0;
    examIndex_ = 0;
    examReviewIndex_ = 0;
    LayoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::PaintExam(ui::Canvas& canvas, float width, float height) {
    if (examSubmitted_) {
        PaintExamResult(canvas, width, height);
    } else if (examActive_) {
        PaintExamRunner(canvas, width, height);
    } else {
        PaintExamSetup(canvas, width, height);
    }
}

void App::PaintExamSetup(ui::Canvas& canvas, float width, float height) {
    const ExamSetupRects rects = ComputeExamSetupRects(width, height);
    canvas.DrawShadow(rects.leftCard, 16.0f);
    canvas.FillRect(rects.leftCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.leftCard, col::Border, 16.0f, 1.0f);
    canvas.Text(L"薄弱点诊断", ui::Rect{rects.leftCard.x + 24.0f, rects.leftCard.y + 20.0f,
                                         240.0f, 26.0f},
                17.0f, col::Ink, true);
    canvas.Text(L"和 AI 说说最近哪一块最吃力，越具体越容易组出合适的题。",
                ui::Rect{rects.leftCard.x + 24.0f, rects.leftCard.y + 48.0f,
                         rects.leftCard.w - 48.0f, 20.0f},
                10.2f, col::Muted, false);

    canvas.FillRect(rects.chatViewport, col::CardAlt, 12.0f);
    canvas.StrokeRect(rects.chatViewport, col::Border, 12.0f, 1.0f);
    canvas.PushClip(rects.chatViewport);
    if (examMessages_.empty()) {
        canvas.TextWrapped(
            L"例如：\n“我导数含参分类讨论总是漏情况，圆锥曲线联立后算不出来，"
            L"选择题速度也慢。”\n\nAI 会先追问，再根据你的描述从题库选题成卷。",
            ui::Rect{rects.chatViewport.x + 18.0f, rects.chatViewport.y + 18.0f,
                     rects.chatViewport.w - 36.0f, rects.chatViewport.h - 30.0f},
            11.0f, col::Faint, false, Gdiplus::StringAlignmentNear,
            Gdiplus::StringAlignmentNear);
    } else {
        int visible = std::max(2, static_cast<int>(rects.chatViewport.h / 66.0f));
        int begin = std::max(0, static_cast<int>(examMessages_.size()) - visible);
        float y = rects.chatViewport.y + 10.0f;
        for (int i = begin; i < static_cast<int>(examMessages_.size()); ++i) {
            const ExamMessage& message = examMessages_[static_cast<size_t>(i)];
            ui::Rect bubble{rects.chatViewport.x + 12.0f, y, rects.chatViewport.w - 24.0f, 58.0f};
            canvas.FillRect(bubble, message.user ? col::AccentSoft : col::Card, 9.0f);
            canvas.Text(message.user ? L"你" : L"AI 教研员",
                        ui::Rect{bubble.x + 12.0f, bubble.y + 5.0f, 100.0f, 15.0f}, 9.0f,
                        message.user ? col::Accent : col::Blue, true);
            canvas.TextBlock(message.text,
                             ui::Rect{bubble.x + 12.0f, bubble.y + 21.0f, bubble.w - 24.0f,
                                      35.0f},
                             10.2f, col::InkSoft, 2, false);
            y += 64.0f;
        }
    }
    canvas.PopClip();

    canvas.Text(examChatBusy_ ? L"AI 正在诊断…" : examStatus_,
                ui::Rect{rects.leftCard.x + 24.0f, rects.chatField.y - 24.0f,
                         rects.leftCard.w - 48.0f, 18.0f},
                9.8f, examChatBusy_ ? col::Accent : col::Faint, false);
    DrawFieldBackground(canvas, rects.chatField, examChatEdit_);
    AddHit(rects.chatField, L"focus_exam_chat");
    DrawButton(canvas, rects.generateButton, L"AI 生成试卷", L"exam_generate", L"", 5,
               !examChatBusy_ && !examGenerating_ && !examMessages_.empty(), 11.0f);
    DrawButton(canvas, rects.sendButton, L"发送", L"exam_send", L"", 0,
               !examChatBusy_ && !examGenerating_, 11.0f);

    canvas.DrawShadow(rects.rightCard, 16.0f);
    canvas.FillRect(rects.rightCard, col::Card, 16.0f);
    canvas.StrokeRect(rects.rightCard, col::Border, 16.0f, 1.0f);
    canvas.Text(L"针对性练习卷", ui::Rect{rects.rightCard.x + 20.0f, rects.rightCard.y + 20.0f,
                                           260.0f, 26.0f},
                17.0f, col::Ink, true);
    canvas.Text(L"由 AI 从题库中挑选，保留题目原始答案与解析。",
                ui::Rect{rects.rightCard.x + 20.0f, rects.rightCard.y + 48.0f,
                         rects.rightCard.w - 40.0f, 18.0f},
                9.8f, col::Muted, false);
    canvas.FillRect(rects.preview, col::CardAlt, 12.0f);
    canvas.StrokeRect(rects.preview, col::Border, 12.0f, 1.0f);
    if (examGenerated_.empty()) {
        canvas.TextWrapped(
            L"先和 AI 对话，再点“AI 生成试卷”。\n\n生成后会显示题目数量、预计用时和难度梯度，"
            L"确认无误再开始计时作答。",
            ui::Rect{rects.preview.x + 18.0f, rects.preview.y + 18.0f,
                     rects.preview.w - 36.0f, rects.preview.h - 30.0f},
            11.0f, col::Faint, false, Gdiplus::StringAlignmentNear,
            Gdiplus::StringAlignmentNear);
    } else {
        canvas.Text(examGeneratedTitle_,
                    ui::Rect{rects.preview.x + 18.0f, rects.preview.y + 14.0f,
                             rects.preview.w - 36.0f, 22.0f},
                    13.0f, col::Ink, true);
        canvas.Text(IntText(static_cast<int>(examGenerated_.size())) + L" 道题 · " +
                        IntText(examGeneratedMinutes_) + L" 分钟",
                    ui::Rect{rects.preview.x + 18.0f, rects.preview.y + 40.0f,
                             rects.preview.w - 36.0f, 18.0f},
                    10.0f, col::Muted, false);
        float rowY = rects.preview.y + 72.0f;
        int shown = 0;
        for (const auto& question : examGenerated_) {
            if (shown >= 7 || rowY > rects.preview.bottom() - 28.0f) break;
            canvas.Circle(rects.preview.x + 24.0f, rowY + 10.0f, 3.0f, col::Accent);
            canvas.Text(IntText(shown + 1) + L". [" +
                            std::wstring(GradeName(question.grade)) + L"/" +
                            DifficultyName(question.difficulty) + L"] ",
                        ui::Rect{rects.preview.x + 34.0f, rowY, rects.preview.w - 50.0f, 20.0f},
                        9.8f, col::InkSoft, false, Gdiplus::StringAlignmentNear);
            canvas.TextEllipsis(W(question.stem),
                                ui::Rect{rects.preview.x + 34.0f, rowY + 19.0f,
                                         rects.preview.w - 52.0f, 18.0f},
                                9.5f, col::Muted, false);
            rowY += 42.0f;
            ++shown;
        }
    }
    DrawButton(canvas, rects.startButton, L"开始考试", L"exam_start", L"", 0,
               !examGenerated_.empty() && !examChatBusy_ && !examGenerating_, 13.0f);
    if (examGenerating_) {
        canvas.Text(L"AI 正在组卷…", ui::Rect{rects.startButton.x,
                                               rects.startButton.y - 24.0f,
                                               rects.startButton.w, 18.0f},
                    9.8f, col::Accent, false, Gdiplus::StringAlignmentCenter);
    }
}

void App::PaintExamRunner(ui::Canvas& canvas, float width, float height) {
    if (examQuestions_.empty()) return;
    const ExamRunnerRects rects = ComputeExamRunnerRects(width, height);
    examIndex_ = std::clamp(examIndex_, 0, static_cast<int>(examQuestions_.size()) - 1);
    const Question& question = examQuestions_[static_cast<size_t>(examIndex_)];
    canvas.DrawShadow(rects.card, 16.0f);
    canvas.FillRect(rects.card, col::Card, 16.0f);
    canvas.StrokeRect(rects.card, col::Border, 16.0f, 1.0f);
    canvas.Text(examTitle_, ui::Rect{rects.card.x + 36.0f, rects.card.y + 22.0f,
                                      rects.card.w - 260.0f, 28.0f},
                18.0f, col::Ink, true);
    canvas.Text(ClockText(examRemainingSeconds_),
                ui::Rect{rects.card.right() - 170.0f, rects.card.y + 16.0f, 130.0f, 30.0f},
                22.0f, col::Accent, true, Gdiplus::StringAlignmentFar);
    canvas.Text(L"剩余时间", ui::Rect{rects.card.right() - 170.0f, rects.card.y + 46.0f,
                                       130.0f, 16.0f},
                9.0f, col::Faint, false, Gdiplus::StringAlignmentFar);
    float progress = static_cast<float>(examIndex_ + 1) /
                     static_cast<float>(examQuestions_.size());
    canvas.FillRect(ui::Rect{rects.card.x + 36.0f, rects.card.y + 64.0f,
                              rects.card.w - 72.0f, 5.0f},
                    col::Track, 2.5f);
    canvas.FillRect(ui::Rect{rects.card.x + 36.0f, rects.card.y + 64.0f,
                              (rects.card.w - 72.0f) * progress, 5.0f},
                    col::Accent, 2.5f);
    canvas.Text(L"第 " + IntText(examIndex_ + 1) + L" / " +
                    IntText(static_cast<int>(examQuestions_.size())) + L" 题",
                ui::Rect{rects.card.x + 36.0f, rects.card.y + 78.0f, 200.0f, 18.0f}, 10.0f,
                col::Muted, false);

    float top = rects.card.y + 108.0f;
    canvas.FillRect(ui::Rect{rects.card.x + 36.0f, top, 46.0f, 24.0f},
                    DifficultyBackground(question.difficulty), 12.0f);
    canvas.Text(DifficultyPillText(question.difficulty),
                ui::Rect{rects.card.x + 36.0f, top, 46.0f, 24.0f}, 10.0f,
                DifficultyForeground(question.difficulty), true,
                Gdiplus::StringAlignmentCenter);
    canvas.Text(TypeName(question.type), ui::Rect{rects.card.x + 90.0f, top, 80.0f, 24.0f},
                10.2f, col::Muted, false);
    canvas.Text(GradeName(question.grade), ui::Rect{rects.card.x + 166.0f, top, 60.0f, 24.0f},
                10.2f, col::Green, true);
    if (!mathdraw::DrawLine(canvas, W(question.stem),
                            ui::Rect{rects.card.x + 36.0f, top + 36.0f, rects.card.w - 72.0f,
                                     160.0f},
                            15.0f, col::Ink, false)) {
        canvas.TextBlock(W(question.stem),
                         ui::Rect{rects.card.x + 36.0f, top + 36.0f, rects.card.w - 72.0f,
                                  160.0f},
                         15.0f, col::Ink, 5, false);
    }

    if (question.type == QuestionType::Choice) {
        float optionTop = top + 190.0f;
        float optionWidth = (rects.card.w - 88.0f) * 0.5f;
        for (int i = 0; i < static_cast<int>(question.options.size()) && i < 4; ++i) {
            float x = rects.card.x + 36.0f + (i % 2) * (optionWidth + 16.0f);
            float y = optionTop + (i / 2) * 68.0f;
            ui::Rect option{x, y, optionWidth, 54.0f};
            bool selected = examChoices_[static_cast<size_t>(examIndex_)] == i;
            bool hovered = IsHovered(L"exam_choice", std::to_wstring(i));
            canvas.FillRect(option, selected ? col::AccentSoft : (hovered ? col::Hover : col::CardAlt),
                            11.0f);
            canvas.StrokeRect(option, selected ? col::Accent : col::Border, 11.0f,
                              selected ? 1.4f : 1.0f);
            canvas.Text(std::wstring(1, static_cast<wchar_t>('A' + i)),
                        ui::Rect{option.x + 14.0f, option.y, 22.0f, option.h}, 12.0f,
                        selected ? col::Accent : col::Muted, true,
                        Gdiplus::StringAlignmentCenter);
            canvas.TextEllipsis(W(question.options[static_cast<size_t>(i)]),
                                ui::Rect{option.x + 42.0f, option.y, option.w - 54.0f, option.h},
                                12.0f, selected ? col::Ink : col::InkSoft, false);
            AddHit(option, L"exam_choice", IntText(i));
        }
    } else {
        canvas.Text(L"作答", ui::Rect{rects.answerField.x, rects.answerField.y - 28.0f, 80.0f,
                                       20.0f},
                    11.0f, col::InkSoft, true);
        DrawFieldBackground(canvas, rects.answerField, examAnswerEdit_);
        AddHit(rects.answerField, L"focus_exam_answer");
        ui::Rect keyboard{rects.answerField.right() - 118.0f, rects.answerField.y - 34.0f,
                          110.0f, 30.0f};
        DrawButton(canvas, keyboard, L"⌨ 数学键盘", L"exam_keyboard", L"", 2, true, 10.0f);
        canvas.Text(question.type == QuestionType::Solution
                        ? L"解答题请写出关键步骤；交卷后可对照参考答案自评。"
                        : L"填写最终答案即可，支持数学键盘输入。",
                    ui::Rect{rects.answerField.x, rects.answerField.bottom() + 8.0f,
                             rects.answerField.w, 18.0f},
                    9.5f, col::Faint, false);
    }

    DrawButton(canvas, rects.prevButton, L"上一题", L"exam_prev", L"", 2, examIndex_ > 0,
               11.0f);
    DrawButton(canvas, rects.nextButton, L"下一题", L"exam_next", L"", 2,
               examIndex_ + 1 < static_cast<int>(examQuestions_.size()), 11.0f);
    DrawButton(canvas, rects.submitButton, L"交卷", L"exam_submit", L"", 0, true, 12.0f);
}

void App::PaintExamResult(ui::Canvas& canvas, float width, float height) {
    const FrameRects frame = ComputeFrame(width, height);
    const ui::Rect card{frame.x, frame.y, frame.w, frame.h};
    canvas.DrawShadow(card, 16.0f);
    canvas.FillRect(card, col::Card, 16.0f);
    canvas.StrokeRect(card, col::Border, 16.0f, 1.0f);
    canvas.Text(L"考试结果", ui::Rect{frame.x + 36.0f, frame.y + 30.0f, 300.0f, 30.0f}, 22.0f,
                col::Ink, true);
    canvas.Text(examTitle_, ui::Rect{frame.x + 36.0f, frame.y + 66.0f, 600.0f, 22.0f}, 12.0f,
                col::Muted, false);
    canvas.Text(IntText(examScore_) + L" 分",
                ui::Rect{frame.x + frame.w - 320.0f, frame.y + 30.0f, 200.0f, 46.0f}, 32.0f,
                col::Accent, true, Gdiplus::StringAlignmentFar);
    canvas.Text(L"满分 " + IntText(examTotalPoints_) + L" 分",
                ui::Rect{frame.x + frame.w - 320.0f, frame.y + 78.0f, 200.0f, 18.0f}, 10.0f,
                col::Muted, false, Gdiplus::StringAlignmentFar);
    canvas.Text(L"自动判分：" + IntText(examCorrect_) + L" / " +
                    IntText(static_cast<int>(examQuestions_.size())) + L" 道正确",
                ui::Rect{frame.x + 36.0f, frame.y + 108.0f, 420.0f, 20.0f}, 11.0f,
                col::Green, true);
    canvas.Text(L"解答题需自评；选择题和填空题已自动判分。",
                ui::Rect{frame.x + 36.0f, frame.y + 132.0f, 520.0f, 20.0f}, 9.8f,
                col::Faint, false);
    float rowY = frame.y + 168.0f;
    for (size_t i = 0; i < examQuestions_.size() && rowY < frame.y + frame.h - 116.0f; ++i) {
        const Question& question = examQuestions_[i];
        bool correct = false;
        if (question.type == QuestionType::Choice) {
            int index = examChoices_[i];
            std::string answer = AnswerKey(question.answer);
            correct = index >= 0 && !answer.empty() &&
                      answer[0] == static_cast<char>('A' + index);
        } else if (question.type == QuestionType::Fill) {
            correct = AnswerKey(question.answer) == AnswerKey(examAnswers_[i]);
        }
        Gdiplus::Color status = question.type == QuestionType::Solution
                                    ? col::Muted
                                    : (correct ? col::Green : col::Red);
        canvas.Circle(frame.x + 42.0f, rowY + 12.0f, 3.5f, status);
        canvas.Text(IntText(static_cast<int>(i + 1)) + L". " + TypeName(question.type) + L"　" +
                        (question.type == QuestionType::Solution ? L"自评" : (correct ? L"正确" : L"错误")),
                    ui::Rect{frame.x + 54.0f, rowY, 260.0f, 22.0f}, 10.5f, status, true);
        canvas.TextEllipsis(W(question.stem),
                            ui::Rect{frame.x + 54.0f, rowY + 22.0f, frame.w - 110.0f, 20.0f},
                            10.2f, col::InkSoft, false);
        canvas.Text(L"答案：" + W(question.answer),
                    ui::Rect{frame.x + 54.0f, rowY + 42.0f, frame.w - 110.0f, 18.0f}, 9.2f,
                    col::Muted, false);
        rowY += 64.0f;
    }
    ui::Rect resetButton{frame.x + 36.0f, frame.y + frame.h - 64.0f, 132.0f, 40.0f};
    ui::Rect backButton{resetButton.right() + 10.0f, frame.y + frame.h - 64.0f, 150.0f,
                        40.0f};
    DrawButton(canvas, resetButton, L"重新组卷", L"exam_reset", L"", 2, true, 11.0f);
    DrawButton(canvas, backButton, L"返回题库", L"exam_back", L"", 0, true, 11.0f);
}

void App::ParseFunction() {
    functionSource_ = GetControlText(functionEdit_);
    std::string error;
    mathexpr::Expression expression;
    if (!expression.Parse(util::ToUtf8(functionSource_), &error)) {
        functionError_ = W(error);
        functionRoots_.clear();
        return;
    }
    functionExpression_ = expression;
    functionError_.clear();
    functionRoots_ = mathexpr::FindRoots(functionExpression_, -10.0, 10.0, 1200);
}

void App::StartFunctionExplanation() {
    if (functionAiRunning_ || geometryAiRunning_) return;
    ParseFunction();
    if (!functionError_.empty()) {
        ShowToast(L"请先修正函数表达式。", true);
        return;
    }
    Settings settings = store_.Config();
    if (util::Trim(settings.model).empty()) {
        ShowToast(L"请先在 AI 接口中配置模型。", true);
        return;
    }
    std::ostringstream rootText;
    for (size_t i = 0; i < functionRoots_.size() && i < 20; ++i) {
        if (i) rootText << ", ";
        rootText << functionRoots_[i].x;
    }
    std::string prompt =
        "函数：f(x)=" + util::ToUtf8(functionSource_) + "\n" +
        "在区间 [-10,10] 内扫描到的零点：" + (rootText.str().empty() ? "无" : rootText.str()) +
        "\n请用中文简洁说明：定义域、值域、单调区间、奇偶性、极值与渐近线（如存在）。"
        "结合图像解释，不要输出 Markdown 表格，控制在 450 字以内。";
    functionAiRunning_ = true;
    functionAiText_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    toolThread_ = std::thread([target, settings, prompt]() {
        AiChatResult result = AskAiChat(
            settings, "你是一名高中数学教师，擅长把函数图像和代数性质联系起来。",
            prompt, 1600, 120000);
        auto* update = new ToolAiUpdate{0, result.ok, result.text, result.error};
        if (!PostMessageW(target, kMsgToolFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::GenerateGeometrySketch() {
    if (geometryAiRunning_ || functionAiRunning_) return;
    std::wstring description = GetControlText(geometryEdit_);
    if (selectedIds_.size() == 1) {
        if (const Question* selected = store_.FindQuestion(*selectedIds_.begin())) {
            description += L"\n\n当前题目：" + W(selected->stem);
        }
    }
    if (util::Trim(description).empty()) {
        ShowToast(L"请先输入几何题描述。", true);
        return;
    }
    Settings settings = store_.Config();
    if (util::Trim(settings.model).empty()) {
        ShowToast(L"请先在 AI 接口中配置模型。", true);
        return;
    }
    std::string prompt = "几何题描述：\n" + util::ToUtf8(description) +
                         "\n\n请生成一个便于板书画图的平面或立体几何示意图。只输出 JSON：\n"
                         "{\"kind\":\"plane\",\"title\":\"三角形 ABC\","
                         "\"points\":[{\"id\":\"A\",\"label\":\"A\",\"x\":0.15,\"y\":0.8}],"
                         "\"segments\":[{\"a\":\"A\",\"b\":\"B\"}],"
                         "\"circles\":[{\"x\":0.5,\"y\":0.5,\"r\":0.2}],"
                         "\"note\":\"关键辅助线说明\"}\n"
                         "所有 x/y 坐标使用 0 到 1 的归一化坐标，y 向上；点 id 必须与线段引用一致。"
                         "平面题 kind 用 plane，立体题用 solid。不要输出 Markdown。";
    geometryAiRunning_ = true;
    geometryError_.clear();
    geometryAiText_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    toolThread_ = std::thread([target, settings, prompt]() {
        AiChatResult result = AskAiChat(
            settings, "你是一名数学教师，负责把几何题转成简洁的示意图数据。", prompt,
            1800, 120000);
        auto* update = new ToolAiUpdate{1, result.ok, result.text, result.error};
        if (!PostMessageW(target, kMsgToolFinished, 0, reinterpret_cast<LPARAM>(update))) {
            delete update;
        }
    });
}

void App::HandleToolFinished(WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    std::unique_ptr<ToolAiUpdate> update(reinterpret_cast<ToolAiUpdate*>(lParam));
    if (toolThread_.joinable()) toolThread_.join();
    if (!update) return;
    if (update->kind == 0) {
        functionAiRunning_ = false;
        if (update->ok) {
            functionAiText_ = W(NormalizeMathText(update->text));
        } else {
            functionAiText_ = L"AI 分析失败：" + W(update->error);
        }
    } else {
        geometryAiRunning_ = false;
        geometryError_.clear();
        if (!update->ok) {
            geometryError_ = W(update->error);
        } else {
            std::string text = update->text;
            size_t first = text.find('{');
            size_t last = text.rfind('}');
            GeometrySpec spec;
            if (first != std::string::npos && last != std::string::npos && last > first) {
                jsonx::Value root = jsonx::Value::Parse(text.substr(first, last - first + 1));
                if (root.IsObject()) {
                    spec.kind = root["kind"].AsString("plane");
                    spec.title = root["title"].AsString();
                    spec.note = root["note"].AsString();
                    const jsonx::Value& points = root["points"];
                    if (points.IsArray()) {
                        for (const auto& item : points.Items()) {
                            if (!item.IsObject()) continue;
                            GeometryPoint point;
                            point.id = item["id"].AsString();
                            point.label = item["label"].AsString(point.id);
                            point.x = item["x"].AsNumber(0.5);
                            point.y = item["y"].AsNumber(0.5);
                            if (!point.id.empty()) spec.points.push_back(std::move(point));
                        }
                    }
                    const jsonx::Value& segments = root["segments"];
                    if (segments.IsArray()) {
                        for (const auto& item : segments.Items()) {
                            if (!item.IsObject()) continue;
                            GeometrySegment segment;
                            segment.a = item["a"].AsString();
                            segment.b = item["b"].AsString();
                            if (!segment.a.empty() && !segment.b.empty()) {
                                spec.segments.push_back(std::move(segment));
                            }
                        }
                    }
                    const jsonx::Value& circles = root["circles"];
                    if (circles.IsArray()) {
                        for (const auto& item : circles.Items()) {
                            if (!item.IsObject()) continue;
                            GeometryCircle circle;
                            circle.x = item["x"].AsNumber(0.5);
                            circle.y = item["y"].AsNumber(0.5);
                            circle.r = item["r"].AsNumber(0.2);
                            spec.circles.push_back(circle);
                        }
                    }
                }
            }
            if (spec.points.empty()) {
                geometryError_ = L"AI 没有返回可绘制的几何点。";
                geometryAiText_ = W(update->text);
            } else {
                spec.ok = true;
                geometrySpec_ = std::move(spec);
                geometryAiText_ = W(geometrySpec_.note);
            }
        }
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::DrawFunctionPlot(ui::Canvas& canvas, const ui::Rect& rect) {
    const ui::Rect plot{rect.x + 20.0f, rect.y + 54.0f, rect.w - 40.0f,
                        std::max(120.0f, rect.h - 84.0f)};
    canvas.FillRect(plot, col::CardAlt, 10.0f);
    canvas.StrokeRect(plot, col::Border, 10.0f, 1.0f);
    if (!functionError_.empty()) {
        canvas.TextWrapped(L"表达式错误：" + functionError_,
                           plot.Inset(18.0f, 18.0f), 11.0f, col::Red, false,
                           Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentNear);
        return;
    }

    const double xMin = -10.0, xMax = 10.0;
    double yMin = -1.0, yMax = 1.0;
    bool haveY = false;
    double sampleMin = 1e100, sampleMax = -1e100;
    std::vector<double> values;
    values.reserve(601);
    for (int i = 0; i <= 600; ++i) {
        double x = xMin + (xMax - xMin) * static_cast<double>(i) / 600.0;
        double y = functionExpression_.ok() ? functionExpression_.Evaluate(x)
                                            : std::numeric_limits<double>::quiet_NaN();
        values.push_back(y);
        if (std::isfinite(y) && std::abs(y) < 1e6) {
            sampleMin = std::min(sampleMin, y);
            sampleMax = std::max(sampleMax, y);
            haveY = true;
        }
    }
    if (!haveY) {
        sampleMin = -1.0;
        sampleMax = 1.0;
    }
    double span = sampleMax - sampleMin;
    if (span < 1e-6) span = 2.0;
    yMin = sampleMin - span * 0.12;
    yMax = sampleMax + span * 0.12;
    if (yMax - yMin < 0.5) {
        double mid = (yMax + yMin) * 0.5;
        yMin = mid - 0.5;
        yMax = mid + 0.5;
    }
    auto mapX = [&](double x) {
        return plot.x + static_cast<float>((x - xMin) / (xMax - xMin)) * plot.w;
    };
    auto mapY = [&](double y) {
        return plot.bottom() - static_cast<float>((y - yMin) / (yMax - yMin)) * plot.h;
    };

    canvas.PushClip(plot);
    const Gdiplus::Color grid = col::GridLineSoft;
    for (int i = -10; i <= 10; ++i) {
        float x = mapX(i);
        canvas.Line(x, plot.y, x, plot.bottom(), grid, 0.7f);
    }
    int yGrid = 5;
    for (int i = 0; i <= yGrid; ++i) {
        double y = yMin + (yMax - yMin) * static_cast<double>(i) / yGrid;
        float py = mapY(y);
        canvas.Line(plot.x, py, plot.right(), py, grid, 0.7f);
    }
    if (yMin <= 0.0 && yMax >= 0.0) {
        float y0 = mapY(0.0);
        canvas.Line(plot.x, y0, plot.right(), y0, col::Muted, 1.3f);
    }
    if (xMin <= 0.0 && xMax >= 0.0) {
        float x0 = mapX(0.0);
        canvas.Line(x0, plot.y, x0, plot.bottom(), col::Muted, 1.3f);
    }

    bool havePrev = false;
    double prevX = 0.0, prevY = 0.0;
    for (size_t i = 0; i < values.size(); ++i) {
        double x = xMin + (xMax - xMin) * static_cast<double>(i) / 600.0;
        double y = values[i];
        if (!std::isfinite(y) || std::abs(y) > 1e6) {
            havePrev = false;
            continue;
        }
        if (havePrev) {
            float x1 = mapX(prevX), y1 = mapY(prevY);
            float x2 = mapX(x), y2 = mapY(y);
            if (std::abs(y2 - y1) < plot.h * 4.0f) {
                canvas.Line(x1, y1, x2, y2, col::Accent, 2.0f);
            }
        }
        prevX = x;
        prevY = y;
        havePrev = true;
    }
    for (const auto& root : functionRoots_) {
        if (root.x < xMin || root.x > xMax || !std::isfinite(root.y)) continue;
        float px = mapX(root.x);
        float py = mapY(0.0);
        canvas.Circle(px, py, 4.0f, col::Amber);
        canvas.CircleOutline(px, py, 5.0f, col::CardAlt, 1.0f);
        std::wstring label = W(util::Trim([&]() {
            if (std::abs(root.x) < 1e-8) return std::string("0");
            std::ostringstream stream;
            stream.precision(4);
            stream << root.x;
            return stream.str();
        }()));
        canvas.Text(label, ui::Rect{px - 20.0f, py - 22.0f, 40.0f, 16.0f}, 8.5f, col::Amber,
                    true, Gdiplus::StringAlignmentCenter);
    }
    canvas.PopClip();
    canvas.Text(L"x", ui::Rect{plot.right() - 18.0f, plot.y + 4.0f, 16.0f, 16.0f}, 10.0f,
                col::Muted, true);
    canvas.Text(L"y", ui::Rect{plot.x + 4.0f, plot.y + 4.0f, 16.0f, 16.0f}, 10.0f,
                col::Muted, true);
}

void App::DrawGeometrySketch(ui::Canvas& canvas, const ui::Rect& rect) {
    const ui::Rect board{rect.x + 20.0f, rect.y + 54.0f, rect.w - 40.0f,
                         std::max(140.0f, rect.h - 88.0f)};
    canvas.FillRect(board, col::CardAlt, 10.0f);
    canvas.StrokeRect(board, col::Border, 10.0f, 1.0f);
    if (geometryError_ != L"") {
        canvas.TextWrapped(L"几何图生成失败：" + geometryError_, board.Inset(18.0f, 18.0f),
                           11.0f, col::Red, false, Gdiplus::StringAlignmentNear,
                           Gdiplus::StringAlignmentNear);
        return;
    }
    if (!geometrySpec_.ok) {
        canvas.TextWrapped(L"输入几何题描述，点击“AI 生成示意图”。\n"
                           L"AI 会返回归一化点、线段和圆，程序在本地绘制，不依赖外部图片服务。",
                           board.Inset(24.0f, 24.0f), 12.0f, col::Faint, false,
                           Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
        return;
    }
    auto mapPoint = [&](double x, double y) {
        return Gdiplus::PointF(board.x + 24.0f + static_cast<float>(x) * (board.w - 48.0f),
                               board.bottom() - 24.0f - static_cast<float>(y) * (board.h - 48.0f));
    };
    for (const auto& circle : geometrySpec_.circles) {
        auto center = mapPoint(circle.x, circle.y);
        float radius = static_cast<float>(circle.r) * std::min(board.w, board.h);
        canvas.CircleOutline(center.X, center.Y, radius, col::Blue, 1.3f);
    }
    for (const auto& segment : geometrySpec_.segments) {
        const GeometryPoint* from = nullptr;
        const GeometryPoint* to = nullptr;
        for (const auto& point : geometrySpec_.points) {
            if (point.id == segment.a) from = &point;
            if (point.id == segment.b) to = &point;
        }
        if (!from || !to) continue;
        auto p1 = mapPoint(from->x, from->y);
        auto p2 = mapPoint(to->x, to->y);
        canvas.Line(p1.X, p1.Y, p2.X, p2.Y, col::InkSoft, 1.8f);
    }
    for (const auto& point : geometrySpec_.points) {
        auto p = mapPoint(point.x, point.y);
        canvas.Circle(p.X, p.Y, 3.5f, col::Accent);
        canvas.Text(W(point.label.empty() ? point.id : point.label),
                    ui::Rect{p.X + 6.0f, p.Y - 18.0f, 80.0f, 18.0f}, 10.5f, col::Ink, true);
    }
    canvas.Text(W(geometrySpec_.title), ui::Rect{rect.x + 20.0f, rect.y + 18.0f,
                                                  rect.w - 40.0f, 24.0f},
                13.0f, col::Ink, true);
}

void App::PaintFunctionTools(ui::Canvas& canvas, float width, float height) {
    const ToolsRects rects = ComputeToolsRects(width, height);
    canvas.DrawShadow(rects.inputCard, 14.0f);
    canvas.FillRect(rects.inputCard, col::Card, 14.0f);
    canvas.StrokeRect(rects.inputCard, col::Border, 14.0f, 1.0f);
    canvas.Text(L"函数图像", ui::Rect{rects.inputCard.x + 20.0f, rects.inputCard.y + 8.0f,
                                       160.0f, 20.0f},
                11.0f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.functionField, functionEdit_);
    AddHit(ui::Rect{rects.functionField.x, rects.functionField.y, rects.functionField.w,
                    rects.functionField.h},
           L"focus_function");
    DrawButton(canvas, rects.keyboardButton, L"⌨ 键盘", L"tools_keyboard", L"", 2, true, 11.0f);
    DrawButton(canvas, rects.drawButton, L"绘制", L"tools_draw", L"", 0, true, 11.0f);
    DrawButton(canvas, rects.aiButton, functionAiRunning_ ? L"AI 分析中…" : L"AI 分析性质",
               L"tools_ai", L"", functionAiRunning_ ? 2 : 5,
               !functionAiRunning_ && !geometryAiRunning_, 11.0f);

    canvas.DrawShadow(rects.plotCard, 14.0f);
    canvas.FillRect(rects.plotCard, col::Card, 14.0f);
    canvas.StrokeRect(rects.plotCard, col::Border, 14.0f, 1.0f);
    canvas.Text(L"曲线 / 零点标记", ui::Rect{rects.plotCard.x + 20.0f, rects.plotCard.y + 18.0f,
                                              260.0f, 22.0f},
                13.0f, col::Ink, true);
    DrawFunctionPlot(canvas, rects.plotCard);

    canvas.DrawShadow(rects.aiCard, 14.0f);
    canvas.FillRect(rects.aiCard, col::Card, 14.0f);
    canvas.StrokeRect(rects.aiCard, col::Border, 14.0f, 1.0f);
    canvas.Text(L"AI 性质说明", ui::Rect{rects.aiCard.x + 20.0f, rects.aiCard.y + 18.0f,
                                          220.0f, 22.0f},
                13.0f, col::Ink, true);
    std::wstring aiText = functionAiText_;
    if (functionAiRunning_) aiText = L"AI 正在结合图像分析定义域、值域和单调性…";
    if (aiText.empty()) {
        aiText = L"绘制函数后点击“AI 分析性质”，这里会显示定义域、值域、单调区间、"
                 L"奇偶性、极值和渐近线。";
    }
    canvas.TextWrapped(aiText,
                       ui::Rect{rects.aiCard.x + 20.0f, rects.aiCard.y + 52.0f,
                                rects.aiCard.w - 40.0f, rects.aiCard.h - 72.0f},
                       10.6f, col::InkSoft, false, Gdiplus::StringAlignmentNear,
                       Gdiplus::StringAlignmentNear);
}

void App::PaintGeometryTools(ui::Canvas& canvas, float width, float height) {
    const ToolsRects rects = ComputeToolsRects(width, height);
    canvas.DrawShadow(rects.geometryInputCard, 14.0f);
    canvas.FillRect(rects.geometryInputCard, col::Card, 14.0f);
    canvas.StrokeRect(rects.geometryInputCard, col::Border, 14.0f, 1.0f);
    canvas.Text(L"几何题描述 / 已知条件", ui::Rect{rects.geometryInputCard.x + 20.0f,
                                                    rects.geometryInputCard.y + 8.0f, 280.0f, 20.0f},
                11.0f, col::InkSoft, true);
    DrawFieldBackground(canvas, rects.geometryField, geometryEdit_);
    AddHit(rects.geometryField, L"focus_geometry");
    DrawButton(canvas, rects.geometryKeyboardButton, L"⌨ 键盘", L"geometry_keyboard", L"", 2,
               true, 11.0f);
    DrawButton(canvas, rects.geometryGenerateButton,
               geometryAiRunning_ ? L"AI 生成中…" : L"AI 生成示意图", L"geometry_generate",
               L"", geometryAiRunning_ ? 2 : 0,
               !geometryAiRunning_ && !functionAiRunning_, 11.0f);

    canvas.DrawShadow(rects.geometryCanvas, 14.0f);
    canvas.FillRect(rects.geometryCanvas, col::Card, 14.0f);
    canvas.StrokeRect(rects.geometryCanvas, col::Border, 14.0f, 1.0f);
    DrawGeometrySketch(canvas, rects.geometryCanvas);

    canvas.DrawShadow(rects.geometryAiCard, 14.0f);
    canvas.FillRect(rects.geometryAiCard, col::Card, 14.0f);
    canvas.StrokeRect(rects.geometryAiCard, col::Border, 14.0f, 1.0f);
    canvas.Text(L"AI 图形说明", ui::Rect{rects.geometryAiCard.x + 20.0f,
                                           rects.geometryAiCard.y + 18.0f, 220.0f, 22.0f},
                13.0f, col::Ink, true);
    canvas.TextWrapped(geometryAiText_.empty() ? L"生成后会在这里显示辅助线、关键点和图形说明。"
                                                : geometryAiText_,
                       ui::Rect{rects.geometryAiCard.x + 20.0f, rects.geometryAiCard.y + 54.0f,
                                rects.geometryAiCard.w - 40.0f,
                                rects.geometryAiCard.h - 74.0f},
                       10.6f, col::InkSoft, false, Gdiplus::StringAlignmentNear,
                       Gdiplus::StringAlignmentNear);
}

void App::PaintTools(ui::Canvas& canvas, float width, float height) {
    const ToolsRects rects = ComputeToolsRects(width, height);
    auto drawTab = [&](const ui::Rect& tab, const std::wstring& text, int index) {
        bool active = toolsTab_ == index;
        Gdiplus::Color fill = active ? col::Card : col::CardAlt;
        Gdiplus::Color border = active ? col::Accent : col::Border;
        canvas.FillRect(tab, fill, 10.0f);
        canvas.StrokeRect(tab, border, 10.0f, active ? 1.3f : 1.0f);
        canvas.Text(text, tab, 12.0f, active ? col::Accent : col::Muted, active,
                    Gdiplus::StringAlignmentCenter);
        AddHit(tab, L"tools_tab", IntText(index));
    };
    drawTab(rects.functionTab, L"函数图像", 0);
    drawTab(rects.geometryTab, L"几何示意图", 1);
    if (toolsTab_ == 0) PaintFunctionTools(canvas, width, height);
    else PaintGeometryTools(canvas, width, height);
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
        else if (payload == L"exam") SwitchPage(Page::Exam);
        else if (payload == L"tools") SwitchPage(Page::Tools);
        else if (payload == L"paper") SwitchPage(Page::Paper);
        return;
    }
    if (action == L"focus") {
        if (payload == L"search") {
            SetFocus(searchEdit_);
        } else if (payload == L"api_base") {
            SetFocus(apiBaseEdit_);
        } else if (payload == L"api_model") {
            modelMenuOpen_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
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
    if (action == L"tools_tab") {
        toolsTab_ = util::ClampInt(std::stoi(payload), 0, 1);
        CloseMathKeyboard();
        LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"tools_draw") {
        ParseFunction();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"tools_ai") {
        StartFunctionExplanation();
        return;
    }
    if (action == L"tools_keyboard") {
        OpenMathKeyboard(functionEdit_);
        return;
    }
    if (action == L"geometry_keyboard") {
        OpenMathKeyboard(geometryEdit_);
        return;
    }
    if (action == L"geometry_generate") {
        GenerateGeometrySketch();
        return;
    }
    if (action == L"focus_function") {
        SetFocus(functionEdit_);
        return;
    }
    if (action == L"focus_geometry") {
        SetFocus(geometryEdit_);
        return;
    }
    if (action == L"exam_send") {
        SendExamMessage();
        return;
    }
    if (action == L"exam_generate") {
        GenerateExamPaper();
        return;
    }
    if (action == L"exam_start") {
        StartExam();
        return;
    }
    if (action == L"exam_choice") {
        if (examActive_ && !examQuestions_.empty()) {
            int choice = std::stoi(payload);
            examChoices_[static_cast<size_t>(examIndex_)] = choice;
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return;
    }
    if (action == L"exam_prev" || action == L"exam_next") {
        if (examActive_ && !examQuestions_.empty()) {
            SaveCurrentExamAnswer();
            int count = static_cast<int>(examQuestions_.size());
            examIndex_ = std::clamp(examIndex_ + (action == L"exam_next" ? 1 : -1), 0,
                                    count - 1);
            LoadExamAnswerToEdit();
            LayoutEditControls();
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return;
    }
    if (action == L"exam_submit") {
        if (examActive_ &&
            MessageBoxW(hwnd_, L"确定要交卷吗？交卷后将自动判分并停止计时。",
                        L"AI 考试", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            SubmitExam();
        }
        return;
    }
    if (action == L"exam_keyboard") {
        OpenMathKeyboard(examAnswerEdit_);
        return;
    }
    if (action == L"focus_exam_chat") {
        SetFocus(examChatEdit_);
        return;
    }
    if (action == L"focus_exam_answer") {
        SetFocus(examAnswerEdit_);
        return;
    }
    if (action == L"exam_reset") {
        ResetExam();
        return;
    }
    if (action == L"exam_back") {
        ResetExam();
        SwitchPage(Page::Bank);
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
    if (action == L"toggle_favorite_question") {
        Question* question = store_.FindQuestion(util::ToUtf8(payload));
        if (!question) return;
        question->favorite = !question->favorite;
        std::string error;
        if (!store_.SaveQuestions(&error)) {
            ShowToast(W(error), true);
        } else {
            ShowToast(question->favorite ? L"已收藏该题。" : L"已取消收藏。");
        }
        if (filter_.onlyFavorites) RebuildFiltered();
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
    if (action == L"chip_grade") {
        filter_.grade = payload == L"all" ? -1 : std::stoi(payload.substr(1));
        bankScroll_.offset = 0.0f;
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"tidy_questions") {
        int changed = 0;
        for (auto& question : store_.Questions()) {
            std::string stem = InsertImplicitMultiplication(NormalizeMathText(question.stem));
            std::string answer = InsertImplicitMultiplication(NormalizeMathText(question.answer));
            std::string analysis = InsertImplicitMultiplication(NormalizeMathText(question.analysis));
            if (stem != question.stem || answer != question.answer || analysis != question.analysis) {
                ++changed;
            }
            question.stem = std::move(stem);
            question.answer = std::move(answer);
            question.analysis = std::move(analysis);
            for (auto& option : question.options) {
                option = InsertImplicitMultiplication(NormalizeMathText(option));
            }
        }
        std::string error;
        if (!store_.SaveQuestions(&error)) {
            ShowToast(W(error), true);
        } else {
            ShowToast(L"AI 整理完成，规范化了 " + IntText(changed) + L" 道题。");
        }
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"dedup_questions") {
        std::vector<DuplicateGroup> groups = FindDuplicateGroups(store_.Questions());
        int removable = 0;
        for (const auto& group : groups) removable += static_cast<int>(group.questionIndices.size()) - 1;
        if (removable == 0) {
            ShowToast(L"没有发现完全重复的题目。");
            return;
        }
        std::wstring message = L"AI 整理后发现 " + IntText(static_cast<int>(groups.size())) +
                               L" 组重复题目，共可删除 " + IntText(removable) +
                               L" 道。\n\n每组保留信息最完整的一道（收藏题优先）。\n\n";
        int shown = 0;
        for (const auto& group : groups) {
            if (shown >= 3) break;
            const Question& first = store_.Questions()[group.questionIndices.front()];
            std::wstring stem = W(first.stem);
            if (stem.size() > 72) stem = stem.substr(0, 72) + L"…";
            message += L"• " + stem + L"（" +
                       IntText(static_cast<int>(group.questionIndices.size())) + L" 道）\n";
            ++shown;
        }
        if (groups.size() > 3) message += L"…\n";
        message += L"\n是否删除重复项？";
        if (MessageBoxW(hwnd_, message.c_str(), L"AI 去重", MB_YESNO | MB_ICONQUESTION) != IDYES) {
            return;
        }
        std::set<std::string> removeIds;
        for (const auto& group : groups) {
            size_t keep = group.questionIndices.front();
            int bestScore = -1;
            for (size_t index : group.questionIndices) {
                const Question& question = store_.Questions()[index];
                int score = static_cast<int>(question.answer.size() + question.analysis.size());
                if (question.favorite) score += 100000;
                if (score > bestScore) {
                    bestScore = score;
                    keep = index;
                }
            }
            for (size_t index : group.questionIndices) {
                if (index != keep) removeIds.insert(store_.Questions()[index].id);
            }
        }
        auto& questions = store_.Questions();
        questions.erase(std::remove_if(questions.begin(), questions.end(),
                                       [&](const Question& question) {
                                           return removeIds.count(question.id) > 0;
                                       }),
                        questions.end());
        for (const auto& id : removeIds) selectedIds_.erase(id);
        std::string error;
        if (!store_.SaveQuestions(&error)) {
            ShowToast(W(error), true);
        } else {
            ShowToast(L"已删除 " + IntText(static_cast<int>(removeIds.size())) + L" 道重复题。");
        }
        RebuildFiltered();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"toggle_favorites") {
        filter_.onlyFavorites = !filter_.onlyFavorites;
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
    if (action == L"model_menu") {
        modelMenuOpen_ = !modelMenuOpen_;
        LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"close_model_menu") {
        modelMenuOpen_ = false;
        LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (action == L"select_model") {
        SetControlText(apiModelEdit_, payload);
        std::wstring base = BaseForModel(payload);
        if (!base.empty()) SetControlText(apiBaseEdit_, base);
        modelMenuOpen_ = false;
        testHasResult_ = false;
        testStatus_.clear();
        LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
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
    modelMenuOpen_ = false;
    if (provider == "deepseek") {
        SetControlText(apiBaseEdit_, L"https://api.deepseek.com");
        SetControlText(apiModelEdit_, L"deepseek-chat");
    } else if (provider == "openai") {
        SetControlText(apiBaseEdit_, L"https://api.openai.com/v1");
        SetControlText(apiModelEdit_, L"gpt-4o-mini");
    } else {
        SetFocus(apiBaseEdit_);
        SendMessageW(apiBaseEdit_, EM_SETSEL, 0, -1);
        LayoutEditControls();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    testHasResult_ = false;
    testStatus_.clear();
    LayoutEditControls();
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
            ShowToast(L"请选择模型。", true);
            modelMenuOpen_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
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
        ShowToast(L"请先在 AI 接口中选择模型。", true);
        modelMenuOpen_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
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
            update->grade = result.grade;
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
            if (update->grade != Grade::Unknown) question->grade = update->grade;
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
        ShowToast(L"请先在 AI 接口中填写 API 地址并选择模型。", true);
        modelMenuOpen_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    wchar_t fileBuffer[32768] = {};
    std::wstring initialDir = util::DesktopDirectory().wstring();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter =
        L"题目图片或文档 (*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.docx;*.doc;*.txt;*.md)\0"
        L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.docx;*.doc;*.txt;*.md\0"
        L"图片文件 (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0"
        L"Word/文本 (*.docx;*.doc;*.txt;*.md)\0*.docx;*.doc;*.txt;*.md\0"
        L"所有文件 (*.*)\0*.*\0\0";
    dialog.lpstrFile = fileBuffer;
    dialog.nMaxFile = static_cast<DWORD>(std::size(fileBuffer));
    dialog.lpstrInitialDir = initialDir.c_str();
    dialog.lpstrTitle = L"输入文件以识题（可多选图片 / DOCX / TXT）";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR |
                   OFN_ALLOWMULTISELECT;
    if (!GetOpenFileNameW(&dialog)) {
        return;
    }

    std::vector<std::filesystem::path> files;
    std::wstring first(fileBuffer);
    if (first.empty()) return;
    const wchar_t* extra = fileBuffer + first.size() + 1;
    if (*extra == L'\0') {
        files.emplace_back(first);
    } else {
        std::filesystem::path directory(first);
        for (const wchar_t* name = extra; *name; name += wcslen(name) + 1) {
            files.push_back(directory / name);
        }
    }
    if (files.empty()) {
        ShowToast(L"没有选择可识别的文件。", true);
        return;
    }

    // A DOCX whose formulas are MathType/WMF images cannot be read from its
    // text layer.  If the user has exported/rendered the pages next to the
    // document as "<name>_页面图片", transparently switch to visual import.
    std::vector<std::filesystem::path> expanded;
    int autoPageCount = 0;
    for (const auto& file : files) {
        const std::string extension = util::ToLowerAscii(util::ToUtf8(file.extension().wstring()));
        if (extension == ".docx" || extension == ".doc") {
            const std::filesystem::path pageDir =
                file.parent_path() / (file.stem().wstring() + L"_页面图片");
            if (std::filesystem::is_directory(pageDir)) {
                std::vector<std::filesystem::path> pages;
                for (const auto& entry : std::filesystem::directory_iterator(pageDir)) {
                    if (!entry.is_regular_file()) continue;
                    const std::string pageExtension =
                        util::ToLowerAscii(util::ToUtf8(entry.path().extension().wstring()));
                    if (pageExtension == ".png" || pageExtension == ".jpg" ||
                        pageExtension == ".jpeg" || pageExtension == ".bmp" ||
                        pageExtension == ".gif") {
                        pages.push_back(entry.path());
                    }
                }
                if (!pages.empty()) {
                    std::sort(pages.begin(), pages.end());
                    expanded.insert(expanded.end(), pages.begin(), pages.end());
                    autoPageCount += static_cast<int>(pages.size());
                    continue;
                }
            }
        }
        expanded.push_back(file);
    }
    files = std::move(expanded);
    if (files.size() > 40) {
        MessageBoxW(hwnd_, L"一次最多识别 40 个文件/页面，请分批识别。", L"输入文件识题",
                    MB_ICONINFORMATION);
        return;
    }
    if (autoPageCount > 0) {
        ShowToast(L"检测到页面图片，已自动载入 " + IntText(autoPageCount) + L" 页。");
        if (!LooksLikeVisionModel(W(settings.model))) {
            const std::wstring warning =
                L"这份文档已转换为 " + IntText(autoPageCount) +
                L" 页图片，但当前模型“" + W(settings.model) +
                L"”看起来不是视觉模型。\n\n图片识题需要 qwen-vl-max、gpt-4o、glm-4v-plus 等"
                L"支持图片输入的模型。\n\n仍要用当前模型尝试吗？";
            if (MessageBoxW(hwnd_, warning.c_str(), L"需要视觉模型",
                            MB_YESNO | MB_ICONWARNING) != IDYES) {
                SwitchPage(Page::Ai);
                modelMenuOpen_ = true;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
        }
    }

    if (imageThread_.joinable()) imageThread_.join();
    imageExtracting_ = true;
    imageBatchCount_ = static_cast<int>(files.size());
    SetTimer(hwnd_, 9, 260, nullptr);
    InvalidateRect(hwnd_, nullptr, FALSE);
    HWND target = hwnd_;
    TagLibrary library = store_.Tags();
    imageThread_ = std::thread([target, files = std::move(files), settings,
                                library = std::move(library)]() mutable {
        auto* update = new ImageUpdate;
        std::string errors;
        for (size_t index = 0; index < files.size(); ++index) {
            const auto& path = files[index];
            std::string prefix = "第 " + std::to_string(index + 1) + " 个文件";
            std::string filename = util::ToUtf8(path.filename().wstring());
            if (!filename.empty()) prefix += "（" + filename + "）";
            std::string ext = util::ToLowerAscii(util::ToUtf8(path.extension().wstring()));
            AiImageQuestionResult result;
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                ext == ".gif") {
                EncodedImageData encoded;
                std::string imageError;
                if (!EncodeImageForAi(path, encoded, &imageError)) {
                    errors += prefix + "：" + imageError + "\n";
                    continue;
                }
                result = AnalyzeQuestionImageWithAi(encoded.dataUrl, settings, library);
            } else {
                TextFileResult document = ExtractTextFile(path);
                if (!document.ok) {
                    errors += prefix + "：" + document.error + "\n";
                    continue;
                }
                if (document.mathTypeObjects > 0) {
                    errors += prefix + "：检测到 " + std::to_string(document.mathTypeObjects) +
                              " 个 MathType 公式对象（WMF 图片），另有 " +
                              std::to_string(document.drawingObjects) +
                              " 个绘图对象。文字层中只有题干碎片，公式无法被文本模型读取，"
                              "继续导入会把题目读成残缺内容。\n"
                              "请用 Word/WPS 将文档逐页导出/截图成 PNG，再在“输入文件识题”中"
                              "选择这些图片（图片识题需要支持视觉输入的模型）。\n";
                    continue;
                }
                result = AnalyzeQuestionDocumentWithAi(document.text, settings, library);
            }
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
        std::wstring message = L"文件识题失败：\n\n" + W(update->error);
        message += L"\n\n提示：图片需要支持视觉输入的模型；DOCX/TXT 使用文本模型。"
                   L"旧版 .doc/PDF 请先另存为 DOCX；如果已有同名“_页面图片”文件夹，"
                   L"选择 .doc 时会自动改用页面图片。";
        MessageBoxW(hwnd_, message.c_str(), L"输入文件识题", MB_ICONWARNING);
        return;
    }

    const int questionCount = static_cast<int>(update->questions.size());
    int missingAnswer = 0;
    for (const auto& question : update->questions) {
        if (util::Trim(question.answer).empty() || util::Trim(question.analysis).empty()) {
            ++missingAnswer;
        }
    }
    std::wstring confirm = L"AI 从 " + IntText(batchCount) + L" 个文件中识别出 " +
                           IntText(questionCount) + L" 道题，并已为每道题生成答案与解析。\n\n" +
                           L"是否全部加入题库？\n加入后可在题库顶部逐题检查、编辑或组卷。";
    if (missingAnswer > 0) {
        confirm += L"\n\n注意：其中 " + IntText(missingAnswer) +
                   L" 道题的答案或解析为空，导入后请补全。";
    }
    if (!util::Trim(update->error).empty()) {
        confirm += L"\n\n有部分文件识别失败，已成功识别的题目仍可导入。";
    }
    int answer = MessageBoxW(hwnd_, confirm.c_str(), L"文件识题结果",
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
        question.stem = InsertImplicitMultiplication(NormalizeMathText(question.stem));
        for (auto& option : question.options) {
            option = InsertImplicitMultiplication(NormalizeMathText(option));
        }
        question.answer = InsertImplicitMultiplication(NormalizeMathText(question.answer));
        question.analysis = InsertImplicitMultiplication(NormalizeMathText(question.analysis));
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
        MessageBoxW(hwnd_, (L"部分文件识别失败：\n\n" + detail).c_str(), L"输入文件识题",
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
