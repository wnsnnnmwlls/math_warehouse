#pragma once

#include "model.h"
#include "ui.h"

#include <windows.h>

enum class QuestionEditorAction { Cancel, Save, Delete };

struct QuestionEditorResult {
    QuestionEditorAction action = QuestionEditorAction::Cancel;
    Question question;
};

QuestionEditorResult ShowQuestionEditor(HWND owner, ui::Theme& theme, const TagLibrary& library,
                                       const Question* existing, const Question* draft = nullptr);
