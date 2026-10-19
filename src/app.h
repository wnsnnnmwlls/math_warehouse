#pragma once

#include "model.h"
#include "store.h"
#include "ui.h"

#include <windows.h>

#include <filesystem>
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
    enum class Page { Bank, Tags, Ai, Paper };

    enum ControlId {
        ID_SEARCH = 1001,
        ID_API_BASE = 1010,
        ID_API_MODEL = 1011,
        ID_API_KEY = 1012,
        ID_PAPER_TITLE = 1020,
        ID_PAPER_MINUTES = 1021,
        ID_PAPER_NOTES = 1022,
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

    ui::Rect searchFieldRect_{};
    ui::Rect searchEditRect_{};
    ui::Rect apiBaseFieldRect_{};
    ui::Rect apiModelFieldRect_{};
    ui::Rect apiKeyFieldRect_{};
    ui::Rect paperTitleFieldRect_{};
    ui::Rect paperMinutesFieldRect_{};
    ui::Rect paperNotesFieldRect_{};

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
    bool initialFieldsLoaded_ = false;

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
    void StartConnectionTest();
    void SaveAiSettings(bool notify);
    void OnProviderPreset(const std::string& provider);
    void ToggleApiKeyVisibility();
    void HandleAiUpdate(WPARAM wParam, LPARAM lParam);
    void HandleAiFinished(WPARAM wParam, LPARAM lParam);
    void HandleTestFinished(WPARAM wParam, LPARAM lParam);

    void GeneratePaper();
    void OpenPath(const std::filesystem::path& path);
    void OpenContainingFolder(const std::filesystem::path& path);

    void ShowToast(const std::wstring& text, bool error = false);
    void HideToast();
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
