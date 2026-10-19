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
    value.Set("stem", stem);
    value.Set("options", StringArray(options));
    value.Set("answer", answer);
    value.Set("analysis", analysis);
    value.Set("formulas", StringArray(formulas));
    value.Set("concepts", StringArray(concepts));
    value.Set("tagged", tagged);
    value.Set("created", created);
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
    question.tagged = question.HasAnyTag();
    question.created = value["created"].AsString();
    if (question.created.empty()) {
        question.created = util::NowIso8601();
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
        std::string haystack = question.stem + "\n" + question.answer + "\n" + question.analysis;
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
           types[2] && !onlyUntagged && tags.empty() && search.empty();
}

void QuestionFilter::Reset() {
    difficulties = {{true, true, true}};
    types = {{true, true, true}};
    onlyUntagged = false;
    tags.clear();
    matchAllTags = false;
    search.clear();
}

int QuestionFilter::ActiveConditionCount() const {
    int count = 0;
    for (bool value : difficulties) if (value) ++count;
    for (bool value : types) if (value) ++count;
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
