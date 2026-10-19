#pragma once

#include "model.h"

#include <string>
#include <vector>

struct AiTagResult {
    bool ok = false;
    std::vector<std::string> formulas;
    std::vector<std::string> concepts;
    std::string error;
    std::string rawContent;
};

struct AiTestResult {
    bool ok = false;
    std::string message;
};

struct AiImageQuestionResult {
    bool ok = false;
    std::vector<Question> questions;
    std::string error;
    std::string rawContent;
};

AiTagResult AnalyzeQuestionWithAi(const Question& question,
                                  const Settings& settings,
                                  const TagLibrary& library);

// imageDataUrl must be a data URL (data:image/jpeg;base64,...).
// Extracts every complete question in the image; each question includes an AI-generated answer and
// analysis.
AiImageQuestionResult AnalyzeQuestionImageWithAi(const std::string& imageDataUrl,
                                                 const Settings& settings,
                                                 const TagLibrary& library);

AiTestResult TestAiConnection(const Settings& settings);

std::string BuildAiPrompt(const Question& question, const TagLibrary& library);
