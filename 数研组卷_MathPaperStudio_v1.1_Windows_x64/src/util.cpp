#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cwctype>
#include <iterator>
#include <random>
#include <sstream>

namespace util {
namespace {

std::wstring FormatWinError(DWORD code) {
    wchar_t* buffer = nullptr;
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring result;
    if (length && buffer) {
        result.assign(buffer, length);
        while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n' || result.back() == L' ')) {
            result.pop_back();
        }
    } else {
        result = L"Windows error " + std::to_wstring(code);
    }
    if (buffer) {
        LocalFree(buffer);
    }
    return result;
}

void SetError(std::string* error, const std::wstring& message) {
    if (error) {
        *error = ToUtf8(message);
    }
}

bool StartsWith(const std::string& text, size_t offset, std::string_view needle) {
    if (offset + needle.size() > text.size()) {
        return false;
    }
    return text.compare(offset, needle.size(), needle) == 0;
}

}  // namespace

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                   static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                   nullptr, 0);
        if (size <= 0) {
            return {};
        }
    }
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), size);
    return result;
}

std::string Trim(const std::string& text) {
    size_t begin = 0;
    size_t end = text.size();
    auto isAsciiSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
    };
    while (begin < end && isAsciiSpace(text[begin])) {
        ++begin;
    }
    while (end > begin && isAsciiSpace(text[end - 1])) {
        --end;
    }
    std::string result = text.substr(begin, end - begin);
    bool changed = true;
    while (changed && !result.empty()) {
        changed = false;
        if (result.rfind("\xE3\x80\x80", 0) == 0) {
            result.erase(0, 3);
            changed = true;
        }
        if (result.size() >= 3 && result.compare(result.size() - 3, 3, "\xE3\x80\x80") == 0) {
            result.erase(result.size() - 3);
            changed = true;
        }
        if (result.rfind("\xC2\xA0", 0) == 0) {
            result.erase(0, 2);
            changed = true;
        }
        if (result.size() >= 2 && result.compare(result.size() - 2, 2, "\xC2\xA0") == 0) {
            result.erase(result.size() - 2);
            changed = true;
        }
    }
    return result;
}

std::wstring Trim(const std::wstring& text) {
    size_t begin = 0;
    size_t end = text.size();
    auto isSpace = [](wchar_t c) {
        return iswspace(c) || c == 0x3000 || c == 0x00A0;
    };
    while (begin < end && isSpace(text[begin])) {
        ++begin;
    }
    while (end > begin && isSpace(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string ToLowerAscii(std::string text) {
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return text;
}

std::wstring ToLower(std::wstring text) {
    for (wchar_t& c : text) {
        c = static_cast<wchar_t>(towlower(c));
    }
    return text;
}

bool ContainsIgnoreCase(const std::wstring& haystack, const std::wstring& needle) {
    if (needle.empty()) {
        return true;
    }
    return ToLower(haystack).find(ToLower(needle)) != std::wstring::npos;
}

std::vector<std::string> SplitTags(const std::string& text) {
    std::vector<std::string> result;
    std::string current;
    auto flush = [&]() {
        std::string tag = Trim(current);
        if (!tag.empty()) {
            if (tag.front() == '#') {
                tag.erase(tag.begin());
                tag = Trim(tag);
            }
            bool duplicate = std::any_of(result.begin(), result.end(), [&](const std::string& item) {
                return ToLowerAscii(item) == ToLowerAscii(tag);
            });
            if (!tag.empty() && !duplicate) {
                result.push_back(tag);
            }
        }
        current.clear();
    };
    for (size_t i = 0; i < text.size();) {
        char c = text[i];
        if (c == ',' || c == ';' || c == '\n' || c == '\r') {
            flush();
            ++i;
            continue;
        }
        if (StartsWith(text, i, "\xEF\xBC\x8C") || StartsWith(text, i, "\xEF\xBC\x9B") ||
            StartsWith(text, i, "\xE3\x80\x81") || StartsWith(text, i, "\xE3\x80\x82")) {
            flush();
            i += 3;
            continue;
        }
        current.push_back(c);
        ++i;
    }
    flush();
    return result;
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            continue;
        }
        if (text[i] == '\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    lines.push_back(current);
    return lines;
}

std::string Join(const std::vector<std::string>& items, std::string_view separator) {
    std::string result;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) {
            result.append(separator);
        }
        result += items[i];
    }
    return result;
}

bool IsValidUtf8(const std::string& text) {
    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        int trailing = 0;
        uint32_t codepoint = 0;
        if ((c & 0xE0) == 0xC0) {
            trailing = 1;
            codepoint = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            trailing = 2;
            codepoint = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            trailing = 3;
            codepoint = c & 0x07;
        } else {
            return false;
        }
        if (i + static_cast<size_t>(trailing) >= text.size()) {
            return false;
        }
        for (int k = 1; k <= trailing; ++k) {
            unsigned char cc = static_cast<unsigned char>(text[i + k]);
            if ((cc & 0xC0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (cc & 0x3F);
        }
        if ((trailing == 1 && codepoint < 0x80) || (trailing == 2 && codepoint < 0x800) ||
            (trailing == 3 && codepoint < 0x10000)) {
            return false;
        }
        i += static_cast<size_t>(trailing) + 1;
    }
    return true;
}

bool ReadFileBytes(const std::filesystem::path& path, std::string& out, std::string* error) {
    out.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        SetError(error, L"无法读取文件：" + path.filename().wstring() + L"（" +
                            FormatWinError(GetLastError()) + L"）");
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > (64LL << 20)) {
        CloseHandle(file);
        if (error) *error = "File is too large or its size cannot be read.";
        return false;
    }
    out.resize(static_cast<size_t>(size.QuadPart));
    size_t offset = 0;
    while (offset < out.size()) {
        DWORD toRead = static_cast<DWORD>(std::min<size_t>(out.size() - offset, 1u << 20));
        DWORD read = 0;
        if (!ReadFile(file, out.data() + offset, toRead, &read, nullptr)) {
            CloseHandle(file);
            SetError(error, L"读取文件失败：" + FormatWinError(GetLastError()));
            out.clear();
            return false;
        }
        if (read == 0) {
            break;
        }
        offset += read;
    }
    CloseHandle(file);
    out.resize(offset);
    return true;
}

bool WriteFileBytes(const std::filesystem::path& path, std::string_view data, std::string* error) {
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        SetError(error, L"无法写入文件：" + path.filename().wstring() + L"（" +
                            FormatWinError(GetLastError()) + L"）");
        return false;
    }
    size_t offset = 0;
    while (offset < data.size()) {
        DWORD toWrite = static_cast<DWORD>(std::min<size_t>(data.size() - offset, 1u << 20));
        DWORD written = 0;
        if (!WriteFile(file, data.data() + offset, toWrite, &written, nullptr) || written == 0) {
            DWORD code = GetLastError();
            CloseHandle(file);
            SetError(error, L"写入文件失败：" + FormatWinError(code));
            return false;
        }
        offset += written;
    }
    CloseHandle(file);
    return true;
}

bool WriteFileAtomic(const std::filesystem::path& path, std::string_view data, std::string* error) {
    std::filesystem::path temp = path;
    temp += L".tmp";
    if (!WriteFileBytes(temp, data, error)) {
        return false;
    }
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD code = GetLastError();
        DeleteFileW(temp.c_str());
        SetError(error, L"保存文件失败：" + FormatWinError(code));
        return false;
    }
    return true;
}

bool FileExists(const std::filesystem::path& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::filesystem::path ExeDirectory() {
    std::wstring buffer(32768, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return std::filesystem::current_path();
    }
    buffer.resize(length);
    std::filesystem::path path(buffer);
    return path.parent_path();
}

std::filesystem::path DesktopDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, buffer))) {
        return std::filesystem::path(buffer);
    }
    wchar_t profile[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(profile) / L"Desktop";
    }
    return ExeDirectory();
}

std::filesystem::path PicturesDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0, buffer))) {
        return std::filesystem::path(buffer);
    }
    return DesktopDirectory();
}

std::wstring FormatLocalTime(const wchar_t* format) {
    SYSTEMTIME local{};
    GetLocalTime(&local);
    tm localTm{};
    localTm.tm_year = local.wYear - 1900;
    localTm.tm_mon = local.wMonth - 1;
    localTm.tm_mday = local.wDay;
    localTm.tm_hour = local.wHour;
    localTm.tm_min = local.wMinute;
    localTm.tm_sec = local.wSecond;
    localTm.tm_wday = local.wDayOfWeek;
    wchar_t buffer[128] = {};
    if (wcsftime(buffer, 128, format, &localTm) == 0) {
        return L"";
    }
    return buffer;
}

std::string NowIso8601() {
    std::time_t now = std::time(nullptr);
    tm utc{};
    gmtime_s(&utc, &now);
    char buffer[64] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

std::string NewId(const char* prefix) {
    static std::atomic<uint64_t> counter{0};
    uint64_t stamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    uint64_t tick = stamp ^ (static_cast<uint64_t>(GetTickCount64()) << 13) ^
                    (counter.fetch_add(1) * 0x9E3779B97F4A7C15ULL);
    std::ostringstream out;
    out << (prefix ? prefix : "q") << "_" << std::hex << (tick & 0xFFFFFFFFFFFFULL);
    return out.str();
}

std::string Base64Encode(const std::vector<unsigned char>& data) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t value = (static_cast<uint32_t>(data[i]) << 16) |
                         (static_cast<uint32_t>(data[i + 1]) << 8) |
                         static_cast<uint32_t>(data[i + 2]);
        result.push_back(alphabet[(value >> 18) & 0x3F]);
        result.push_back(alphabet[(value >> 12) & 0x3F]);
        result.push_back(alphabet[(value >> 6) & 0x3F]);
        result.push_back(alphabet[value & 0x3F]);
        i += 3;
    }
    if (i + 1 == data.size()) {
        uint32_t value = static_cast<uint32_t>(data[i]) << 16;
        result.push_back(alphabet[(value >> 18) & 0x3F]);
        result.push_back(alphabet[(value >> 12) & 0x3F]);
        result += "==";
    } else if (i + 2 == data.size()) {
        uint32_t value = (static_cast<uint32_t>(data[i]) << 16) |
                         (static_cast<uint32_t>(data[i + 1]) << 8);
        result.push_back(alphabet[(value >> 18) & 0x3F]);
        result.push_back(alphabet[(value >> 12) & 0x3F]);
        result.push_back(alphabet[(value >> 6) & 0x3F]);
        result.push_back('=');
    }
    return result;
}

std::vector<unsigned char> Base64Decode(const std::string& text) {
    std::vector<unsigned char> output;
    int decoding[256];
    std::fill(std::begin(decoding), std::end(decoding), -1);
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (int i = 0; i < 64; ++i) {
        decoding[static_cast<unsigned char>(alphabet[i])] = i;
    }
    uint32_t value = 0;
    int bits = 0;
    for (unsigned char c : text) {
        if (c == '=') {
            break;
        }
        int digit = decoding[c];
        if (digit < 0) {
            continue;
        }
        value = (value << 6) | static_cast<uint32_t>(digit);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output.push_back(static_cast<unsigned char>((value >> bits) & 0xFF));
        }
    }
    return output;
}

int ClampInt(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

}  // namespace util
