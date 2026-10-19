#pragma once

#include "math_expr.h"
#include "model.h"
#include "store.h"
#include "ui.h"

#include <windows.h>

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

class App {
public:
    App();
    ~App();

    bool Create(HINSTANCE instance, int showCommand);
    int Run();

    HWND Hwnd() const { return hwnd_; }
    Store& Data() { return store_; }
    ui::Theme& Theme() { return theme_; }
    float Scale() const { return scale_; }

    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    enum class Page { Bank, Tags, Ai, Exam, Tools, Paper };

    enum ControlId {
        ID_SEARCH = 1001,
        ID_API_BASE = 1010,
        ID_API_MODEL = 1011,
        ID_API_KEY = 1012,
        ID_PAPER_TITLE = 1020,
        ID_PAPER_MINUTES = 1021,
        ID_PAPER_NOTES = 1022,
        ID_FUNCTION_INPUT = 1030,
        ID_GEOMETRY_INPUT = 1031,
        ID_EXAM_CHAT = 1040,
        ID_EXAM_ANSWER = 1041,
    };

    struct ExamMessage {
        bool user = false;
        std::wstring text;
    };

    struct GeometryPoint {
        std::string id;
        std::string label;
        double x = 0.5;
        double y = 0.5;
    };
    struct GeometrySegment {
        std::string a;
        std::string b;
    };
    struct GeometryCircle {
        double x = 0.5;
        double y = 0.5;
        double r = 0.2;
    };
    struct GeometrySpec {
        bool ok = false;
        std::string kind = "plane";
        std::string title;
        std::vector<GeometryPoint> points;
        std::vector<GeometrySegment> segments;
        std::vector<GeometryCircle> circles;
        std::string note;
    };

    struct HitTarget {
        ui::Rect rect;
        std::wstring action;
        std::wstring payload;
    };

    struct ScrollState {
        ui::Rect viewport;
        ui::Rect track;
        ui::Rect thumb;
        float offset = 0.0f;
        float maxOffset = 0.0f;
    };

    struct GroupCounts {
        int choice = 0;
        int fill = 0;
        int solution = 0;
        int points = 0;
    };

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    Store store_;
    ui::Theme theme_;
    float scale_ = 1.0f;
    int dpi_ = 96;
    HFONT editFont_ = nullptr;
    HBRUSH whiteBrush_ = nullptr;
    HBRUSH darkFieldBrush_ = nullptr;
    HBRUSH canvasBrush_ = nullptr;
    HICON smallIcon_ = nullptr;
    HICON largeIcon_ = nullptr;
    bool darkMode_ = false;

    Page page_ = Page::Bank;
    QuestionFilter filter_;
    std::vector<std::string> visibleIds_;
    std::set<std::string> selectedIds_;

    HWND searchEdit_ = nullptr;
    HWND apiBaseEdit_ = nullptr;
    HWND apiModelEdit_ = nullptr;
    HWND apiKeyEdit_ = nullptr;
    HWND paperTitleEdit_ = nullptr;
    HWND paperMinutesEdit_ = nullptr;
    HWND paperNotesEdit_ = nullptr;
    HWND functionEdit_ = nullptr;
    HWND geometryEdit_ = nullptr;
    HWND examChatEdit_ = nullptr;
    HWND examAnswerEdit_ = nullptr;

    ui::Rect searchFieldRect_{};
    ui::Rect searchEditRect_{};
    ui::Rect apiBaseFieldRect_{};
    ui::Rect apiModelFieldRect_{};
    ui::Rect apiKeyFieldRect_{};
    ui::Rect paperTitleFieldRect_{};
    ui::Rect paperMinutesFieldRect_{};
    ui::Rect paperNotesFieldRect_{};
    ui::Rect functionFieldRect_{};
    ui::Rect geometryFieldRect_{};
    ui::Rect examChatFieldRect_{};
    ui::Rect examAnswerFieldRect_{};

    std::vector<HitTarget> hits_;
    std::wstring hoverKey_;
    std::wstring pressedKey_;
    bool trackingMouse_ = false;

    ScrollState bankScroll_;
    ScrollState filterTagScroll_;
    ScrollState formulaScroll_;
    ScrollState conceptScroll_;
    ScrollState* draggingScroll_ = nullptr;
    float dragStartY_ = 0.0f;
    float dragStartOffset_ = 0.0f;

    std::wstring toastText_;
    bool toastError_ = false;
    bool toastActive_ = false;

    bool aiRunning_ = false;
    int aiTotal_ = 0;
    int aiDone_ = 0;
    int aiSuccess_ = 0;
    int aiFailed_ = 0;
    std::wstring aiLastError_;
    std::thread aiThread_;

    bool testRunning_ = false;
    bool testHasResult_ = false;
    bool testOk_ = false;
    std::wstring testStatus_;
    std::thread testThread_;

    bool imageExtracting_ = false;
    int imageBatchCount_ = 0;
    std::thread imageThread_;

    bool apiKeyVisible_ = false;
    bool modelMenuOpen_ = false;
    bool initialFieldsLoaded_ = false;
    std::unique_ptr<Gdiplus::Bitmap> backBuffer_;
    HDC backDc_ = nullptr;
    HBITMAP backBitmap_ = nullptr;
    HGDIOBJ backOldBitmap_ = nullptr;
    void* backBits_ = nullptr;
    int backBufferWidth_ = 0;
    int backBufferHeight_ = 0;

    std::vector<ExamMessage> examMessages_;
    std::wstring examStatus_;
    bool examChatBusy_ = false;
    bool examGenerating_ = false;
    std::thread examThread_;
    std::wstring examGeneratedTitle_;
    int examGeneratedMinutes_ = 30;
    std::vector<Question> examGenerated_;
    bool examActive_ = false;
    bool examSubmitted_ = false;
    std::wstring examTitle_;
    int examMinutes_ = 30;
    int examRemainingSeconds_ = 0;
    int examIndex_ = 0;
    int examReviewIndex_ = 0;
    int examScore_ = 0;
    int examTotalPoints_ = 0;
    int examCorrect_ = 0;
    std::vector<int> examChoices_;
    std::vector<std::string> examAnswers_;
    std::vector<Question> examQuestions_;

    int toolsTab_ = 0;  // 0 functions, 1 geometry
    std::wstring functionSource_ = L"sin(x)";
    mathexpr::Expression functionExpression_;
    std::vector<mathexpr::RootInfo> functionRoots_;
    std::wstring functionError_;
    std::wstring functionAiText_;
    bool functionAiRunning_ = false;
    std::wstring geometryPrompt_;
    GeometrySpec geometrySpec_;
    std::wstring geometryAiText_;
    std::wstring geometryError_;
    bool geometryAiRunning_ = false;
    std::thread toolThread_;

    bool paperFieldsReady_ = false;
    bool includeAnswers_ = true;
    bool includeAnalysis_ = true;
    bool includeAnswerSpace_ = true;
    std::filesystem::path lastExportPath_;

    std::wstring startupWarning_;

    void CreateFontsAndBrushes();
    void CreateEditControls();
    void ApplyTheme();
    void SetDarkMode(bool dark);
    void ApplyNativeTheme();
    void LayoutEditControls();
    void PositionSingleLineEdit(HWND edit, const ui::Rect& field, float horizontalInset = 12.0f);
    void PositionMultilineEdit(HWND edit, const ui::Rect& field, float inset = 10.0f);
    void ReadSettingsIntoFields();
    bool ReadSettingsFromFields(Settings& settings, bool showErrors);
    void ReadPaperDefaults();
    void EnsurePaperFields();

    void OnPaint();
    void PaintSidebar(ui::Canvas& canvas, float width, float height);
    void PaintHeader(ui::Canvas& canvas, float width, float height);
    void PaintBank(ui::Canvas& canvas, float width, float height);
    void PaintTags(ui::Canvas& canvas, float width, float height);
    void PaintAi(ui::Canvas& canvas, float width, float height);
    void PaintExam(ui::Canvas& canvas, float width, float height);
    void PaintExamSetup(ui::Canvas& canvas, float width, float height);
    void PaintExamRunner(ui::Canvas& canvas, float width, float height);
    void PaintExamResult(ui::Canvas& canvas, float width, float height);
    void PaintTools(ui::Canvas& canvas, float width, float height);
    void PaintFunctionTools(ui::Canvas& canvas, float width, float height);
    void PaintGeometryTools(ui::Canvas& canvas, float width, float height);
    void DrawFunctionPlot(ui::Canvas& canvas, const ui::Rect& rect);
    void DrawGeometrySketch(ui::Canvas& canvas, const ui::Rect& rect);
    void PaintPaper(ui::Canvas& canvas, float width, float height);
    void PaintToast(ui::Canvas& canvas, float width, float height);

    void DrawNavIcon(ui::Canvas& canvas, int index, const ui::Rect& rect, const Gdiplus::Color& color);
    void DrawSearchIcon(ui::Canvas& canvas, float x, float y, float size, const Gdiplus::Color& color);
    void DrawSparkIcon(ui::Canvas& canvas, const ui::Rect& rect, const Gdiplus::Color& color);
    void DrawDocumentIcon(ui::Canvas& canvas, const ui::Rect& rect, const Gdiplus::Color& color);
    void DrawCheckboxRow(ui::Canvas& canvas, const ui::Rect& row, const std::wstring& label,
                         int count, bool checked, const std::wstring& action,
                         const std::wstring& payload, bool enabled = true);
    bool DrawButton(ui::Canvas& canvas, const ui::Rect& rect, const std::wstring& text,
                    const std::wstring& action, const std::wstring& payload, int style,
                    bool enabled = true, float fontSize = 13.0f);
    void DrawFieldBackground(ui::Canvas& canvas, const ui::Rect& rect, HWND edit);
    void DrawScrollbar(ui::Canvas& canvas, ScrollState& scroll, const ui::Rect& viewport,
                       float contentHeight);
    void DrawSelectionBar(ui::Canvas& canvas, const ui::Rect& rect);
    void DrawQuestionRow(ui::Canvas& canvas, const Question& question, const ui::Rect& rect,
                         int displayIndex, bool checked, bool interactive);
    void DrawTagPill(ui::Canvas& canvas, const std::wstring& text, float x, float y,
                     const Gdiplus::Color& background, const Gdiplus::Color& foreground,
                     float maxWidth, float* measuredWidth = nullptr);
    void DrawMiniPage(ui::Canvas& canvas, const ui::Rect& rect);

    void AddHit(const ui::Rect& rect, std::wstring action, std::wstring payload = {});
    bool IsHovered(const std::wstring& action, const std::wstring& payload = {}) const;
    std::wstring MakeKey(const std::wstring& action, const std::wstring& payload) const;
    const HitTarget* HitTest(float x, float y) const;
    void Activate(const HitTarget& target);
    void OnMouseDown(float x, float y);
    void OnMouseUp(float x, float y);
    void OnMouseMove(float x, float y);
    void OnMouseWheel(float x, float y, int delta);
    void OnMouseLeave();
    void ClampScroll(ScrollState& scroll);

    void SwitchPage(Page page);
    void RebuildFiltered();
    void SyncSearchFromEdit();
    void ClearFilter();
    void ToggleTagFilter(const std::string& tag);
    void ApplyTagFromLibrary(const std::string& tag);
    GroupCounts CurrentSelectionCounts() const;

    void SaveQuestionFromEditor(const Question& question, const std::string& originalId);
    void OpenQuestionEditor(const std::string& questionId);
    void DeleteQuestion(const std::string& questionId);

    void StartAiTagging();
    void StartImageQuestionImport();
    void HandleImageFinished(WPARAM wParam, LPARAM lParam);
    void SendExamMessage();
    void GenerateExamPaper();
    void StartExam();
    void SaveCurrentExamAnswer();
    void LoadExamAnswerToEdit();
    void SubmitExam();
    void ResetExam();
    void HandleExamFinished(WPARAM wParam, LPARAM lParam);
    void ParseFunction();
    void StartFunctionExplanation();
    void GenerateGeometrySketch();
    void HandleToolFinished(WPARAM wParam, LPARAM lParam);
    void StartConnectionTest();
    void SaveAiSettings(bool notify);
    void OnProviderPreset(const std::string& provider);
    void ToggleApiKeyVisibility();
    void HandleAiUpdate(WPARAM wParam, LPARAM lParam);
    void HandleAiFinished(WPARAM wParam, LPARAM lParam);
    void HandleTestFinished(WPARAM wParam, LPARAM lParam);

    void GeneratePaper();
    void OpenPath(const std::filesystem::path& path);
    void OpenMathKeyboard(HWND target);
    void OpenContainingFolder(const std::filesystem::path& path);

    void ShowToast(const std::wstring& text, bool error = false);
    void HideToast();
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
