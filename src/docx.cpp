#include "docx.h"

#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr int kPageWidthTwips = 11906;
constexpr int kPageHeightTwips = 16838;
constexpr int kMarginTopTwips = 1247;
constexpr int kMarginBottomTwips = 1134;
constexpr int kMarginLeftTwips = 1134;
constexpr int kMarginRightTwips = 1134;

struct RunStyle {
    std::string eastAsia = "宋体";
    std::string ascii = "Times New Roman";
    int halfPoints = 21;
    bool bold = false;
    bool italic = false;
    std::string color = "000000";
};

struct ParaStyle {
    std::string align = "left";
    int before = 0;
    int after = 0;
    int line = 0;
    int left = 0;
    int firstLine = 0;
    int hanging = 0;
    std::vector<int> tabs;
    bool keepNext = false;
    bool keepLines = false;
    bool pageBreakBefore = false;
    bool bottomBorder = false;
    bool topBorder = false;
    std::string borderColor = "AEB4C0";
};

struct PlacedQuestion {
    const Question* question = nullptr;
    int number = 0;
};

struct PaperGroup {
    QuestionType type = QuestionType::Choice;
    int points = 5;
    std::vector<PlacedQuestion> items;
};

struct PaperModel {
    std::vector<PaperGroup> groups;
    int totalPoints = 0;
    int questionCount = 0;
    int duration = 120;
};

struct DocxImage {
    std::string fileName;
    std::string contentType;
    std::string bytes;
    std::string relationshipId;
    int widthEmu = 0;
    int heightEmu = 0;
};

std::string XmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r') {
                    out.push_back(' ');
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    return out;
}

std::string Int(int value) {
    return std::to_string(value);
}

std::string RunProps(const RunStyle& style) {
    std::string props = "<w:rPr>";
    props += "<w:rFonts w:ascii=\"" + XmlEscape(style.ascii) + "\" w:hAnsi=\"" +
             XmlEscape(style.ascii) + "\" w:eastAsia=\"" + XmlEscape(style.eastAsia) +
             "\" w:cs=\"" + XmlEscape(style.ascii) + "\"/>";
    if (style.bold) props += "<w:b/><w:bCs/>";
    if (style.italic) props += "<w:i/><w:iCs/>";
    props += "<w:color w:val=\"" + style.color + "\"/>";
    props += "<w:sz w:val=\"" + Int(style.halfPoints) + "\"/><w:szCs w:val=\"" +
             Int(style.halfPoints) + "\"/>";
    props += "</w:rPr>";
    return props;
}

std::string TextRunPiece(const std::string& text, const RunStyle& style) {
    if (text.empty()) return {};
    std::string run = "<w:r>" + RunProps(style);
    run += "<w:t xml:space=\"preserve\">" + XmlEscape(text) + "</w:t>";
    run += "</w:r>";
    return run;
}

std::string MakeRun(const std::string& text, const RunStyle& style) {
    if (text.empty()) return {};
    std::string result;
    std::string current;
    auto flush = [&]() {
        if (!current.empty()) {
            result += TextRunPiece(current, style);
            current.clear();
        }
    };
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\n') {
            flush();
            result += "<w:r>" + RunProps(style) + "<w:br/></w:r>";
        } else if (c == '\t') {
            flush();
            result += "<w:r>" + RunProps(style) + "<w:tab/></w:r>";
        } else if (c == '\r') {
            continue;
        } else {
            current.push_back(c);
        }
    }
    flush();
    return result;
}

std::string ParagraphProps(const ParaStyle& style) {
    std::string props = "<w:pPr>";
    if (style.pageBreakBefore) props += "<w:pageBreakBefore/>";
    if (style.keepNext) props += "<w:keepNext/>";
    if (style.keepLines) props += "<w:keepLines/>";
    if (style.topBorder || style.bottomBorder) {
        props += "<w:pBdr>";
        if (style.topBorder) {
            props += "<w:top w:val=\"single\" w:sz=\"6\" w:space=\"4\" w:color=\"" +
                     style.borderColor + "\"/>";
        }
        if (style.bottomBorder) {
            props += "<w:bottom w:val=\"single\" w:sz=\"6\" w:space=\"4\" w:color=\"" +
                     style.borderColor + "\"/>";
        }
        props += "</w:pBdr>";
    }
    if (!style.tabs.empty()) {
        props += "<w:tabs>";
        for (int tab : style.tabs) {
            props += "<w:tab w:val=\"left\" w:pos=\"" + Int(tab) + "\"/>";
        }
        props += "</w:tabs>";
    }
    if (style.before > 0 || style.after > 0 || style.line > 0) {
        props += "<w:spacing";
        if (style.before > 0) props += " w:before=\"" + Int(style.before) + "\"";
        if (style.after > 0) props += " w:after=\"" + Int(style.after) + "\"";
        if (style.line > 0) {
            props += " w:line=\"" + Int(style.line) + "\" w:lineRule=\"auto\"";
        }
        props += "/>";
    }
    if (style.left > 0 || style.firstLine > 0 || style.hanging > 0) {
        props += "<w:ind";
        if (style.left > 0) props += " w:left=\"" + Int(style.left) + "\"";
        if (style.hanging > 0) {
            props += " w:hanging=\"" + Int(style.hanging) + "\"";
        } else if (style.firstLine > 0) {
            props += " w:firstLine=\"" + Int(style.firstLine) + "\"";
        }
        props += "/>";
    }
    if (!style.align.empty()) {
        props += "<w:jc w:val=\"" + style.align + "\"/>";
    }
    props += "</w:pPr>";
    return props;
}

std::string Paragraph(const std::string& runs, const ParaStyle& style) {
    return "<w:p>" + ParagraphProps(style) + runs + "</w:p>";
}

std::string TextParagraph(const std::string& text, const RunStyle& run, const ParaStyle& para) {
    return Paragraph(MakeRun(text, run), para);
}

std::string EmptyParagraph(const ParaStyle& style) {
    return Paragraph({}, style);
}

std::string ImageParagraph(const DocxImage& image, int index) {
    const std::string id = std::to_string(index);
    return "<w:p><w:pPr><w:jc w:val=\"center\"/><w:spacing w:before=\"80\" w:after=\"100\"/>"
           "</w:pPr><w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">"
           "<wp:extent cx=\"" + Int(image.widthEmu) + "\" cy=\"" + Int(image.heightEmu) + "\"/>"
           "<wp:docPr id=\"" + id + "\" name=\"Picture " + id + "\" descr=\"题目配图\"/>"
           "<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
           "<pic:pic><pic:nvPicPr><pic:cNvPr id=\"" + id + "\" name=\"" +
           XmlEscape(image.fileName) + "\"/><pic:cNvPicPr/></pic:nvPicPr>"
           "<pic:blipFill><a:blip r:embed=\"" + image.relationshipId +
           "\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>"
           "<pic:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"" +
           Int(image.widthEmu) + "\" cy=\"" + Int(image.heightEmu) +
           "\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr>"
           "</pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p>";
}

bool ImageDimensions(const std::string& bytes, int& width, int& height) {
    if (bytes.size() < 8) return false;
    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) || !stream) return false;
    ULONG written = 0;
    if (FAILED(stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written)) ||
        written != bytes.size()) {
        stream->Release();
        return false;
    }
    LARGE_INTEGER zero{};
    stream->Seek(zero, STREAM_SEEK_SET, nullptr);
    bool ok = false;
    {
        Gdiplus::Image image(stream, TRUE);
        if (image.GetLastStatus() == Gdiplus::Ok && image.GetWidth() > 0 &&
            image.GetHeight() > 0) {
            width = static_cast<int>(image.GetWidth());
            height = static_cast<int>(image.GetHeight());
            ok = true;
        }
    }
    stream->Release();
    return ok;
}

void PrepareQuestionImages(const std::vector<Question>& questions,
                           std::unordered_map<std::string, DocxImage>& images) {
    int nextId = 4;
    for (const auto& question : questions) {
        if (question.imageBase64.empty() || images.find(question.id) != images.end()) continue;
        const std::vector<unsigned char> decoded = util::Base64Decode(question.imageBase64);
        if (decoded.size() < 8) continue;
        std::string extension;
        std::string contentType;
        if (decoded[0] == 0x89 && decoded[1] == 0x50 && decoded[2] == 0x4E &&
            decoded[3] == 0x47) {
            extension = "png";
            contentType = "image/png";
        } else if (decoded[0] == 0xFF && decoded[1] == 0xD8) {
            extension = "jpeg";
            contentType = "image/jpeg";
        } else {
            continue;
        }
        const std::string bytes(decoded.begin(), decoded.end());
        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!ImageDimensions(bytes, pixelWidth, pixelHeight)) continue;
        const double naturalWidth = pixelWidth * 9525.0;
        const double naturalHeight = pixelHeight * 9525.0;
        const double maxWidth = 5000000.0;
        const double maxHeight = 3400000.0;
        const double factor =
            std::min(1.0, std::min(maxWidth / naturalWidth, maxHeight / naturalHeight));
        DocxImage image;
        image.fileName = "image" + std::to_string(nextId) + "." + extension;
        image.contentType = contentType;
        image.bytes = bytes;
        image.relationshipId = "rIdImage" + std::to_string(nextId);
        image.widthEmu = static_cast<int>(std::lround(naturalWidth * factor));
        image.heightEmu = static_cast<int>(std::lround(naturalHeight * factor));
        images[question.id] = std::move(image);
        ++nextId;
    }
}

std::string OptionsBlock(const Question& question) {
    if (question.options.empty()) return {};
    RunStyle optionStyle;
    optionStyle.halfPoints = 21;
    const std::string labels[] = {"A．", "B．", "C．", "D．", "E．", "F．"};
    std::string result;
    for (size_t i = 0; i < question.options.size(); i += 2) {
        std::string text = labels[i] + question.options[i];
        if (i + 1 < question.options.size()) {
            text += "\t";
            text += labels[i + 1] + question.options[i + 1];
        }
        ParaStyle optionPara;
        optionPara.left = 300;
        optionPara.tabs = {4700};
        optionPara.line = 340;
        optionPara.after = 40;
        optionPara.keepLines = true;
        optionPara.keepNext = i + 2 < question.options.size();
        result += TextParagraph(text, optionStyle, optionPara);
    }
    return result;
}

std::string RomanForSection(size_t index) {
    static const char* values[] = {"一", "二", "三", "四", "五", "六"};
    return index < 6 ? values[index] : std::to_string(index + 1);
}

std::string BuildSectionIntro(const PaperGroup& group, size_t sectionIndex) {
    const int count = static_cast<int>(group.items.size());
    const int total = count * group.points;
    const std::string type = util::ToUtf8(TypeName(group.type));
    std::string intro = RomanForSection(sectionIndex) + "、" + type + "：本题共 " +
                        Int(count) + " 小题，每小题 " + Int(group.points) + " 分，共 " +
                        Int(total) + " 分。";
    if (group.type == QuestionType::Choice) {
        intro += "在每小题给出的四个选项中，只有一项是符合题目要求的。";
    } else if (group.type == QuestionType::Fill) {
        intro += "请将答案填写在题中的横线上。";
    } else {
        intro += "解答应写出文字说明、证明过程或演算步骤。";
    }
    return intro;
}

std::string FooterXml() {
    RunStyle footerStyle;
    footerStyle.eastAsia = "宋体";
    footerStyle.halfPoints = 18;
    footerStyle.color = "555B68";
    std::string pPr = "<w:pPr><w:pBdr><w:top w:val=\"single\" w:sz=\"4\" w:space=\"4\" "
                      "w:color=\"B7BCC8\"/></w:pBdr><w:jc w:val=\"center\"/>"
                      "<w:spacing w:before=\"60\" w:line=\"240\" w:lineRule=\"auto\"/></w:pPr>";
    std::string runProps = RunProps(footerStyle);
    std::string text = "<w:p>" + pPr;
    text += "<w:r>" + runProps + "<w:t xml:space=\"preserve\">数学试题　第 </w:t></w:r>";
    text += "<w:fldSimple w:instr=\" PAGE \"><w:r>" + runProps +
            "<w:t>1</w:t></w:r></w:fldSimple>";
    text += "<w:r>" + runProps + "<w:t xml:space=\"preserve\"> 页（共 </w:t></w:r>";
    text += "<w:fldSimple w:instr=\" NUMPAGES \"><w:r>" + runProps +
            "<w:t>1</w:t></w:r></w:fldSimple>";
    text += "<w:r>" + runProps + "<w:t xml:space=\"preserve\"> 页）</w:t></w:r>";
    text += "</w:p>";
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<w:ftr xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">" +
           text + "</w:ftr>";
}

PaperModel PreparePaper(const std::vector<Question>& questions, const PaperOptions& options) {
    PaperModel model;
    model.duration = util::ClampInt(options.durationMinutes, 1, 600);
    for (QuestionType type : {QuestionType::Choice, QuestionType::Fill, QuestionType::Solution}) {
        PaperGroup group;
        group.type = type;
        group.points = PointsForType(type);
        for (const auto& question : questions) {
            if (question.type == type) {
                group.items.push_back(PlacedQuestion{&question, 0});
            }
        }
        std::stable_sort(group.items.begin(), group.items.end(),
                         [](const PlacedQuestion& a, const PlacedQuestion& b) {
                             return static_cast<int>(a.question->difficulty) <
                                    static_cast<int>(b.question->difficulty);
                         });
        if (!group.items.empty()) {
            model.groups.push_back(std::move(group));
        }
    }
    int number = 1;
    for (auto& group : model.groups) {
        for (auto& item : group.items) {
            item.number = number++;
            model.totalPoints += group.points;
            ++model.questionCount;
        }
    }
    return model;
}

std::string BuildDocument(const PaperOptions& options, PaperModel& model,
                          const std::unordered_map<std::string, DocxImage>& images) {
    std::string body;
    RunStyle titleStyle;
    titleStyle.eastAsia = "黑体";
    titleStyle.ascii = "Arial";
    titleStyle.halfPoints = 40;
    titleStyle.bold = true;
    ParaStyle titlePara;
    titlePara.align = "center";
    titlePara.after = 120;
    titlePara.line = 480;
    std::string title = util::Trim(options.title);
    if (title.empty()) title = "数学试卷";
    body += TextParagraph(title, titleStyle, titlePara);

    RunStyle metaStyle;
    metaStyle.halfPoints = 21;
    ParaStyle metaPara;
    metaPara.align = "center";
    metaPara.after = 100;
    metaPara.line = 320;
    body += TextParagraph("考试时间：" + Int(model.duration) + " 分钟　　满分：" +
                              Int(model.totalPoints) + " 分",
                          metaStyle, metaPara);

    RunStyle noteStyle;
    noteStyle.halfPoints = 18;
    noteStyle.color = "333333";
    ParaStyle noteTitlePara;
    noteTitlePara.line = 300;
    noteTitlePara.after = 30;
    noteTitlePara.before = 40;
    ParaStyle notePara;
    notePara.line = 300;
    notePara.after = 10;
    if (!util::Trim(options.notes).empty()) {
        body += TextParagraph("注意事项：", noteStyle, noteTitlePara);
        for (const auto& line : util::SplitLines(options.notes)) {
            std::string trimmed = util::Trim(line);
            if (trimmed.empty()) continue;
            body += TextParagraph(trimmed, noteStyle, notePara);
        }
    }
    ParaStyle separator;
    separator.topBorder = true;
    separator.before = 80;
    separator.after = 90;
    separator.line = 120;
    body += EmptyParagraph(separator);

    RunStyle sectionStyle;
    sectionStyle.eastAsia = "黑体";
    sectionStyle.ascii = "Arial";
    sectionStyle.halfPoints = 24;
    sectionStyle.bold = true;
    ParaStyle sectionPara;
    sectionPara.before = 140;
    sectionPara.after = 100;
    sectionPara.line = 340;
    sectionPara.keepNext = true;
    sectionPara.keepLines = true;

    RunStyle bodyStyle;
    bodyStyle.halfPoints = 21;
    RunStyle numberStyle = bodyStyle;
    numberStyle.bold = true;

    for (size_t sectionIndex = 0; sectionIndex < model.groups.size(); ++sectionIndex) {
        const auto& group = model.groups[sectionIndex];
        body += TextParagraph(BuildSectionIntro(group, sectionIndex), sectionStyle, sectionPara);

        for (const auto& placed : group.items) {
            const Question& question = *placed.question;
            ParaStyle questionPara;
            questionPara.before = 110;
            questionPara.after = 40;
            questionPara.line = 360;
            questionPara.keepLines = true;
            questionPara.keepNext = group.type == QuestionType::Choice;
            if (options.includeAnswerSpace) {
                if (group.type == QuestionType::Fill) {
                    questionPara.after = 720;
                } else if (group.type == QuestionType::Solution) {
                    // Paragraph spacing keeps writing space without generating empty paragraphs
                    // that could spill onto an otherwise blank page.
                    questionPara.after = 2600;
                }
            }
            RunStyle qNumberStyle = numberStyle;
            std::string runs = MakeRun(Int(placed.number) + "．", qNumberStyle);
            runs += MakeRun(question.stem, bodyStyle);
            body += Paragraph(runs, questionPara);

            auto image = images.find(question.id);
            if (image != images.end()) {
                body += ImageParagraph(image->second, placed.number);
            } else if (QuestionNeedsFigure(question)) {
                RunStyle missingStyle;
                missingStyle.halfPoints = 19;
                missingStyle.color = "B23A48";
                ParaStyle missingPara;
                missingPara.align = "center";
                missingPara.after = 90;
                body += TextParagraph("【配图缺失：原题提到“如图”，题库中未收录配图】",
                                      missingStyle, missingPara);
            }

            if (group.type == QuestionType::Choice) {
                body += OptionsBlock(question);
            }
        }
    }

    const bool withAnswers = options.includeAnswers;
    if (withAnswers) {
        RunStyle answerTitleStyle;
        answerTitleStyle.eastAsia = "黑体";
        answerTitleStyle.ascii = "Arial";
        answerTitleStyle.halfPoints = 34;
        answerTitleStyle.bold = true;
        ParaStyle answerTitlePara;
        answerTitlePara.align = "center";
        answerTitlePara.after = 160;
        answerTitlePara.line = 420;
        answerTitlePara.pageBreakBefore = true;
        body += TextParagraph("参考答案与解析", answerTitleStyle, answerTitlePara);

        RunStyle answerGroupStyle = sectionStyle;
        answerGroupStyle.halfPoints = 24;
        ParaStyle answerGroupPara;
        answerGroupPara.before = 120;
        answerGroupPara.after = 70;
        answerGroupPara.line = 320;
        answerGroupPara.keepNext = true;

        RunStyle answerStyle = bodyStyle;
        answerStyle.halfPoints = 20;
        ParaStyle answerPara;
        answerPara.line = 340;
        answerPara.after = 40;
        answerPara.keepNext = options.includeAnalysis;
        ParaStyle analysisPara;
        analysisPara.line = 320;
        analysisPara.after = 100;
        analysisPara.left = 220;
        RunStyle analysisStyle;
        analysisStyle.halfPoints = 19;
        analysisStyle.color = "333333";

        for (size_t sectionIndex = 0; sectionIndex < model.groups.size(); ++sectionIndex) {
            const auto& group = model.groups[sectionIndex];
            body += TextParagraph(RomanForSection(sectionIndex) + "、" +
                                      util::ToUtf8(TypeName(group.type)),
                                  answerGroupStyle, answerGroupPara);
            for (const auto& placed : group.items) {
                const Question& question = *placed.question;
                std::string answer = util::Trim(question.answer);
                if (answer.empty()) answer = "（略）";
                std::string answerRuns = MakeRun(Int(placed.number) + "．", numberStyle);
                answerRuns += MakeRun(answer, answerStyle);
                body += Paragraph(answerRuns, answerPara);
                if (options.includeAnalysis && !util::Trim(question.analysis).empty()) {
                    std::string analysisRuns = MakeRun("【解析】", answerStyle);
                    analysisRuns += MakeRun(question.analysis, analysisStyle);
                    body += Paragraph(analysisRuns, analysisPara);
                }
            }
        }
    }

    std::string sectPr;
    sectPr += "<w:sectPr>";
    sectPr += "<w:footerReference w:type=\"default\" r:id=\"rId3\"/>";
    sectPr += "<w:pgSz w:w=\"" + Int(kPageWidthTwips) + "\" w:h=\"" + Int(kPageHeightTwips) + "\"/>";
    sectPr += "<w:pgMar w:top=\"" + Int(kMarginTopTwips) + "\" w:right=\"" +
              Int(kMarginRightTwips) + "\" w:bottom=\"" + Int(kMarginBottomTwips) +
              "\" w:left=\"" + Int(kMarginLeftTwips) + "\" w:header=\"567\" w:footer=\"567\" "
              "w:gutter=\"0\"/>";
    sectPr += "<w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"312\"/></w:sectPr>";

    std::string document = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    document += "<w:document "
                "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
                "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
                "xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\" "
                "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" "
                "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
                "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">";
    document += "<w:body>" + body + sectPr + "</w:body></w:document>";
    return document;
}

std::string StylesXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
           "<w:docDefaults><w:rPrDefault><w:rPr>"
           "<w:rFonts w:ascii=\"Times New Roman\" w:hAnsi=\"Times New Roman\" "
           "w:eastAsia=\"宋体\" w:cs=\"Times New Roman\"/>"
           "<w:sz w:val=\"21\"/><w:szCs w:val=\"21\"/></w:rPr></w:rPrDefault>"
           "<w:pPrDefault><w:pPr><w:spacing w:after=\"0\" w:line=\"340\" "
           "w:lineRule=\"auto\"/></w:pPr></w:pPrDefault></w:docDefaults>"
           "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
           "<w:name w:val=\"Normal\"/><w:qFormat/></w:style>"
           "</w:styles>";
}

std::string SettingsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<w:settings xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
           "<w:zoom w:percent=\"100\"/>"
           "<w:defaultTabStop w:val=\"420\"/>"
           "<w:updateFields w:val=\"true\"/>"
           "<w:compat><w:compatSetting w:name=\"compatibilityMode\" "
           "w:uri=\"http://schemas.microsoft.com/office/word\" w:val=\"15\"/></w:compat>"
           "</w:settings>";
}

std::string ContentTypesXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
           "<Default Extension=\"png\" ContentType=\"image/png\"/>"
           "<Default Extension=\"jpeg\" ContentType=\"image/jpeg\"/>"
           "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
           "<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
           "<Override PartName=\"/word/settings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
           "<Override PartName=\"/word/footer1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>"
           "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
           "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
           "</Types>";
}

std::string RootRelsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
           "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
           "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
           "</Relationships>";
}

std::string DocumentRelsXml(const std::unordered_map<std::string, DocxImage>& images) {
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
                      "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
                      "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" Target=\"settings.xml\"/>"
                      "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer\" Target=\"footer1.xml\"/>";
    for (const auto& entry : images) {
        const auto& image = entry.second;
        xml += "<Relationship Id=\"" + image.relationshipId +
               "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
               "Target=\"media/" + image.fileName + "\"/>";
    }
    xml += "</Relationships>";
    return xml;
}

std::string CoreXml(const std::string& title) {
    std::string now = util::NowIso8601();
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<cp:coreProperties "
           "xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
           "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
           "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
           "xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
           "<dc:title>" + XmlEscape(title) + "</dc:title>"
           "<dc:creator>数研组卷</dc:creator>"
           "<cp:lastModifiedBy>数研组卷</cp:lastModifiedBy>"
           "<dcterms:created xsi:type=\"dcterms:W3CDTF\">" + now + "</dcterms:created>"
           "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">" + now + "</dcterms:modified>"
           "</cp:coreProperties>";
}

std::string AppXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" "
           "xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\">"
           "<Application>数研组卷</Application>"
           "<AppVersion>1.0</AppVersion>"
           "</Properties>";
}

uint32_t Crc32(const std::string& data) {
    static uint32_t table[256];
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value & 1u) ? (0xEDB88320u ^ (value >> 1)) : (value >> 1);
            }
            table[i] = value;
        }
        initialized = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : data) {
        crc = table[(crc ^ c) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void Put16(std::vector<unsigned char>& out, uint16_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

void Put32(std::vector<unsigned char>& out, uint32_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

struct ZipEntry {
    std::string name;
    std::string data;
    uint32_t crc = 0;
    uint32_t offset = 0;
};

bool BuildZip(const std::vector<std::pair<std::string, std::string>>& files, std::string& output,
              std::string* error) {
    SYSTEMTIME local{};
    GetLocalTime(&local);
    uint16_t dosTime = static_cast<uint16_t>((local.wHour << 11) | (local.wMinute << 5) |
                                             (local.wSecond / 2));
    uint16_t dosDate = static_cast<uint16_t>(((local.wYear - 1980) << 9) | (local.wMonth << 5) |
                                             local.wDay);
    std::vector<unsigned char> bytes;
    std::vector<ZipEntry> entries;
    entries.reserve(files.size());
    for (const auto& file : files) {
        if (file.first.empty()) continue;
        if (file.first.size() > 0xFFFF || file.second.size() > 0xFFFFFFFFull) {
            if (error) *error = "文件过大，无法写入 DOCX。";
            return false;
        }
        ZipEntry entry;
        entry.name = file.first;
        entry.data = file.second;
        entry.crc = Crc32(entry.data);
        entry.offset = static_cast<uint32_t>(bytes.size());

        Put32(bytes, 0x04034B50);
        Put16(bytes, 20);
        Put16(bytes, 0x0800);
        Put16(bytes, 0);
        Put16(bytes, dosTime);
        Put16(bytes, dosDate);
        Put32(bytes, entry.crc);
        Put32(bytes, static_cast<uint32_t>(entry.data.size()));
        Put32(bytes, static_cast<uint32_t>(entry.data.size()));
        Put16(bytes, static_cast<uint16_t>(entry.name.size()));
        Put16(bytes, 0);
        bytes.insert(bytes.end(), entry.name.begin(), entry.name.end());
        bytes.insert(bytes.end(), entry.data.begin(), entry.data.end());
        entries.push_back(std::move(entry));
    }
    const uint32_t centralOffset = static_cast<uint32_t>(bytes.size());
    for (const auto& entry : entries) {
        Put32(bytes, 0x02014B50);
        Put16(bytes, 20);
        Put16(bytes, 20);
        Put16(bytes, 0x0800);
        Put16(bytes, 0);
        Put16(bytes, dosTime);
        Put16(bytes, dosDate);
        Put32(bytes, entry.crc);
        Put32(bytes, static_cast<uint32_t>(entry.data.size()));
        Put32(bytes, static_cast<uint32_t>(entry.data.size()));
        Put16(bytes, static_cast<uint16_t>(entry.name.size()));
        Put16(bytes, 0);
        Put16(bytes, 0);
        Put16(bytes, 0);
        Put16(bytes, 0);
        Put32(bytes, 0);
        Put32(bytes, entry.offset);
        bytes.insert(bytes.end(), entry.name.begin(), entry.name.end());
    }
    const uint32_t centralSize = static_cast<uint32_t>(bytes.size()) - centralOffset;
    Put32(bytes, 0x06054B50);
    Put16(bytes, 0);
    Put16(bytes, 0);
    Put16(bytes, static_cast<uint16_t>(entries.size()));
    Put16(bytes, static_cast<uint16_t>(entries.size()));
    Put32(bytes, centralSize);
    Put32(bytes, centralOffset);
    Put16(bytes, 0);
    output.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

}  // namespace

DocxBuildResult GenerateExamDocx(const std::vector<Question>& questions,
                                 const PaperOptions& options,
                                 const std::filesystem::path& outputPath) {
    DocxBuildResult result;
    result.questionCount = static_cast<int>(questions.size());
    if (questions.empty()) {
        result.error = "没有已选择的题目，无法生成试卷。";
        return result;
    }
    PaperModel model = PreparePaper(questions, options);
    result.totalPoints = model.totalPoints;
    std::unordered_map<std::string, DocxImage> images;
    PrepareQuestionImages(questions, images);
    std::string document = BuildDocument(options, model, images);

    std::string zip;
    std::string zipError;
    std::vector<std::pair<std::string, std::string>> files = {
        {"[Content_Types].xml", ContentTypesXml()},
        {"_rels/.rels", RootRelsXml()},
        {"docProps/core.xml", CoreXml(util::Trim(options.title))},
        {"docProps/app.xml", AppXml()},
        {"word/document.xml", document},
        {"word/styles.xml", StylesXml()},
        {"word/settings.xml", SettingsXml()},
        {"word/footer1.xml", FooterXml()},
        {"word/_rels/document.xml.rels", DocumentRelsXml(images)},
    };
    for (const auto& entry : images) {
        files.push_back({"word/media/" + entry.second.fileName, entry.second.bytes});
    }
    if (!BuildZip(files, zip, &zipError)) {
        result.error = zipError;
        return result;
    }
    std::string writeError;
    if (!util::WriteFileAtomic(outputPath, zip, &writeError)) {
        result.error = writeError;
        return result;
    }
    result.ok = true;
    return result;
}
