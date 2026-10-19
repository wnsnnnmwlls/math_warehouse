#include "ai.h"
#include "dedup.h"
#include "docx.h"
#include "image_data.h"
#include "json.h"
#include "math_expr.h"
#include "math_text.h"
#include "seed.h"
#include "store.h"
#include "text_file.h"
#include "util.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <gdiplus.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "[PASS] " << message << "\n";
    } else {
        std::cout << "[FAIL] " << message << "\n";
        ++failures;
    }
}

void AppendU16(std::string& data, unsigned value) {
    data.push_back(static_cast<char>(value & 0xFF));
    data.push_back(static_cast<char>((value >> 8) & 0xFF));
}

void AppendU32(std::string& data, unsigned value) {
    data.push_back(static_cast<char>(value & 0xFF));
    data.push_back(static_cast<char>((value >> 8) & 0xFF));
    data.push_back(static_cast<char>((value >> 16) & 0xFF));
    data.push_back(static_cast<char>((value >> 24) & 0xFF));
}

std::string MakeTestBmp() {
    const int width = 96;
    const int height = 64;
    const int rowSize = ((width * 3 + 3) / 4) * 4;
    const int dataSize = rowSize * height;
    std::string bmp;
    bmp += "BM";
    AppendU32(bmp, 54 + dataSize);
    AppendU16(bmp, 0);
    AppendU16(bmp, 0);
    AppendU32(bmp, 54);
    AppendU32(bmp, 40);
    AppendU32(bmp, width);
    AppendU32(bmp, height);
    AppendU16(bmp, 1);
    AppendU16(bmp, 24);
    AppendU32(bmp, 0);
    AppendU32(bmp, dataSize);
    AppendU32(bmp, 2835);
    AppendU32(bmp, 2835);
    AppendU32(bmp, 0);
    AppendU32(bmp, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool dark = x > 20 && x < 76 && y > 16 && y < 48;
            const unsigned char v = dark ? 35 : 255;
            bmp.push_back(static_cast<char>(v));
            bmp.push_back(static_cast<char>(v));
            bmp.push_back(static_cast<char>(v));
        }
        for (int pad = width * 3; pad < rowSize; ++pad) {
            bmp.push_back('\0');
        }
    }
    return bmp;
}

}  // namespace

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "MathPaperStudio core self-test\n";
    std::cout << "----------------------------------------\n";

    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    bool gdiplusReady = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) ==
                        Gdiplus::Ok;
    Check(gdiplusReady, "GDI+ initializes for image encoding");

    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(
        "{\"name\":\"三角函数\",\"values\":[1,2.5,true,null],\"text\":\"\\u4e2d\\u6587\"}",
        &parseError);
    Check(root.IsObject(), "JSON object parses");
    Check(root["name"].AsString() == "三角函数", "JSON UTF-8 string reads");
    Check(root["text"].AsString() == "中文", "JSON Unicode escape decodes");
    Check(root["values"].size() == 4, "JSON array size");
    Check(root["values"][1].AsNumber() == 2.5, "JSON number reads");
    Check(jsonx::Value::Parse("{\"a\":1}").DumpCompact() == "{\"a\":1}", "JSON compact dump");
    Check(!jsonx::Value::Parse("{\"a\":}").IsObject(), "Invalid JSON rejected");

    const std::string latexStem =
        "已知 $\\cos\\left(\\alpha+\\frac{\\pi}{4}\\right)=\\frac{\\sqrt{2}}{2}$，则 "
        "$\\frac{\\cos\n2\\alpha}{\\sin\\alpha+\\cos\\alpha}=$（　　）";
    const std::string readableStem = NormalizeMathText(latexStem);
    Check(readableStem.find("cos(α+π/4)=√2/2") != std::string::npos,
          "LaTeX fraction and Greek letters become readable");
    Check(readableStem.find("(cos 2α)/(sin α+cos α)") != std::string::npos,
          "LaTeX fraction with complex numerator/denominator becomes readable");
    Check(readableStem.find('$') == std::string::npos &&
              readableStem.find('\\') == std::string::npos,
          "LaTeX delimiters and commands removed");
    Check(NormalizeMathText("$-\\sqrt{2}$") == "-√2", "LaTeX option becomes readable");
    Check(NormalizeMathText("$x^{2}+y_{n}$") == "x²+yₙ", "LaTeX scripts become Unicode");
    Check(NormalizeMathText("$A=\\{1,2\\}$") == "A={1,2}", "Set braces preserved");
    Check(NormalizeMathText("\\frac{1}{2}") == "1/2", "Raw LaTeX without delimiters converted");

    auto questions = BuildSeedQuestions();
    Check(questions.size() >= 20, "Seed question bank has at least 20 questions");
    int counts[3] = {0, 0, 0};
    int types[3] = {0, 0, 0};
    int maxPoints = 0;
    for (const auto& q : questions) {
        counts[static_cast<int>(q.difficulty)]++;
        types[static_cast<int>(q.type)]++;
        maxPoints += PointsForType(q.type);
        Check(q.HasAnyTag(), "Seed question has tags: " + q.id);
    }
    Check(counts[0] > 0 && counts[1] > 0 && counts[2] > 0, "All difficulties represented");
    Check(types[0] > 0 && types[1] > 0 && types[2] > 0, "All question types represented");

    QuestionFilter filter;
    filter.tags.insert("三角函数");
    filter.matchAllTags = false;
    filter.difficulties = {{false, true, true}};
    int filtered = 0;
    for (const auto& q : questions) {
        if (filter.Matches(q)) ++filtered;
    }
    Check(filtered > 0, "Tag and difficulty filter finds questions");

    TagLibrary library;
    library.Rebuild(questions);
    Check(!library.formulas.empty(), "Formula tag container populated");
    Check(!library.concepts.empty(), "Concept tag container populated");
    Check(!BuildAiPrompt(questions[0], library).empty(), "AI prompt builds");

    std::filesystem::path outputDir = std::filesystem::current_path() / "docs";
    std::filesystem::create_directories(outputDir);
    PaperOptions options;
    options.title = "2026 届高三数学模拟试卷";
    options.durationMinutes = 120;
    options.includeAnswers = true;
    options.includeAnalysis = true;
    options.includeAnswerSpace = true;
    options.notes =
        "1．答题前，请将自己的姓名、班级填写在试卷指定位置。\n"
        "2．选择题答案填涂在答题卡上，填空题和解答题答案写在试卷相应位置。";
    auto result = GenerateExamDocx(questions, options, outputDir / "示例试卷.docx");
    Check(result.ok, "DOCX generated: " + result.error);
    Check(result.totalPoints == maxPoints, "DOCX score total matches");
    Check(result.questionCount == static_cast<int>(questions.size()), "DOCX question count matches");

    if (gdiplusReady) {
        std::filesystem::create_directories(std::filesystem::current_path() / "build");
        std::filesystem::path imagePath =
            std::filesystem::current_path() / "build" / "selftest-image.bmp";
        Check(util::WriteFileBytes(imagePath, MakeTestBmp()), "Test BMP written");
        EncodedImageData encoded;
        std::string imageError;
        bool encodedOk = EncodeImageForAi(imagePath, encoded, &imageError);
        Check(encodedOk, "Test image encoded: " + imageError);
        Check(encoded.width == 96 && encoded.height == 64, "Encoded image dimensions preserved");
        Check(encoded.dataUrl.rfind("data:image/jpeg;base64,", 0) == 0,
              "Image data URL is JPEG and base64");
        Check(encoded.dataUrl.size() > 200, "Image data URL contains encoded bytes");
        std::filesystem::remove(imagePath);
    }

    std::filesystem::path testRoot = std::filesystem::current_path() / "build" / "selftest-data";
    std::error_code ec;
    std::filesystem::remove_all(testRoot, ec);
    Store store(testRoot);
    std::string warning;
    Check(store.Initialize(&warning), "Store initializes");
    Check(store.Questions().size() == questions.size(), "Store loads seeded questions");
    Check(util::FileExists(store.QuestionsPath()), "questions.json created");
    Check(util::FileExists(store.TagsPath()), "tags.json created");
    Check(util::FileExists(store.SettingsPath()), "settings.json created");

    auto* first = store.FindQuestion(store.Questions().front().id);
    if (first) {
        first->stem += "（已修改）";
        std::string error;
        Check(store.SaveQuestions(&error), "Store saves modified questions");
    } else {
        Check(false, "Store finds question by id");
    }
    Store reloaded(testRoot);
    std::string warning2;
    reloaded.Initialize(&warning2);
    Check(reloaded.Questions().front().stem.find("（已修改）") != std::string::npos,
          "Store reloads saved data");

    reloaded.Config().apiKey = "sk-selftest-secret";
    reloaded.Config().darkMode = true;
    std::string settingsError;
    Check(reloaded.SaveSettings(&settingsError), "Store saves encrypted API key");
    std::string settingsFile;
    Check(util::ReadFileBytes(reloaded.SettingsPath(), settingsFile),
          "Encrypted settings.json is readable");
    Check(settingsFile.find("sk-selftest-secret") == std::string::npos,
          "Plaintext API key is not written to disk");
    Check(settingsFile.find("api_key_protected") != std::string::npos,
          "Protected API key exists in settings.json");
    Store keyReload(testRoot);
    std::string warning3;
    keyReload.Initialize(&warning3);
    Check(keyReload.Config().apiKey == "sk-selftest-secret",
          "DPAPI-protected API key decrypts for this Windows user");
    Check(keyReload.Config().darkMode, "Dark mode preference persists in settings");

    mathexpr::Expression expression;
    std::string expressionError;
    Check(expression.Parse("x^2-4", &expressionError), "Function expression parses");
    Check(std::abs(expression.Evaluate(3.0) - 5.0) < 1e-9, "Function expression evaluates");
    std::vector<mathexpr::RootInfo> roots =
        mathexpr::FindRoots(expression, -10.0, 10.0);
    Check(roots.size() == 2 && std::abs(roots[0].x + 2.0) < 1e-6 &&
              std::abs(roots[1].x - 2.0) < 1e-6,
          "Function zeros are detected");

    Question duplicateA;
    duplicateA.stem = "计算：cos 7.5°cos 52.5°-sin 7.5°sin 52.5° 等于（ ）";
    Question duplicateB;
    duplicateB.stem = "计算: cos7.5°*cos52.5°-sin7.5°*sin52.5°等于()";
    Check(CanonicalQuestionKey(duplicateA.stem) == CanonicalQuestionKey(duplicateB.stem),
          "Missing multiplication signs are normalized for dedup");
    std::vector<Question> duplicateList{duplicateA, duplicateB};
    Check(FindDuplicateGroups(duplicateList).size() == 1, "Duplicate question groups found");
    Check(InsertImplicitMultiplication("7.5°cos 52.5°").find("°·cos") != std::string::npos,
          "Implicit multiplication can be made explicit");
    Check(NormalizeMathText("x²÷4").find("x²/4") != std::string::npos,
          "Division signs normalize to a typesettable slash");

    const std::filesystem::path textPath =
        std::filesystem::current_path() / "build" / "selftest-text.txt";
    Check(util::WriteFileBytes(textPath, "第一题：集合 A={1,2}，求 A∩A。\n第二题：sin30°=______。"),
          "TXT import fixture writes");
    TextFileResult textFile = ExtractTextFile(textPath);
    Check(textFile.ok && textFile.text.find("集合") != std::string::npos,
          "TXT import extracts UTF-8 text");
    std::filesystem::remove(textPath);
    const std::filesystem::path sampleDoc =
        std::filesystem::current_path() / "docs" / "示例试卷.docx";
    TextFileResult docxFile = ExtractTextFile(sampleDoc);
    Check(docxFile.ok && docxFile.text.find("数学") != std::string::npos,
          "DOCX import extracts WordprocessingML text");

    std::cout << "----------------------------------------\n";
    if (failures == 0) {
        std::cout << "ALL CORE TESTS PASSED\n";
    } else {
        std::cout << failures << " TEST(S) FAILED\n";
    }
    if (gdiplusReady) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
    return failures == 0 ? 0 : 1;
}
