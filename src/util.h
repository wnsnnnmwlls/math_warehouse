#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace util {

std::string ToUtf8(const std::wstring& text);
std::wstring ToWide(const std::string& text);
std::string Trim(const std::string& text);
std::wstring Trim(const std::wstring& text);
std::string ToLowerAscii(std::string text);
std::wstring ToLower(std::wstring text);
bool ContainsIgnoreCase(const std::wstring& haystack, const std::wstring& needle);
std::vector<std::string> SplitTags(const std::string& text);
std::vector<std::string> SplitLines(const std::string& text);
std::string Join(const std::vector<std::string>& items, std::string_view separator);
bool IsValidUtf8(const std::string& text);

bool ReadFileBytes(const std::filesystem::path& path, std::string& out, std::string* error = nullptr);
bool WriteFileBytes(const std::filesystem::path& path, std::string_view data, std::string* error = nullptr);
bool WriteFileAtomic(const std::filesystem::path& path, std::string_view data, std::string* error = nullptr);
bool FileExists(const std::filesystem::path& path);

std::filesystem::path ExeDirectory();
std::filesystem::path DesktopDirectory();
std::filesystem::path PicturesDirectory();
std::wstring FormatLocalTime(const wchar_t* format);
std::string NowIso8601();
std::string NewId(const char* prefix);

std::string Base64Encode(const std::vector<unsigned char>& data);
std::vector<unsigned char> Base64Decode(const std::string& text);

int ClampInt(int value, int low, int high);

}  // namespace util
