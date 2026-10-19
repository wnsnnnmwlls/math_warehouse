#include "editor.h"

#include "keyboard.h"
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
#include <uxtheme.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr wchar_t kEditorClass[] = L"MathPaperStudioQuestionEditor";
constexpr int kDesignWidth = 920;
constexpr int kDesignHeight = 700;
constexpr int kIdStem = 3001;
constexpr int kIdOptionA = 3002;
constexpr int kIdOptionB = 3003;
constexpr int kIdOptionC = 3004;
constexpr int kIdOptionD = 3005;
constexpr int kIdAnswer = 3006;
constexpr int kIdAnalysis = 3007;
constexpr int kIdFormulas = 3008;
constexpr int kIdConcepts = 3009;

namespace edcol {
Gdiplus::Color Background;
Gdiplus::Color Card;
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
Gdiplus::Color Soft;
Gdiplus::Color Red;
Gdiplus::Color RedSoft;
Gdiplus::Color Hover;
Gdiplus::Color Green;
Gdiplus::Color GreenSoft;
Gdiplus::Color Amber;
Gdiplus::Color AmberSoft;
Gdiplus::Color OnAccent;
Gdiplus::Color DisabledFill;
Gdiplus::Color AccentBorder;

void Apply(bool dark) {
    OnAccent = ui::Color(255, 255, 255);
    if (!dark) {
        Background = ui::Color(245, 246, 250);
        Card = ui::Color(255, 255, 255);
        Field = ui::Color(255, 255, 255);
        Ink = ui::Color(22, 29, 45);
        InkSoft = ui::Color(51, 60, 80);
        Muted = ui::Color(104, 112, 129);
        Faint = ui::Color(146, 153, 168);
        Border = ui::Color(224, 228, 237);
        BorderStrong = ui::Color(203, 209, 221);
        Accent = ui::Color(79, 70, 229);
        AccentDark = ui::Color(67, 56, 202);
        AccentSoft = ui::Color(238, 237, 255);
        Soft = ui::Color(247, 248, 252);
        Red = ui::Color(193, 63, 74);
        RedSoft = ui::Color(254, 239, 241);
        Hover = ui::Color(246, 246, 251);
        Green = ui::Color(21, 134, 88);
        GreenSoft = ui::Color(234, 248, 240);
        Amber = ui::Color(178, 106, 9);
        AmberSoft = ui::Color(255, 244, 227);
        DisabledFill = ui::Color(239, 241, 246);
        AccentBorder = ui::Color(203, 199, 255);
    } else {
        Background = ui::Color(12, 16, 23);
        Card = ui::Color(21, 27, 38);
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
        Soft = ui::Color(17, 23, 34);
        Red = ui::Color(255, 123, 134);
        RedSoft = ui::Color(58, 34, 40);
        Hover = ui::Color(32, 40, 54);
        Green = ui::Color(91, 214, 163);
        GreenSoft = ui::Color(22, 53, 43);
        Amber = ui::Color(233, 185, 110);
        AmberSoft = ui::Color(58, 45, 27);
        DisabledFill = ui::Color(26, 33, 48);
        AccentBorder = ui::Color(76, 70, 140);
    }
}
}  // namespace edcol

std::wstring GetText(HWND edit) {
    if (!edit) return {};
    int length = GetWindowTextLengthW(edit);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    int copied = GetWindowTextW(edit, text.data(), length + 1);
    text.resize(copied > 0 ? static_cast<size_t>(copied) : 0);
    return text;
}

std::string GetTextUtf8(HWND edit) {
    std::string text = util::ToUtf8(GetText(edit));
    std::string normalized;
    normalized.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') continue;
            normalized.push_back('\n');
        } else {
            normalized.push_back(text[i]);
        }
    }
    return normalized;
}

void SetText(HWND edit, const std::wstring& text) {
    if (!edit) return;
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
    SetWindowTextW(edit, normalized.c_str());
}

struct Layout {
    ui::Rect stemField;
    ui::Rect optionFields[4];
    ui::Rect answerField;
    ui::Rect analysisField;
    ui::Rect formulaField;
    ui::Rect conceptField;
    ui::Rect difficultyChips[3];
    ui::Rect typeChips[3];
    ui::Rect saveButton;
    ui::Rect cancelButton;
    ui::Rect deleteButton;
    ui::Rect mathKeyboardButton;
    ui::Rect tipCard;
};

class EditorWindow {
public:
    EditorWindow(HWND owner, ui::Theme& theme, const TagLibrary& library, const Question* existing,
                 const Question* draft);
    ~EditorWindow();

    bool Create();
    void RunModal();
    const QuestionEditorResult& Result() const { return result_; }

private:
    HWND owner_ = nullptr;
    HWND hwnd_ = nullptr;
    ui::Theme& theme_;
    const TagLibrary& library_;
    float scale_ = 1.0f;
    HFONT font_ = nullptr;
    HBRUSH whiteBrush_ = nullptr;
    HBRUSH backgroundBrush_ = nullptr;
    bool closed_ = false;
    bool loading_ = true;
    bool dirty_ = false;
    bool hasExisting_ = false;
    bool hasDraft_ = false;
    Difficulty difficulty_ = Difficulty::B;
    QuestionType type_ = QuestionType::Choice;
    Grade grade_ = Grade::Unknown;
    bool favorite_ = false;
    Question original_;
    QuestionEditorResult result_;
    std::wstring errorMessage_;

    HWND hStem_ = nullptr;
    HWND hOption_[4] = {};
    HWND hAnswer_ = nullptr;
    HWND hAnalysis_ = nullptr;
    HWND hFormulas_ = nullptr;
    HWND hConcepts_ = nullptr;
    HWND lastFocusedEdit_ = nullptr;

    std::vector<HWND> tabOrder_;
    std::vector<HWND> allEdits_;

    struct Hit {
        ui::Rect rect;
        std::wstring action;
        std::wstring payload;
    };
    std::vector<Hit> hits_;
    std::wstring hoverKey_;
    bool tracking_ = false;

    Layout GetLayout() const;
    void ApplyQuestionToFields();
    void UpdateChildLayout();
    void RebuildTabOrder();
    void FocusNext(bool backwards);
    void AddHit(const ui::Rect& rect, const std::wstring& action,
                const std::wstring& payload = {});
    std::wstring MakeKey(const std::wstring& action, const std::wstring& payload) const;
    bool IsHovered(const std::wstring& action, const std::wstring& payload = {}) const;
    void OnPaint();
    void OnMouseMove(float x, float y);
    void OnMouseUp(float x, float y);
    void DrawButton(ui::Canvas& canvas, const ui::Rect& rect, const std::wstring& text,
                    const std::wstring& action, int style, bool enabled = true);
    void DrawField(ui::Canvas& canvas, const ui::Rect& rect, HWND edit, bool option = false,
                   int optionIndex = 0);
    void DrawChips(ui::Canvas& canvas, const ui::Rect (&chips)[3], const wchar_t* const* labels,
                   int selected, const std::wstring& action, const std::wstring& payloadPrefix);
    void TrySave();
    void Cancel();
    void Delete();
    void RequestWindowClose();
    void DestroyEditor();

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

EditorWindow::EditorWindow(HWND owner, ui::Theme& theme, const TagLibrary& library,
                           const Question* existing, const Question* draft)
    : owner_(owner), theme_(theme), library_(library) {
    scale_ = theme.scale();
    if (existing) {
        hasExisting_ = true;
        original_ = *existing;
        difficulty_ = original_.difficulty;
        type_ = original_.type;
        grade_ = original_.grade;
        favorite_ = original_.favorite;
        result_.question = original_;
    } else if (draft) {
        hasDraft_ = true;
        original_ = *draft;
        difficulty_ = original_.difficulty;
        type_ = original_.type;
        grade_ = original_.grade;
        favorite_ = original_.favorite;
        result_.question = original_;
    } else {
        difficulty_ = Difficulty::B;
        type_ = QuestionType::Choice;
    }
}

EditorWindow::~EditorWindow() {
    if (font_) DeleteObject(font_);
    if (whiteBrush_) DeleteObject(whiteBrush_);
    if (backgroundBrush_) DeleteObject(backgroundBrush_);
}

LRESULT CALLBACK EditorWindow::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam,
                                              LPARAM lParam) {
    EditorWindow* editor =
        reinterpret_cast<EditorWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!editor && message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        editor = reinterpret_cast<EditorWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(editor));
    }
    if (editor) return editor->HandleMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool EditorWindow::Create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &EditorWindow::StaticWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kEditorClass;
    wc.hIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101),
                                            IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    RegisterClassExW(&wc);

    edcol::Apply(theme_.dark());
    font_ = CreateFontW(-static_cast<int>(std::lround(14.0f * scale_)), 0, 0, 0, FW_NORMAL, FALSE,
                        FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    whiteBrush_ =
        CreateSolidBrush(theme_.dark() ? RGB(15, 21, 32) : RGB(255, 255, 255));
    backgroundBrush_ =
        CreateSolidBrush(theme_.dark() ? RGB(12, 16, 23) : RGB(245, 246, 250));

    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
    DWORD exStyle = WS_EX_DLGMODALFRAME;
    RECT windowRect{0, 0, static_cast<LONG>(std::lround(kDesignWidth * scale_)),
                    static_cast<LONG>(std::lround(kDesignHeight * scale_))};
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);
    RECT ownerRect{};
    if (owner_) GetWindowRect(owner_, &ownerRect);
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;
    int minX = static_cast<int>(workArea.left) + 20;
    int minY = static_cast<int>(workArea.top) + 20;
    int maxX = std::max(minX, static_cast<int>(workArea.right) - width - 20);
    int maxY = std::max(minY, static_cast<int>(workArea.bottom) - height - 20);
    x = std::clamp(x, minX, maxX);
    y = std::clamp(y, minY, maxY);

    std::wstring title = hasExisting_ ? L"编辑题目 · 数研组卷" : L"新增题目 · 数研组卷";
    hwnd_ = CreateWindowExW(exStyle, kEditorClass, title.c_str(), style, x, y, width, height,
                            owner_, nullptr, GetModuleHandleW(nullptr), this);
    if (!hwnd_) return false;
    ui::ApplyWindowTheme(hwnd_, theme_.dark());

    auto createEdit = [&](int id, DWORD editStyle) -> HWND {
        HWND edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_TABSTOP | editStyle, 0, 0, 10,
                                    10, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                    GetModuleHandleW(nullptr), nullptr);
        if (!edit) return nullptr;
        SetWindowTheme(edit, theme_.dark() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4, 4));
        SendMessageW(edit, EM_SETLIMITTEXT, 40000, 0);
        allEdits_.push_back(edit);
        return edit;
    };

    hStem_ = createEdit(kIdStem, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN |
                                      WS_VSCROLL);
    hOption_[0] = createEdit(kIdOptionA, ES_LEFT | ES_AUTOHSCROLL);
    hOption_[1] = createEdit(kIdOptionB, ES_LEFT | ES_AUTOHSCROLL);
    hOption_[2] = createEdit(kIdOptionC, ES_LEFT | ES_AUTOHSCROLL);
    hOption_[3] = createEdit(kIdOptionD, ES_LEFT | ES_AUTOHSCROLL);
    hAnswer_ = createEdit(kIdAnswer, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN);
    hAnalysis_ = createEdit(kIdAnalysis, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                             ES_WANTRETURN | WS_VSCROLL);
    hFormulas_ = createEdit(kIdFormulas, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN);
    hConcepts_ = createEdit(kIdConcepts, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN);

    const wchar_t* cues[] = {L"输入选项 A", L"输入选项 B", L"输入选项 C（可选）",
                             L"输入选项 D（可选）"};
    for (int i = 0; i < 4; ++i) {
        SendMessageW(hOption_[i], EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cues[i]));
    }
    SendMessageW(hStem_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"输入题干，可换行"));
    SendMessageW(hAnswer_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"输入答案"));
    SendMessageW(hAnalysis_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"输入解析（可选）"));
    SendMessageW(hFormulas_, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(L"如：余弦定理，正弦定理（用逗号分隔）"));
    SendMessageW(hConcepts_, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(L"如：解三角形，函数的单调性（用逗号分隔）"));

    ApplyQuestionToFields();
    loading_ = false;
    dirty_ = false;
    RebuildTabOrder();
    UpdateChildLayout();
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetFocus(hStem_);
    return true;
}

void EditorWindow::ApplyQuestionToFields() {
    if (hasExisting_ || hasDraft_) {
        SetText(hStem_, util::ToWide(original_.stem));
        for (int i = 0; i < 4; ++i) {
            SetText(hOption_[i], i < static_cast<int>(original_.options.size())
                                     ? util::ToWide(original_.options[static_cast<size_t>(i)])
                                     : L"");
        }
        SetText(hAnswer_, util::ToWide(original_.answer));
        SetText(hAnalysis_, util::ToWide(original_.analysis));
        SetText(hFormulas_, util::ToWide(util::Join(original_.formulas, "，")));
        SetText(hConcepts_, util::ToWide(util::Join(original_.concepts, "，")));
    }
}

Layout EditorWindow::GetLayout() const {
    Layout layout;
    const float leftX = 28.0f;
    const float leftW = 542.0f;
    const float rightX = 588.0f;
    const float rightW = 304.0f;

    layout.stemField = ui::Rect{leftX, 112.0f, leftW, 132.0f};
    if (type_ == QuestionType::Choice) {
        const float optionW = (leftW - 12.0f) * 0.5f;
        layout.optionFields[0] = ui::Rect{leftX, 278.0f, optionW, 36.0f};
        layout.optionFields[1] = ui::Rect{leftX + optionW + 12.0f, 278.0f, optionW, 36.0f};
        layout.optionFields[2] = ui::Rect{leftX, 318.0f, optionW, 36.0f};
        layout.optionFields[3] = ui::Rect{leftX + optionW + 12.0f, 318.0f, optionW, 36.0f};
        layout.answerField = ui::Rect{leftX, 390.0f, leftW, 78.0f};
        layout.analysisField = ui::Rect{leftX, 502.0f, leftW, 124.0f};
    } else {
        for (int i = 0; i < 4; ++i) layout.optionFields[i] = ui::Rect{};
        layout.answerField = ui::Rect{leftX, 282.0f, leftW, 108.0f};
        layout.analysisField = ui::Rect{leftX, 426.0f, leftW, 200.0f};
    }

    const float chipW = (rightW - 16.0f) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        layout.difficultyChips[i] =
            ui::Rect{rightX + static_cast<float>(i) * (chipW + 8.0f), 114.0f, chipW, 32.0f};
        layout.typeChips[i] =
            ui::Rect{rightX + static_cast<float>(i) * (chipW + 8.0f), 180.0f, chipW, 32.0f};
    }
    layout.formulaField = ui::Rect{rightX, 266.0f, rightW, 88.0f};
    layout.conceptField = ui::Rect{rightX, 390.0f, rightW, 88.0f};
    layout.tipCard = ui::Rect{rightX, 503.0f, rightW, 123.0f};

    layout.saveButton = ui::Rect{kDesignWidth - 28.0f - 112.0f, 650.0f, 112.0f, 38.0f};
    layout.cancelButton = ui::Rect{layout.saveButton.x - 10.0f - 84.0f, 650.0f, 84.0f, 38.0f};
    layout.deleteButton = ui::Rect{28.0f, 653.0f, 96.0f, 32.0f};
    layout.mathKeyboardButton = ui::Rect{140.0f, 650.0f, 108.0f, 34.0f};
    return layout;
}

void EditorWindow::UpdateChildLayout() {
    if (!hwnd_) return;
    Layout layout = GetLayout();
    auto setBounds = [&](HWND edit, const ui::Rect& field, float leftInset, float topInset,
                         bool multiline) {
        if (!edit) return;
        ui::Rect rect;
        if (multiline) {
            rect = ui::Rect{field.x + leftInset, field.y + topInset,
                            std::max(10.0f, field.w - leftInset * 2.0f),
                            std::max(18.0f, field.h - topInset * 2.0f)};
        } else {
            float height = std::min(26.0f, field.h - 8.0f);
            rect = ui::Rect{field.x + leftInset, field.y + (field.h - height) * 0.5f,
                            std::max(10.0f, field.w - leftInset * 2.0f), height};
        }
        SetWindowPos(edit, nullptr, static_cast<int>(std::lround(rect.x * scale_)),
                     static_cast<int>(std::lround(rect.y * scale_)),
                     static_cast<int>(std::lround(rect.w * scale_)),
                     static_cast<int>(std::lround(rect.h * scale_)),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    };

    setBounds(hStem_, layout.stemField, 10.0f, 8.0f, true);
    for (int i = 0; i < 4; ++i) {
        setBounds(hOption_[i], layout.optionFields[i], 30.0f, 4.0f, false);
        ShowWindow(hOption_[i], type_ == QuestionType::Choice ? SW_SHOWNA : SW_HIDE);
    }
    setBounds(hAnswer_, layout.answerField, 8.0f, 8.0f, true);
    setBounds(hAnalysis_, layout.analysisField, 8.0f, 8.0f, true);
    setBounds(hFormulas_, layout.formulaField, 8.0f, 8.0f, true);
    setBounds(hConcepts_, layout.conceptField, 8.0f, 8.0f, true);
    for (HWND edit : {hStem_, hAnswer_, hAnalysis_, hFormulas_, hConcepts_}) {
        ShowWindow(edit, SW_SHOWNA);
    }
    RebuildTabOrder();
}

void EditorWindow::RebuildTabOrder() {
    tabOrder_.clear();
    tabOrder_.push_back(hStem_);
    if (type_ == QuestionType::Choice) {
        for (HWND edit : hOption_) tabOrder_.push_back(edit);
    }
    tabOrder_.push_back(hAnswer_);
    tabOrder_.push_back(hAnalysis_);
    tabOrder_.push_back(hFormulas_);
    tabOrder_.push_back(hConcepts_);
}

void EditorWindow::FocusNext(bool backwards) {
    if (tabOrder_.empty()) return;
    HWND focused = GetFocus();
    auto it = std::find(tabOrder_.begin(), tabOrder_.end(), focused);
    int index = it == tabOrder_.end() ? 0 : static_cast<int>(it - tabOrder_.begin());
    int count = static_cast<int>(tabOrder_.size());
    index = backwards ? (index - 1 + count) % count : (index + 1) % count;
    SetFocus(tabOrder_[static_cast<size_t>(index)]);
}

void EditorWindow::AddHit(const ui::Rect& rect, const std::wstring& action,
                          const std::wstring& payload) {
    hits_.push_back(Hit{rect, action, payload});
}

std::wstring EditorWindow::MakeKey(const std::wstring& action,
                                   const std::wstring& payload) const {
    return action + L"\x1F" + payload;
}

bool EditorWindow::IsHovered(const std::wstring& action, const std::wstring& payload) const {
    return hoverKey_ == MakeKey(action, payload);
}

void EditorWindow::DrawButton(ui::Canvas& canvas, const ui::Rect& rect, const std::wstring& text,
                              const std::wstring& action, int style, bool enabled) {
    std::wstring key = MakeKey(action, L"");
    bool hovered = enabled && hoverKey_ == key;
    Gdiplus::Color fill;
    Gdiplus::Color border(0, 0, 0, 0);
    Gdiplus::Color textColor = edcol::InkSoft;
    if (!enabled) {
        fill = edcol::DisabledFill;
        textColor = edcol::Faint;
    } else if (style == 0) {
        fill = hovered ? edcol::AccentDark : edcol::Accent;
        textColor = edcol::OnAccent;
    } else if (style == 1) {
        fill = hovered ? edcol::Hover : edcol::Card;
        border = edcol::BorderStrong;
    } else if (style == 2) {
        fill = hovered ? edcol::RedSoft : Gdiplus::Color(0, 0, 0, 0);
        textColor = edcol::Red;
    } else {
        fill = edcol::Card;
        border = edcol::Border;
    }
    canvas.FillRect(rect, fill, 9.0f);
    if (border.GetA() > 0) canvas.StrokeRect(rect, border, 9.0f, 1.0f);
    canvas.Text(text, rect, 12.0f, textColor, style == 0 || style == 2,
                Gdiplus::StringAlignmentCenter);
    if (enabled) AddHit(rect, action);
}

void EditorWindow::DrawField(ui::Canvas& canvas, const ui::Rect& rect, HWND edit, bool option,
                             int optionIndex) {
    bool focused = edit && GetFocus() == edit;
    canvas.FillRect(rect, edcol::Field, 9.0f);
    canvas.StrokeRect(rect, focused ? edcol::Accent : edcol::BorderStrong, 9.0f,
                      focused ? 1.5f : 1.0f);
    if (option) {
        static const wchar_t* letters[] = {L"A", L"B", L"C", L"D"};
        canvas.Text(letters[optionIndex],
                    ui::Rect{rect.x + 10.0f, rect.y, 18.0f, rect.h}, 11.0f, edcol::Muted, true,
                    Gdiplus::StringAlignmentCenter);
    }
}

void EditorWindow::DrawChips(ui::Canvas& canvas, const ui::Rect (&chips)[3],
                             const wchar_t* const* labels, int selected,
                             const std::wstring& action, const std::wstring& payloadPrefix) {
    for (int i = 0; i < 3; ++i) {
        std::wstring payload = payloadPrefix + std::to_wstring(i);
        bool active = i == selected;
        bool hovered = IsHovered(action, payload);
        Gdiplus::Color fill = active ? edcol::AccentSoft : edcol::Card;
        Gdiplus::Color border = active ? edcol::AccentBorder : edcol::Border;
        if (hovered && !active) {
            fill = edcol::Hover;
            border = edcol::BorderStrong;
        }
        canvas.FillRect(chips[i], fill, 8.0f);
        canvas.StrokeRect(chips[i], border, 8.0f, active ? 1.2f : 1.0f);
        canvas.Text(labels[i], chips[i], 11.5f, active ? edcol::Accent : edcol::InkSoft, active,
                    Gdiplus::StringAlignmentCenter);
        AddHit(chips[i], action, payload);
    }
}

void EditorWindow::OnPaint() {
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
    edcol::Apply(theme_.dark());
    Gdiplus::Bitmap buffer(width, height, PixelFormat32bppRGB);
    Gdiplus::Graphics graphics(&buffer);
    graphics.Clear(edcol::Background);
    ui::Canvas canvas(graphics, theme_);
    hits_.clear();
    Layout layout = GetLayout();

    canvas.FillRect(ui::Rect{0, 0, kDesignWidth, 82.0f}, edcol::Card);
    canvas.Line(0, 81.0f, kDesignWidth, 81.0f, edcol::Border, 1.0f);
    canvas.FillRect(ui::Rect{28.0f, 21.0f, 34.0f, 34.0f}, edcol::Accent, 10.0f);
    canvas.Text(L"题", ui::Rect{28.0f, 21.0f, 34.0f, 34.0f}, 17.0f, edcol::OnAccent, true,
                Gdiplus::StringAlignmentCenter);
    canvas.Text(hasExisting_ ? L"编辑题目" : L"新增题目",
                ui::Rect{74.0f, 17.0f, 300.0f, 28.0f}, 20.0f, edcol::Ink, true);
    canvas.Text(hasDraft_ ? L"AI 已从图片中提取内容，请核对并修正后保存"
                          : L"完成内容后保存，题目会立即进入题库并参与筛选",
                ui::Rect{74.0f, 45.0f, 600.0f, 20.0f}, 10.5f, edcol::Muted, false);
    if (dirty_) {
        ui::Rect dirtyPill{kDesignWidth - 28.0f - 90.0f, 24.0f, 90.0f, 24.0f};
        canvas.FillRect(dirtyPill, edcol::GreenSoft, 7.0f);
        canvas.Circle(dirtyPill.x + 12.0f, dirtyPill.y + 12.0f, 2.7f, edcol::Green);
        canvas.Text(L"未保存", ui::Rect{dirtyPill.x + 20.0f, dirtyPill.y, 62.0f, 24.0f}, 9.5f,
                    edcol::Green, true);
    }

    canvas.Text(L"年级", ui::Rect{544.0f, 26.0f, 42.0f, 18.0f}, 9.5f, edcol::Muted, true);
    const wchar_t* gradeLabels[] = {L"高一", L"高二", L"高三"};
    for (int i = 0; i < 3; ++i) {
        ui::Rect chip{590.0f + static_cast<float>(i) * 52.0f, 22.0f, 48.0f, 30.0f};
        bool active = static_cast<int>(grade_) == i + 1;
        bool hovered = IsHovered(L"grade", std::to_wstring(i + 1));
        Gdiplus::Color fill = active ? edcol::AccentSoft : edcol::Card;
        Gdiplus::Color border = active ? edcol::AccentBorder : edcol::Border;
        if (hovered && !active) {
            fill = edcol::Hover;
            border = edcol::BorderStrong;
        }
        canvas.FillRect(chip, fill, 8.0f);
        canvas.StrokeRect(chip, border, 8.0f, active ? 1.2f : 1.0f);
        canvas.Text(gradeLabels[i], chip, 10.0f, active ? edcol::Accent : edcol::InkSoft, active,
                    Gdiplus::StringAlignmentCenter);
        AddHit(chip, L"grade", std::to_wstring(i + 1));
    }
    ui::Rect favoriteButton{748.0f, 22.0f, 36.0f, 30.0f};
    bool favoriteHovered = IsHovered(L"favorite", L"");
    if (favoriteHovered) canvas.FillRect(favoriteButton, edcol::AmberSoft, 8.0f);
    canvas.Text(favorite_ ? L"★" : L"☆", favoriteButton, 17.0f,
                favorite_ ? edcol::Amber : edcol::Faint, false,
                Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
    AddHit(favoriteButton, L"favorite");

    canvas.Text(L"题干 *", ui::Rect{28.0f, 93.0f, 160.0f, 16.0f}, 11.0f, edcol::InkSoft, true);
    DrawField(canvas, layout.stemField, hStem_);

    if (type_ == QuestionType::Choice) {
        canvas.Text(L"选项", ui::Rect{28.0f, 254.0f, 100.0f, 16.0f}, 11.0f, edcol::InkSoft,
                    true);
        canvas.Text(L"至少填写 A、B 两项", ui::Rect{90.0f, 254.0f, 180.0f, 16.0f}, 9.2f,
                    edcol::Faint, false);
        for (int i = 0; i < 4; ++i) {
            DrawField(canvas, layout.optionFields[i], hOption_[i], true, i);
        }
        canvas.Text(L"参考答案", ui::Rect{28.0f, 370.0f, 160.0f, 16.0f}, 11.0f, edcol::InkSoft,
                    true);
        canvas.Text(L"选择题建议填写字母，例如 B", ui::Rect{104.0f, 370.0f, 260.0f, 16.0f},
                    9.2f, edcol::Faint, false);
        DrawField(canvas, layout.answerField, hAnswer_);
        canvas.Text(L"解析", ui::Rect{28.0f, 481.0f, 100.0f, 16.0f}, 11.0f, edcol::InkSoft, true);
        DrawField(canvas, layout.analysisField, hAnalysis_);
    } else {
        canvas.Text(L"参考答案", ui::Rect{28.0f, 256.0f, 160.0f, 16.0f}, 11.0f, edcol::InkSoft,
                    true);
        canvas.Text(L"填空题可填写数值或表达式；解答题可填写简要答案",
                    ui::Rect{104.0f, 256.0f, 420.0f, 16.0f}, 9.2f, edcol::Faint, false);
        DrawField(canvas, layout.answerField, hAnswer_);
        canvas.Text(L"解析", ui::Rect{28.0f, 403.0f, 100.0f, 16.0f}, 11.0f, edcol::InkSoft, true);
        DrawField(canvas, layout.analysisField, hAnalysis_);
    }

    canvas.Text(L"难度", ui::Rect{588.0f, 93.0f, 100.0f, 16.0f}, 11.0f, edcol::InkSoft, true);
    const wchar_t* difficultyLabels[] = {L"A 基础", L"B 提升", L"C 挑战"};
    DrawChips(canvas, layout.difficultyChips, difficultyLabels, static_cast<int>(difficulty_),
              L"difficulty", L"");
    canvas.Text(L"题型", ui::Rect{588.0f, 159.0f, 100.0f, 16.0f}, 11.0f, edcol::InkSoft, true);
    const wchar_t* typeLabels[] = {L"选择题", L"填空题", L"解答题"};
    DrawChips(canvas, layout.typeChips, typeLabels, static_cast<int>(type_), L"type", L"");
    canvas.Line(588.0f, 230.0f, 892.0f, 230.0f, edcol::Border, 1.0f);

    canvas.Text(L"公式标签", ui::Rect{588.0f, 244.0f, 160.0f, 16.0f}, 11.0f, edcol::InkSoft,
                true);
    canvas.Text(L"用逗号或顿号分隔", ui::Rect{720.0f, 244.0f, 170.0f, 16.0f}, 9.0f, edcol::Faint,
                false, Gdiplus::StringAlignmentFar);
    DrawField(canvas, layout.formulaField, hFormulas_);

    canvas.Text(L"知识点标签", ui::Rect{588.0f, 368.0f, 160.0f, 16.0f}, 11.0f, edcol::InkSoft,
                true);
    canvas.Text(L"会同步到独立标签容器", ui::Rect{720.0f, 368.0f, 170.0f, 16.0f}, 9.0f,
                edcol::Faint, false, Gdiplus::StringAlignmentFar);
    DrawField(canvas, layout.conceptField, hConcepts_);

    canvas.FillRect(layout.tipCard, edcol::Soft, 12.0f);
    canvas.StrokeRect(layout.tipCard, edcol::Border, 12.0f, 1.0f);
    canvas.Text(L"标签小贴士", ui::Rect{layout.tipCard.x + 16.0f, layout.tipCard.y + 13.0f,
                                        160.0f, 18.0f},
                10.8f, edcol::InkSoft, true);
    canvas.TextWrapped(
        L"• 多个标签用逗号、顿号或分号分隔。\n"
        L"• 保存后可在题库中勾选题目，使用 AI 智能标注自动补全。\n"
        L"• 标签库当前记录：" +
            std::to_wstring(library_.formulas.size()) + L" 个公式、" +
            std::to_wstring(library_.concepts.size()) + L" 个知识点。",
        ui::Rect{layout.tipCard.x + 16.0f, layout.tipCard.y + 37.0f,
                 layout.tipCard.w - 32.0f, layout.tipCard.h - 45.0f},
        9.3f, edcol::Muted, false, Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentNear);

    canvas.FillRect(ui::Rect{0, 632.0f, kDesignWidth, 68.0f}, edcol::Card);
    canvas.Line(0, 632.0f, kDesignWidth, 632.0f, edcol::Border, 1.0f);
    if (!errorMessage_.empty()) {
        canvas.Text(errorMessage_, ui::Rect{28.0f, 650.0f, 560.0f, 38.0f}, 10.5f, edcol::Red,
                    true);
    } else {
        canvas.Text(hasExisting_ ? L"修改会覆盖原题；可以删除不再需要的题目。"
                                 : L"Ctrl + S 保存 · Esc 取消 · 标签可留空，稍后让 AI 补全",
                    ui::Rect{254.0f, 650.0f, 410.0f, 38.0f}, 9.8f, edcol::Faint, false);
    }
    if (hasExisting_) {
        DrawButton(canvas, layout.deleteButton, L"删除题目", L"delete", 2, true);
    }
    DrawButton(canvas, layout.mathKeyboardButton, L"⌨ 数学键盘", L"math_keyboard", 2, true);
    DrawButton(canvas, layout.cancelButton, L"取消", L"cancel", 1, true);
    DrawButton(canvas, layout.saveButton, L"保存题目", L"save", 0, true);

    AddHit(layout.stemField, L"focus_field", L"stem");
    for (int i = 0; i < 4; ++i) {
        if (type_ == QuestionType::Choice) {
            AddHit(layout.optionFields[i], L"focus_field", L"option_" + std::to_wstring(i));
        }
    }
    AddHit(layout.answerField, L"focus_field", L"answer");
    AddHit(layout.analysisField, L"focus_field", L"analysis");
    AddHit(layout.formulaField, L"focus_field", L"formulas");
    AddHit(layout.conceptField, L"focus_field", L"concepts");

    Gdiplus::Graphics target(dc);
    target.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    target.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    target.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    target.DrawImage(&buffer, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height,
                     Gdiplus::UnitPixel);
    EndPaint(hwnd_, &paint);
}

void EditorWindow::OnMouseMove(float x, float y) {
    if (!tracking_) {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        TrackMouseEvent(&track);
        tracking_ = true;
    }
    std::wstring key;
    bool overClickable = false;
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (it->rect.Contains(x, y)) {
            key = MakeKey(it->action, it->payload);
            overClickable = true;
            break;
        }
    }
    SetCursor(LoadCursorW(nullptr, overClickable ? IDC_HAND : IDC_ARROW));
    if (key != hoverKey_) {
        hoverKey_ = std::move(key);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void EditorWindow::OnMouseUp(float x, float y) {
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (!it->rect.Contains(x, y)) continue;
        std::wstring action = it->action;
        std::wstring payload = it->payload;
        if (action == L"save") {
            TrySave();
        } else if (action == L"cancel") {
            Cancel();
        } else if (action == L"delete") {
            Delete();
        } else if (action == L"math_keyboard") {
            HWND target = GetFocus();
            bool validTarget = false;
            for (HWND edit : allEdits_) {
                if (edit == target) {
                    validTarget = true;
                    break;
                }
            }
            if (!validTarget) target = lastFocusedEdit_;
            if (!target || !IsWindow(target)) target = hStem_;
            ShowMathKeyboard(hwnd_, target, theme_);
        } else if (action == L"difficulty") {
            int index = util::ClampInt(std::stoi(payload), 0, 2);
            difficulty_ = static_cast<Difficulty>(index);
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
        } else if (action == L"type") {
            int index = util::ClampInt(std::stoi(payload), 0, 2);
            QuestionType newType = static_cast<QuestionType>(index);
            if (newType != type_) {
                type_ = newType;
                dirty_ = true;
                UpdateChildLayout();
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
        } else if (action == L"grade") {
            grade_ = static_cast<Grade>(util::ClampInt(std::stoi(payload), 1, 3));
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
        } else if (action == L"favorite") {
            favorite_ = !favorite_;
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
        } else if (action == L"focus_field") {
            if (payload == L"stem") SetFocus(hStem_);
            else if (payload == L"answer") SetFocus(hAnswer_);
            else if (payload == L"analysis") SetFocus(hAnalysis_);
            else if (payload == L"formulas") SetFocus(hFormulas_);
            else if (payload == L"concepts") SetFocus(hConcepts_);
            else if (payload.rfind(L"option_", 0) == 0) {
                int index = util::ClampInt(std::stoi(payload.substr(7)), 0, 3);
                SetFocus(hOption_[index]);
            }
        }
        return;
    }
}

void EditorWindow::TrySave() {
    errorMessage_.clear();
    std::string stem = util::Trim(GetTextUtf8(hStem_));
    if (stem.empty()) {
        errorMessage_ = L"请填写题干。";
        SetFocus(hStem_);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    Question question = hasExisting_ ? original_ : Question{};
    if (question.id.empty()) question.id = util::NewId("q");
    if (question.created.empty()) question.created = util::NowIso8601();
    question.difficulty = difficulty_;
    question.type = type_;
    question.grade = grade_;
    question.favorite = favorite_;
    question.stem = stem;
    question.answer = util::Trim(GetTextUtf8(hAnswer_));
    question.analysis = util::Trim(GetTextUtf8(hAnalysis_));
    question.formulas = util::SplitTags(GetTextUtf8(hFormulas_));
    question.concepts = util::SplitTags(GetTextUtf8(hConcepts_));
    question.tagged = question.HasAnyTag();

    if (type_ == QuestionType::Choice) {
        std::vector<std::string> options;
        for (HWND edit : hOption_) {
            options.push_back(util::Trim(GetTextUtf8(edit)));
        }
        while (!options.empty() && options.back().empty()) options.pop_back();
        int nonEmpty = 0;
        for (const auto& option : options) {
            if (!option.empty()) ++nonEmpty;
        }
        if (options.size() < 2 || options[0].empty() || options[1].empty() || nonEmpty < 2) {
            errorMessage_ = L"选择题至少需要填写 A、B 两个选项。";
            SetFocus(hOption_[0]);
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
        question.options = std::move(options);
    } else {
        question.options.clear();
    }

    result_.action = QuestionEditorAction::Save;
    result_.question = std::move(question);
    DestroyEditor();
}

void EditorWindow::Cancel() {
    result_.action = QuestionEditorAction::Cancel;
    DestroyEditor();
}

void EditorWindow::Delete() {
    if (!hasExisting_) {
        Cancel();
        return;
    }
    int answer = MessageBoxW(hwnd_, L"确定删除这道题吗？删除后无法恢复。", L"删除题目",
                             MB_YESNO | MB_ICONWARNING);
    if (answer != IDYES) return;
    result_.action = QuestionEditorAction::Delete;
    result_.question = original_;
    DestroyEditor();
}

void EditorWindow::RequestWindowClose() {
    if (!closed_ && dirty_) {
        int answer = MessageBoxW(hwnd_, L"还有未保存的修改，确定放弃并关闭吗？", L"数研组卷",
                                 MB_YESNO | MB_ICONQUESTION);
        if (answer != IDYES) return;
    }
    result_.action = QuestionEditorAction::Cancel;
    DestroyEditor();
}

void EditorWindow::DestroyEditor() {
    CloseMathKeyboard();
    if (hwnd_) DestroyWindow(hwnd_);
}

LRESULT EditorWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    hwnd_ = hwnd;
    switch (message) {
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_COMMAND: {
            int notification = HIWORD(wParam);
            if (notification == EN_CHANGE && !loading_) {
                dirty_ = true;
                if (!errorMessage_.empty()) {
                    errorMessage_.clear();
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
            } else if (notification == EN_SETFOCUS) {
                lastFocusedEdit_ = reinterpret_cast<HWND>(lParam);
                InvalidateRect(hwnd_, nullptr, FALSE);
            } else if (notification == EN_KILLFOCUS) {
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            HDC editDc = reinterpret_cast<HDC>(wParam);
            if (theme_.dark()) {
                SetTextColor(editDc, RGB(232, 236, 245));
                SetBkColor(editDc, RGB(15, 21, 32));
            } else {
                SetTextColor(editDc, RGB(31, 38, 54));
                SetBkColor(editDc, RGB(255, 255, 255));
            }
            return reinterpret_cast<LRESULT>(whiteBrush_);
        }
        case WM_MOUSEMOVE:
            OnMouseMove(static_cast<float>(GET_X_LPARAM(lParam)) / scale_,
                        static_cast<float>(GET_Y_LPARAM(lParam)) / scale_);
            return 0;
        case WM_MOUSELEAVE:
            tracking_ = false;
            hoverKey_.clear();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_LBUTTONUP:
            OnMouseUp(static_cast<float>(GET_X_LPARAM(lParam)) / scale_,
                      static_cast<float>(GET_Y_LPARAM(lParam)) / scale_);
            return 0;
        case WM_CLOSE:
            RequestWindowClose();
            return 0;
        case WM_DESTROY:
            closed_ = true;
            hwnd_ = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void EditorWindow::RunModal() {
    if (!hwnd_) return;
    MSG message{};
    while (!closed_) {
        BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result <= 0) {
            if (result == 0) PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        if (message.message == WM_KEYDOWN) {
            bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (message.wParam == VK_ESCAPE) {
                Cancel();
                continue;
            }
            if (control && (message.wParam == 'S' || message.wParam == 's')) {
                TrySave();
                continue;
            }
            if (message.wParam == VK_TAB) {
                FocusNext(shift);
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

}  // namespace

QuestionEditorResult ShowQuestionEditor(HWND owner, ui::Theme& theme, const TagLibrary& library,
                                       const Question* existing, const Question* draft) {
    EditorWindow editor(owner, theme, library, existing, draft);
    if (!editor.Create()) {
        return {};
    }
    EnableWindow(owner, FALSE);
    editor.RunModal();
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    return editor.Result();
}
