#pragma once

#include <string>

// Full question bank compiled into the executable. This keeps the single-file
// MathPaperStudio.exe useful without requiring a sibling data directory.
std::string LoadBundledQuestionsJson();
