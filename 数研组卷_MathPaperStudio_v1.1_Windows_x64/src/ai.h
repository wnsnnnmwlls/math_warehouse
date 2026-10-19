#pragma once

#include "model.h"

#include <string>
#include <vector>

struct AiTagResult {
    bool ok = false;
    Grade grade = Grade::Unknown;
    std::vector<std::string> formulas;
    std::vector<std::string> concepts;
    std::string error;
    std::string rawContent;
};

struct AiTestResult {
    bool ok = false;
    std::string message;
};

struct AiChatResult {
    bool ok = false;
    std::string text;
    std::string error;
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

// Extracts all complete questions and generated solutions from plain text
// (for example a DOCX or TXT question sheet).
AiImageQuestionResult AnalyzeQuestionDocumentWithAi(const std::string& documentText,
                                                    const Settings& settings,
                                                    const TagLibrary& library);

AiTestResult TestAiConnection(const Settings& settings);

// Generic OpenAI-compatible chat request used by the AI exam, function analysis and
// geometry sketch tools. The caller owns prompt/schema design and parsing.
AiChatResult AskAiChat(const Settings& settings, const std::string& systemPrompt,
                       const std::string& userPrompt, int maxTokens = 1800,
                       int timeoutMs = 120000);

std::string BuildAiPrompt(const Question& question, const TagLibrary& library);
