#include "model.h"

#include "util.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {

jsonx::Value StringArray(const std::vector<std::string>& values) {
    jsonx::Value::Array array;
    array.reserve(values.size());
    for (const auto& value : values) {
        array.emplace_back(value);
    }
    return jsonx::Value(std::move(array));
}

std::vector<std::string> ReadStringArray(const jsonx::Value& value) {
    std::vector<std::string> result;
    if (!value.IsArray()) {
        return result;
    }
    for (const auto& item : value.Items()) {
        if (!item.IsString()) {
            continue;
        }
        std::string text = util::Trim(item.AsString());
        if (text.empty()) {
            continue;
        }
        bool duplicate = std::any_of(result.begin(), result.end(), [&](const std::string& old) {
            return TagsEqual(old, text);
        });
        if (!duplicate) {
            result.push_back(std::move(text));
        }
    }
    return result;
}

}  // namespace

std::string TagKey(const std::string& tag) {
    return util::ToLowerAscii(util::Trim(tag));
}

bool TagsEqual(const std::string& a, const std::string& b) {
    return TagKey(a) == TagKey(b);
}

bool Question::HasTag(const std::string& tag) const {
    auto test = [&](const std::vector<std::string>& tags) {
        return std::any_of(tags.begin(), tags.end(), [&](const std::string& item) {
            return TagsEqual(item, tag);
        });
    };
    return test(formulas) || test(concepts);
}

bool Question::HasAnyTag() const {
    return !formulas.empty() || !concepts.empty();
}

std::vector<std::string> Question::AllTags() const {
    std::vector<std::string> result;
    result.reserve(formulas.size() + concepts.size());
    for (const auto& tag : formulas) result.push_back(tag);
    for (const auto& tag : concepts) {
        bool duplicate = std::any_of(result.begin(), result.end(), [&](const std::string& old) {
            return TagsEqual(old, tag);
        });
        if (!duplicate) result.push_back(tag);
    }
    return result;
}

jsonx::Value Question::ToJson() const {
    jsonx::Value value = jsonx::Value::MakeObject();
    value.Set("id", id);
    value.Set("difficulty", DifficultyKey(difficulty));
    value.Set("type", TypeKey(type));
    value.Set("grade", GradeKey(grade));
    value.Set("favorite", favorite);
    value.Set("stem", stem);
    value.Set("options", StringArray(options));
    value.Set("answer", answer);
    value.Set("analysis", analysis);
    value.Set("formulas", StringArray(formulas));
    value.Set("concepts", StringArray(concepts));
    value.Set("tagged", tagged);
    value.Set("created", created);
    if (!imageBase64.empty()) {
        value.Set("image_base64", imageBase64);
    }
    return value;
}

Question Question::FromJson(const jsonx::Value& value) {
    Question question;
    if (value.IsString()) {
        question.stem = value.AsString();
        question.created = util::NowIso8601();
        return question;
    }
    question.id = util::Trim(value["id"].AsString());
    if (question.id.empty()) {
        question.id = util::NewId("q");
    }
    ParseDifficulty(value["difficulty"].AsString("B"), question.difficulty);
    ParseQuestionType(value["type"].AsString("choice"), question.type);
    question.stem = value["stem"].AsString();
    question.options = ReadStringArray(value["options"]);
    question.answer = value["answer"].AsString();
    question.analysis = value["analysis"].AsString();
    question.formulas = ReadStringArray(value["formulas"]);
    question.concepts = ReadStringArray(value["concepts"]);
    question.favorite = value["favorite"].AsBool(false);
    Grade parsedGrade = Grade::Unknown;
    if (ParseGrade(value["grade"].AsString(), parsedGrade)) {
        question.grade = parsedGrade;
    } else {
        question.grade = InferGrade(question);
    }
    question.tagged = question.HasAnyTag();
    question.created = value["created"].AsString();
    if (question.created.empty()) {
        question.created = util::NowIso8601();
    }
    question.imageBase64 = value["image_base64"].AsString();
    if (question.imageBase64.empty()) question.imageBase64 = value["imageBase64"].AsString();
    if (question.imageBase64.empty()) question.imageBase64 = value["image"].AsString();
    const size_t dataMarker = question.imageBase64.find("base64,");
    if (dataMarker != std::string::npos) {
        question.imageBase64.erase(0, dataMarker + 7);
    }
    while (question.type == QuestionType::Choice && question.options.size() < 4) {
        question.options.emplace_back();
    }
    return question;
}

bool TagLibrary::AddFormula(const std::string& name) {
    std::string tag = util::Trim(name);
    if (tag.empty()) {
        return false;
    }
    bool duplicate = std::any_of(formulas.begin(), formulas.end(), [&](const std::string& old) {
        return TagsEqual(old, tag);
    });
    if (!duplicate) {
        formulas.push_back(std::move(tag));
        return true;
    }
    return false;
}

bool TagLibrary::AddConcept(const std::string& name) {
    std::string tag = util::Trim(name);
    if (tag.empty()) {
        return false;
    }
    bool duplicate = std::any_of(concepts.begin(), concepts.end(), [&](const std::string& old) {
        return TagsEqual(old, tag);
    });
    if (!duplicate) {
        concepts.push_back(std::move(tag));
        return true;
    }
    return false;
}

bool TagLibrary::RemoveFormula(const std::string& name) {
    auto it = std::find_if(formulas.begin(), formulas.end(), [&](const std::string& item) {
        return TagsEqual(item, name);
    });
    if (it == formulas.end()) return false;
    formulas.erase(it);
    return true;
}

bool TagLibrary::RemoveConcept(const std::string& name) {
    auto it = std::find_if(concepts.begin(), concepts.end(), [&](const std::string& item) {
        return TagsEqual(item, name);
    });
    if (it == concepts.end()) return false;
    concepts.erase(it);
    return true;
}

bool TagLibrary::AddFromQuestion(const Question& question) {
    bool added = false;
    for (const auto& tag : question.formulas) {
        added |= AddFormula(tag);
    }
    for (const auto& tag : question.concepts) {
        added |= AddConcept(tag);
    }
    return added;
}

void TagLibrary::Rebuild(const std::vector<Question>& questions) {
    formulas.clear();
    concepts.clear();
    for (const auto& question : questions) {
        AddFromQuestion(question);
    }
}

jsonx::Value TagLibrary::ToJson() const {
    jsonx::Value value = jsonx::Value::MakeObject();
    value.Set("formulas", StringArray(formulas));
    value.Set("concepts", StringArray(concepts));
    value.Set("version", 1);
    return value;
}

TagLibrary TagLibrary::FromJson(const jsonx::Value& value) {
    TagLibrary library;
    for (const auto& tag : ReadStringArray(value["formulas"])) {
        library.AddFormula(tag);
    }
    for (const auto& tag : ReadStringArray(value["concepts"])) {
        library.AddConcept(tag);
    }
    return library;
}

bool QuestionFilter::Matches(const Question& question) const {
    int difficultyIndex = static_cast<int>(question.difficulty);
    int typeIndex = static_cast<int>(question.type);
    if (difficultyIndex < 0 || difficultyIndex > 2 || !difficulties[difficultyIndex]) {
        return false;
    }
    if (typeIndex < 0 || typeIndex > 2 || !types[typeIndex]) {
        return false;
    }
    if (grade >= 0 && static_cast<int>(question.grade) != grade) {
        return false;
    }
    if (onlyFavorites && !question.favorite) {
        return false;
    }
    if (onlyUntagged && question.tagged) {
        return false;
    }
    if (!tags.empty()) {
        if (matchAllTags) {
            for (const auto& tag : tags) {
                if (!question.HasTag(tag)) return false;
            }
        } else {
            bool any = false;
            for (const auto& tag : tags) {
                if (question.HasTag(tag)) {
                    any = true;
                    break;
                }
            }
            if (!any) return false;
        }
    }
    std::string query = util::Trim(search);
    if (!query.empty()) {
        std::string haystack = question.id + "\n" + question.stem + "\n" + question.answer + "\n" + question.analysis;
        for (const auto& option : question.options) haystack += "\n" + option;
        for (const auto& tag : question.formulas) haystack += "\n" + tag;
        for (const auto& tag : question.concepts) haystack += "\n" + tag;
        if (!util::ContainsIgnoreCase(util::ToWide(haystack), util::ToWide(query))) {
            return false;
        }
    }
    return true;
}

bool QuestionFilter::IsDefault() const {
    return difficulties[0] && difficulties[1] && difficulties[2] && types[0] && types[1] &&
           types[2] && grade < 0 && !onlyFavorites && !onlyUntagged && tags.empty() &&
           search.empty();
}

void QuestionFilter::Reset() {
    difficulties = {{true, true, true}};
    types = {{true, true, true}};
    grade = -1;
    onlyFavorites = false;
    onlyUntagged = false;
    tags.clear();
    matchAllTags = false;
    search.clear();
}

int QuestionFilter::ActiveConditionCount() const {
    int count = 0;
    for (bool value : difficulties) if (value) ++count;
    for (bool value : types) if (value) ++count;
    if (grade >= 0) ++count;
    if (onlyFavorites) ++count;
    if (onlyUntagged) ++count;
    count += static_cast<int>(tags.size());
    if (!search.empty()) ++count;
    return count;
}

const wchar_t* DifficultyName(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return L"A";
        case Difficulty::B: return L"B";
        case Difficulty::C: return L"C";
    }
    return L"B";
}

const wchar_t* DifficultyHint(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return L"基础";
        case Difficulty::B: return L"提升";
        case Difficulty::C: return L"挑战";
    }
    return L"提升";
}

const wchar_t* TypeName(QuestionType type) {
    switch (type) {
        case QuestionType::Choice: return L"选择题";
        case QuestionType::Fill: return L"填空题";
        case QuestionType::Solution: return L"解答题";
    }
    return L"选择题";
}

const wchar_t* GradeName(Grade grade) {
    switch (grade) {
        case Grade::High1: return L"高一";
        case Grade::High2: return L"高二";
        case Grade::High3: return L"高三";
        case Grade::Unknown: return L"未分级";
    }
    return L"未分级";
}

std::string DifficultyKey(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::A: return "A";
        case Difficulty::B: return "B";
        case Difficulty::C: return "C";
    }
    return "B";
}

std::string TypeKey(QuestionType type) {
    switch (type) {
        case QuestionType::Choice: return "choice";
        case QuestionType::Fill: return "fill";
        case QuestionType::Solution: return "solution";
    }
    return "choice";
}

std::string GradeKey(Grade grade) {
    switch (grade) {
        case Grade::High1: return "g1";
        case Grade::High2: return "g2";
        case Grade::High3: return "g3";
        case Grade::Unknown: return "";
    }
    return "";
}

bool ParseGrade(const std::string& text, Grade& grade) {
    std::string key = util::ToLowerAscii(util::Trim(text));
    if (key.empty() || key == "unknown" || key == "none" || key == "未分级") {
        grade = Grade::Unknown;
        return !key.empty();
    }
    if (key == "g1" || key == "高一" || key == "high1" || key == "grade1") {
        grade = Grade::High1;
        return true;
    }
    if (key == "g2" || key == "高二" || key == "high2" || key == "grade2") {
        grade = Grade::High2;
        return true;
    }
    if (key == "g3" || key == "高三" || key == "high3" || key == "grade3") {
        grade = Grade::High3;
        return true;
    }
    return false;
}

Grade InferGrade(const Question& question) {
    std::string haystack = question.stem;
    for (const auto& tag : question.formulas) haystack += "\n" + tag;
    for (const auto& tag : question.concepts) haystack += "\n" + tag;
    auto has = [&](const char* keyword) { return haystack.find(keyword) != std::string::npos; };

    const char* high3[] = {"导数", "极值", "最值", "圆锥曲线", "椭圆", "双曲线", "抛物线",
                           "离心率", "概率", "随机变量", "统计", "计数原理", "排列", "组合",
                           "二项式", "复数", "立体几何", "空间向量", "外接球", "表面积", "体积"};
    const char* high2[] = {"数列", "等差", "等比", "不等式", "直线", "圆的方程", "解析几何",
                           "正弦定理", "余弦定理", "解三角形", "平面向量", "线性规划", "空间直角坐标"};
    const char* high1[] = {"集合", "函数", "指数", "对数", "幂函数", "三角函数", "任意角",
                           "同角", "诱导公式", "二次函数", "不等式性质"};
    for (const char* keyword : high3) if (has(keyword)) return Grade::High3;
    for (const char* keyword : high2) if (has(keyword)) return Grade::High2;
    for (const char* keyword : high1) if (has(keyword)) return Grade::High1;
    return Grade::Unknown;
}

bool ParseDifficulty(const std::string& text, Difficulty& difficulty) {
    std::string key = util::ToLowerAscii(util::Trim(text));
    if (key == "a" || key == "基础" || key == "easy") {
        difficulty = Difficulty::A;
        return true;
    }
    if (key == "b" || key == "提升" || key == "medium") {
        difficulty = Difficulty::B;
        return true;
    }
    if (key == "c" || key == "挑战" || key == "hard") {
        difficulty = Difficulty::C;
        return true;
    }
    return false;
}

bool ParseQuestionType(const std::string& text, QuestionType& type) {
    std::string key = util::ToLowerAscii(util::Trim(text));
    if (key == "choice" || key == "选择题" || key == "选择") {
        type = QuestionType::Choice;
        return true;
    }
    if (key == "fill" || key == "填空题" || key == "填空") {
        type = QuestionType::Fill;
        return true;
    }
    if (key == "solution" || key == "解答题" || key == "解答") {
        type = QuestionType::Solution;
        return true;
    }
    return false;
}

int PointsForType(QuestionType type) {
    switch (type) {
        case QuestionType::Choice: return 5;
        case QuestionType::Fill: return 5;
        case QuestionType::Solution: return 12;
    }
    return 5;
}

bool QuestionHasImage(const Question& question) {
    return !util::Trim(question.imageBase64).empty();
}

bool QuestionNeedsFigure(const Question& question) {
    if (QuestionHasImage(question)) return false;
    std::string text = question.stem + "\n" + question.analysis + "\n";
    for (const auto& option : question.options) {
        text += option;
        text += "\n";
    }
    return text.find("如图") != std::string::npos ||
           text.find("下图") != std::string::npos ||
           text.find("图中") != std::string::npos ||
           text.find("图示") != std::string::npos;
}

bool QuestionNeedsCompletion(const Question& question) {
    if (util::Trim(question.answer).empty() || util::Trim(question.analysis).empty()) {
        return true;
    }
    if (QuestionNeedsFigure(question)) return true;
    if (question.type != QuestionType::Choice) return false;
    int nonEmptyOptions = 0;
    for (const auto& option : question.options) {
        if (!util::Trim(option).empty()) ++nonEmptyOptions;
    }
    if (nonEmptyOptions < 2) return true;
    const bool multiPart = question.stem.find("(1)") != std::string::npos ||
                           question.stem.find("（1）") != std::string::npos;
    if (multiPart) return false;
    if (nonEmptyOptions < 4) return true;
    std::string answer = util::Trim(question.answer);
    if (answer.empty()) return true;
    char first = static_cast<char>(std::toupper(static_cast<unsigned char>(answer[0])));
    return first < 'A' || first > 'D';
}
