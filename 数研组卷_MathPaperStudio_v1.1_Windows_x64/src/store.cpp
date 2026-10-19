#include "store.h"

#include "seed.h"
#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <dpapi.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <utility>

namespace {

void AppendWarning(std::string* warning, const std::string& text) {
    if (!warning) {
        return;
    }
    if (!warning->empty()) {
        *warning += "\n";
    }
    *warning += text;
}

bool ProtectSecret(const std::string& plain, std::string& encoded, std::string* error) {
    if (plain.empty()) {
        encoded.clear();
        return true;
    }
    DATA_BLOB input{};
    input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
    input.cbData = static_cast<DWORD>(plain.size());
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"MathPaperStudio API key", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        if (error) {
            *error = "API 密钥加密失败（Windows DPAPI 错误 " +
                     std::to_string(static_cast<unsigned long>(GetLastError())) + "）";
        }
        return false;
    }
    std::vector<unsigned char> bytes(output.pbData, output.pbData + output.cbData);
    LocalFree(output.pbData);
    encoded = util::Base64Encode(bytes);
    return true;
}

bool UnprotectSecret(const std::string& encoded, std::string& plain, std::string* error) {
    plain.clear();
    if (encoded.empty()) {
        return true;
    }
    std::vector<unsigned char> bytes = util::Base64Decode(encoded);
    if (bytes.empty()) {
        if (error) *error = "API 密钥密文格式不正确";
        return false;
    }
    DATA_BLOB input{};
    input.pbData = bytes.data();
    input.cbData = static_cast<DWORD>(bytes.size());
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        if (error) {
            *error = "API 密钥无法解密（可能由其他 Windows 用户创建），请重新填写";
        }
        return false;
    }
    plain.assign(reinterpret_cast<char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return true;
}

std::filesystem::path BackupPath(const std::filesystem::path& source) {
    std::wstring stamp = util::FormatLocalTime(L"%Y%m%d-%H%M%S");
    std::filesystem::path backup = source;
    backup += L".corrupt-" + stamp;
    return backup;
}

void BackupCorruptFile(const std::filesystem::path& source) {
    std::error_code ec;
    if (util::FileExists(source)) {
        std::filesystem::copy_file(source, BackupPath(source),
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }
}

std::string NormalizeBase(std::string value) {
    value = util::Trim(value);
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    if (value.empty()) {
        return "https://api.deepseek.com";
    }
    return value;
}

int CountTag(const std::vector<Question>& questions, const std::string& tag, bool formulasOnly,
             bool conceptsOnly) {
    int count = 0;
    for (const auto& question : questions) {
        bool found = false;
        if (!conceptsOnly) {
            for (const auto& item : question.formulas) {
                if (TagsEqual(item, tag)) {
                    found = true;
                    break;
                }
            }
        }
        if (!found && !formulasOnly) {
            for (const auto& item : question.concepts) {
                if (TagsEqual(item, tag)) {
                    found = true;
                    break;
                }
            }
        }
        if (found) ++count;
    }
    return count;
}

}  // namespace

Store::Store(std::filesystem::path appRoot) : appRoot_(std::move(appRoot)) {
    dataDir_ = appRoot_ / L"data";
}

bool Store::Initialize(std::string* warning) {
    if (warning) warning->clear();
    std::string error;
    if (!EnsureDataDirectory(&error)) {
        if (warning) *warning = error;
        return false;
    }
    LoadQuestions(warning);
    LoadSettings(warning);
    LoadTags(warning);
    MergeTagsFromQuestions();
    std::string tagError;
    if (!SaveTags(&tagError)) {
        AppendWarning(warning, tagError);
    }
    return true;
}

bool Store::EnsureDataDirectory(std::string* error) {
    std::error_code ec;
    std::filesystem::create_directories(dataDir_, ec);
    if (ec) {
        if (error) {
            *error = "无法创建数据目录：" + util::ToUtf8(dataDir_.wstring());
        }
        return false;
    }
    return true;
}

bool Store::LoadQuestions(std::string* warning) {
    const std::filesystem::path path = QuestionsPath();
    if (!util::FileExists(path)) {
        questions_ = BuildSeedQuestions();
        std::string saveError;
        if (!SaveQuestions(&saveError)) AppendWarning(warning, saveError);
        return true;
    }
    std::string text;
    std::string error;
    if (!util::ReadFileBytes(path, text, &error)) {
        AppendWarning(warning, error + "，已载入示例题库。");
        questions_ = BuildSeedQuestions();
        return true;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(text, &parseError);
    const jsonx::Value* list = nullptr;
    if (root.IsObject()) {
        list = root.Find("questions");
    }
    if (!list || !list->IsArray()) {
        BackupCorruptFile(path);
        AppendWarning(warning, "题库文件格式无效，已备份原文件并载入示例题库。" +
                                   (parseError.empty() ? "" : " " + parseError));
        questions_ = BuildSeedQuestions();
        std::string saveError;
        if (!SaveQuestions(&saveError)) AppendWarning(warning, saveError);
        return true;
    }
    std::vector<Question> loaded;
    std::set<std::string> ids;
    loaded.reserve(list->size());
    for (const auto& item : list->Items()) {
        Question question = Question::FromJson(item);
        if (util::Trim(question.stem).empty()) {
            continue;
        }
        while (question.id.empty() || ids.count(question.id)) {
            question.id = util::NewId("q");
        }
        ids.insert(question.id);
        loaded.push_back(std::move(question));
    }
    if (loaded.empty()) {
        BackupCorruptFile(path);
        AppendWarning(warning, "题库为空或没有有效题目，已载入示例题库。");
        questions_ = BuildSeedQuestions();
        std::string saveError;
        if (!SaveQuestions(&saveError)) AppendWarning(warning, saveError);
        return true;
    }
    questions_ = std::move(loaded);
    return true;
}

bool Store::LoadTags(std::string* warning) {
    const std::filesystem::path path = TagsPath();
    if (!util::FileExists(path)) {
        tags_.Rebuild(questions_);
        std::string saveError;
        if (!SaveTags(&saveError)) AppendWarning(warning, saveError);
        return true;
    }
    std::string text;
    std::string error;
    if (!util::ReadFileBytes(path, text, &error)) {
        AppendWarning(warning, error);
        tags_.Rebuild(questions_);
        return true;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(text, &parseError);
    if (!root.IsObject()) {
        BackupCorruptFile(path);
        AppendWarning(warning, "标签库文件格式无效，已根据题库重建。");
        tags_.Rebuild(questions_);
        SaveTags();
        return true;
    }
    tags_ = TagLibrary::FromJson(root);
    return true;
}

bool Store::LoadSettings(std::string* warning) {
    const std::filesystem::path path = SettingsPath();
    settings_ = Settings{};
    if (!util::FileExists(path)) {
        std::string saveError;
        if (!SaveSettings(&saveError)) AppendWarning(warning, saveError);
        return true;
    }
    std::string text;
    std::string error;
    if (!util::ReadFileBytes(path, text, &error)) {
        AppendWarning(warning, error + "，已使用默认 AI 设置。");
        return true;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(text, &parseError);
    if (!root.IsObject()) {
        AppendWarning(warning, "AI 配置文件格式无效，已使用默认设置。");
        return true;
    }
    settings_.apiBase = NormalizeBase(root["api_base"].AsString(settings_.apiBase));
    settings_.model = util::Trim(root["model"].AsString(settings_.model));
    if (settings_.model.empty()) settings_.model = "deepseek-chat";
    double temperature = root["temperature"].AsNumber(0.2);
    settings_.temperature = std::clamp(temperature, 0.0, 1.5);
    settings_.darkMode = root["dark_mode"].AsBool(false);
    std::string protectedKey = root["api_key_protected"].AsString();
    if (!protectedKey.empty()) {
        std::string keyError;
        if (!UnprotectSecret(protectedKey, settings_.apiKey, &keyError)) {
            AppendWarning(warning, keyError);
        }
    } else if (root["api_key"].IsString()) {
        settings_.apiKey = util::Trim(root["api_key"].AsString());
    }
    return true;
}

jsonx::Value Store::SerializeQuestions() const {
    jsonx::Value root = jsonx::Value::MakeObject();
    root.Set("version", 1);
    root.Set("updated", util::NowIso8601());
    jsonx::Value::Array array;
    array.reserve(questions_.size());
    for (const auto& question : questions_) {
        array.push_back(question.ToJson());
    }
    root.Set("questions", jsonx::Value(std::move(array)));
    return root;
}

bool Store::SerializeSettings(jsonx::Value& root, std::string* error) const {
    root = jsonx::Value::MakeObject();
    root.Set("version", 1);
    root.Set("api_base", settings_.apiBase);
    root.Set("model", settings_.model);
    root.Set("temperature", settings_.temperature);
    root.Set("dark_mode", settings_.darkMode);
    if (!settings_.apiKey.empty()) {
        std::string protectedKey;
        if (!ProtectSecret(settings_.apiKey, protectedKey, error)) {
            return false;
        }
        root.Set("api_key_protected", protectedKey);
        root.Set("key_protection", "windows-dpapi");
    }
    return true;
}

bool Store::SaveQuestions(std::string* error) {
    return util::WriteFileAtomic(QuestionsPath(), SerializeQuestions().Dump(2) + "\n", error);
}

bool Store::SaveTags(std::string* error) {
    return util::WriteFileAtomic(TagsPath(), tags_.ToJson().Dump(2) + "\n", error);
}

bool Store::SaveSettings(std::string* error) {
    jsonx::Value root;
    if (!SerializeSettings(root, error)) {
        return false;
    }
    return util::WriteFileAtomic(SettingsPath(), root.Dump(2) + "\n", error);
}

bool Store::SaveAll(std::string* error) {
    if (!SaveQuestions(error)) return false;
    if (!SaveTags(error)) return false;
    if (!SaveSettings(error)) return false;
    return true;
}

Question* Store::FindQuestion(const std::string& id) {
    auto it = std::find_if(questions_.begin(), questions_.end(),
                           [&](const Question& q) { return q.id == id; });
    return it == questions_.end() ? nullptr : &*it;
}

const Question* Store::FindQuestion(const std::string& id) const {
    auto it = std::find_if(questions_.begin(), questions_.end(),
                           [&](const Question& q) { return q.id == id; });
    return it == questions_.end() ? nullptr : &*it;
}

int Store::TagUsage(const std::string& tag) const {
    return CountTag(questions_, tag, false, false);
}

int Store::FormulaUsage(const std::string& tag) const {
    return CountTag(questions_, tag, true, false);
}

int Store::ConceptUsage(const std::string& tag) const {
    return CountTag(questions_, tag, false, true);
}

void Store::MergeTagsFromQuestions() {
    for (const auto& question : questions_) {
        tags_.AddFromQuestion(question);
    }
}

void Store::MergeQuestionTags(const Question& question) {
    tags_.AddFromQuestion(question);
}

std::filesystem::path Store::QuestionsPath() const {
    return dataDir_ / L"questions.json";
}

std::filesystem::path Store::TagsPath() const {
    return dataDir_ / L"tags.json";
}

std::filesystem::path Store::SettingsPath() const {
    return dataDir_ / L"settings.json";
}
