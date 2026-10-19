#pragma once

#include "model.h"

#include <filesystem>
#include <string>
#include <vector>

struct PaperOptions {
    std::string title = "高三数学模拟试卷";
    int durationMinutes = 120;
    bool includeAnswers = true;
    bool includeAnalysis = true;
    bool includeAnswerSpace = true;
    std::string notes;
};

struct DocxBuildResult {
    bool ok = false;
    int totalPoints = 0;
    int questionCount = 0;
    std::string error;
};

DocxBuildResult GenerateExamDocx(const std::vector<Question>& questions,
                                 const PaperOptions& options,
                                 const std::filesystem::path& outputPath);
