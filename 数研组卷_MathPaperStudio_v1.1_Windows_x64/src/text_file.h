#pragma once

#include <filesystem>
#include <string>

// Plain-text extraction for AI import.
//
// Supported inputs:
//   .txt / .md  UTF-8 (optionally with a UTF-8 BOM), or UTF-16LE with a BOM.
//   .docx       Office Open XML package (stored or DEFLATE-compressed members).
//
// .doc and .pdf are recognized explicitly but are not extractable without a
// third-party decoder, so ExtractTextFile() fails with an actionable message
// instead of returning questionable text.
struct TextFileResult {
    bool ok = false;
    std::string text;  // UTF-8, newlines normalized to '\n'
    std::string error;
    std::string warning;
    int textRuns = 0;
    int mathTypeObjects = 0;
    int drawingObjects = 0;
};

TextFileResult ExtractTextFile(const std::filesystem::path& path);

// True exactly when ExtractTextFile() can extract content: .txt, .md and .docx
// (extension comparison is ASCII case-insensitive). .doc/.pdf return false.
bool IsSupportedTextDocument(const std::filesystem::path& path);
