#pragma once

#include "json.h"

#include <array>
#include <set>
#include <string>
#include <vector>

enum class Difficulty { A = 0, B = 1, C = 2 };
enum class QuestionType { Choice = 0, Fill = 1, Solution = 2 };

struct Question {
    std::string id;
    Difficulty difficulty = Difficulty::B;
    QuestionType type = QuestionType::Choice;
    std::string stem;
    std::vector<std::string> options;
    std::string answer;
    std::string analysis;
    std::vector<std::string> formulas;
    std::vector<std::string> concepts;
    bool tagged = false;
    std::string created;

    bool HasTag(const std::string& tag) const;
    bool HasAnyTag() const;
    std::vector<std::string> AllTags() const;
    jsonx::Value ToJson() const;
    static Question FromJson(const jsonx::Value& value);
};

struct TagLibrary {
    std::vector<std::string> formulas;
    std::vector<std::string> concepts;

    bool AddFormula(const std::string& name);
    bool AddConcept(const std::string& name);
    bool RemoveFormula(const std::string& name);
    bool RemoveConcept(const std::string& name);
    bool AddFromQuestion(const Question& question);
    void Rebuild(const std::vector<Question>& questions);
    jsonx::Value ToJson() const;
    static TagLibrary FromJson(const jsonx::Value& value);
};

struct Settings {
    std::string apiBase = "https://api.deepseek.com";
    std::string apiKey;
    std::string model = "deepseek-chat";
    double temperature = 0.2;
    bool darkMode = false;
};

struct QuestionFilter {
    std::array<bool, 3> difficulties{{true, true, true}};
    std::array<bool, 3> types{{true, true, true}};
    bool onlyUntagged = false;
    std::set<std::string> tags;
    bool matchAllTags = false;
    std::string search;

    bool Matches(const Question& question) const;
    bool IsDefault() const;
    void Reset();
    int ActiveConditionCount() const;
};

const wchar_t* DifficultyName(Difficulty difficulty);
const wchar_t* DifficultyHint(Difficulty difficulty);
const wchar_t* TypeName(QuestionType type);
std::string DifficultyKey(Difficulty difficulty);
std::string TypeKey(QuestionType type);
bool ParseDifficulty(const std::string& text, Difficulty& difficulty);
bool ParseQuestionType(const std::string& text, QuestionType& type);
int PointsForType(QuestionType type);

std::string TagKey(const std::string& tag);
bool TagsEqual(const std::string& a, const std::string& b);
