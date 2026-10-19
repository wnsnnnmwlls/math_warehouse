#pragma once

#include "model.h"

#include <filesystem>
#include <string>
#include <vector>

class Store {
public:
    explicit Store(std::filesystem::path appRoot);

    bool Initialize(std::string* warning = nullptr);
    bool SaveQuestions(std::string* error = nullptr);
    bool SaveTags(std::string* error = nullptr);
    bool SaveSettings(std::string* error = nullptr);
    bool SaveAll(std::string* error = nullptr);

    Question* FindQuestion(const std::string& id);
    const Question* FindQuestion(const std::string& id) const;
    int TagUsage(const std::string& tag) const;
    int FormulaUsage(const std::string& tag) const;
    int ConceptUsage(const std::string& tag) const;
    void MergeTagsFromQuestions();
    void MergeQuestionTags(const Question& question);

    std::vector<Question>& Questions() { return questions_; }
    const std::vector<Question>& Questions() const { return questions_; }
    TagLibrary& Tags() { return tags_; }
    const TagLibrary& Tags() const { return tags_; }
    Settings& Config() { return settings_; }
    const Settings& Config() const { return settings_; }

    const std::filesystem::path& AppRoot() const { return appRoot_; }
    const std::filesystem::path& DataDir() const { return dataDir_; }
    std::filesystem::path QuestionsPath() const;
    std::filesystem::path TagsPath() const;
    std::filesystem::path SettingsPath() const;

private:
    std::filesystem::path appRoot_;
    std::filesystem::path dataDir_;
    std::vector<Question> questions_;
    TagLibrary tags_;
    Settings settings_;

    bool LoadQuestions(std::string* warning);
    bool LoadTags(std::string* warning);
    bool LoadSettings(std::string* warning);
    bool EnsureDataDirectory(std::string* error);
    jsonx::Value SerializeQuestions() const;
    bool SerializeSettings(jsonx::Value& root, std::string* error) const;
};
