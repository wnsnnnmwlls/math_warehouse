#include "ai.h"

#include "math_text.h"
#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

constexpr int kTaggingTimeoutMs = 90000;
constexpr int kImageTimeoutMs = 150000;
constexpr int kTestTimeoutMs = 60000;

class WinHttpHandle {
public:
    explicit WinHttpHandle(HINTERNET handle = nullptr) : handle_(handle) {}
    ~WinHttpHandle() {
        if (handle_) WinHttpCloseHandle(handle_);
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
        if (this != &other) {
            if (handle_) WinHttpCloseHandle(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    HINTERNET get() const { return handle_; }
    explicit operator bool() const { return handle_ != nullptr; }

private:
    HINTERNET handle_ = nullptr;
};

struct HttpResponse {
    bool transportOk = false;
    DWORD status = 0;
    std::string body;
    std::string error;
};

std::string WinErrorText(DWORD code) {
    wchar_t* buffer = nullptr;
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring text;
    if (length && buffer) {
        text.assign(buffer, length);
        while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ')) {
            text.pop_back();
        }
    }
    if (buffer) LocalFree(buffer);
    if (text.empty()) {
        text = L"错误码 " + std::to_wstring(code);
    }
    return util::ToUtf8(text);
}

std::wstring ChatEndpoint(const std::string& apiBase) {
    std::string base = util::Trim(apiBase);
    if (base.empty()) {
        base = "https://api.deepseek.com";
    }
    if (base.rfind("http://", 0) != 0 && base.rfind("https://", 0) != 0) {
        std::string lower = util::ToLowerAscii(base);
        bool local = lower.find("localhost") != std::string::npos ||
                     lower.find("127.0.0.1") != std::string::npos ||
                     lower.find("0.0.0.0") != std::string::npos;
        base = (local ? "http://" : "https://") + base;
    }
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    if (base.find("/chat/completions") != std::string::npos) {
        return util::ToWide(base);
    }
    base += "/chat/completions";
    return util::ToWide(base);
}

HttpResponse HttpPostJson(const std::string& apiBase, const std::string& apiKey,
                          const std::string& body, int timeoutMs) {
    HttpResponse response;
    std::wstring url = ChatEndpoint(apiBase);
    if (url.empty()) {
        response.error = "API 地址为空。";
        return response;
    }

    std::wstring mutableUrl = url;
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(mutableUrl.c_str(), static_cast<DWORD>(mutableUrl.size()), 0, &components)) {
        response.error = "API 地址格式不正确，请填写以 http:// 或 https:// 开头的完整地址。";
        return response;
    }
    if (!components.lpszHostName || components.dwHostNameLength == 0) {
        response.error = "API 地址缺少主机名。";
        return response;
    }
    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path =
        components.lpszUrlPath ? std::wstring(components.lpszUrlPath, components.dwUrlPathLength)
                               : std::wstring(L"/");
    if (components.lpszExtraInfo && components.dwExtraInfoLength > 0) {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }
    if (path.empty()) path = L"/";
    bool secure = components.nScheme == INTERNET_SCHEME_HTTPS;

    WinHttpHandle session(WinHttpOpen(L"MathPaperStudio/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        response.error = "初始化网络组件失败：" + WinErrorText(GetLastError());
        return response;
    }
    WinHttpSetTimeouts(session.get(), 12000, 15000, 30000, timeoutMs);

    WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
    if (!connection) {
        response.error = "无法连接 AI 服务：" + WinErrorText(GetLastError());
        return response;
    }
    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"POST", path.c_str(), nullptr,
                                             WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request) {
        response.error = "创建网络请求失败：" + WinErrorText(GetLastError());
        return response;
    }

    std::wstring headers = L"Content-Type: application/json; charset=utf-8\r\n";
    headers += L"Accept: application/json\r\n";
    if (!util::Trim(apiKey).empty()) {
        headers += L"Authorization: Bearer " + util::ToWide(util::Trim(apiKey)) + L"\r\n";
    }
    if (body.size() > 32u * 1024u * 1024u) {
        response.error = "请求内容过大。";
        return response;
    }
    BOOL sent = WinHttpSendRequest(
        request.get(), headers.c_str(), static_cast<DWORD>(-1),
        const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0);
    if (!sent) {
        DWORD code = GetLastError();
        response.error = "发送 AI 请求失败：" + WinErrorText(code);
        return response;
    }
    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        DWORD code = GetLastError();
        response.error = "接收 AI 响应失败：" + WinErrorText(code);
        return response;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                             WINHTTP_NO_HEADER_INDEX)) {
        response.error = "读取 HTTP 状态码失败：" + WinErrorText(GetLastError());
        return response;
    }
    response.status = status;

    std::string bodyData;
    char buffer[8192];
    while (true) {
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), buffer, sizeof(buffer), &read)) {
            DWORD code = GetLastError();
            response.error = "读取 AI 响应失败：" + WinErrorText(code);
            return response;
        }
        if (read == 0) break;
        if (bodyData.size() + read > 8u * 1024u * 1024u) {
            response.error = "AI 响应内容过大。";
            return response;
        }
        bodyData.append(buffer, read);
    }
    response.body = std::move(bodyData);
    response.transportOk = true;
    return response;
}

std::string ClampPreview(std::string text, size_t maxBytes) {
    text = util::Trim(text);
    if (text.size() <= maxBytes) return text;
    size_t cut = maxBytes;
    while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80) {
        --cut;
    }
    text.resize(cut);
    return text + "…";
}

std::string ApiErrorMessage(const jsonx::Value& root, DWORD status, const std::string& rawBody) {
    const jsonx::Value& error = root["error"];
    if (error.IsObject()) {
        std::string message = util::Trim(error["message"].AsString());
        if (!message.empty()) return message;
    } else if (error.IsString()) {
        return error.AsString();
    }
    std::string message = util::Trim(root["message"].AsString());
    if (!message.empty() && !root["choices"].IsArray()) return message;
    std::string body = ClampPreview(rawBody, 400);
    std::string result = "HTTP " + std::to_string(status);
    if (!body.empty()) result += "：" + body;
    return result;
}

std::string ExtractMessageContent(const jsonx::Value& root) {
    const jsonx::Value& choices = root["choices"];
    if (choices.IsArray() && choices.size() > 0) {
        const jsonx::Value& message = choices[0]["message"];
        const jsonx::Value& content = message["content"];
        if (content.IsString()) {
            return content.AsString();
        }
        if (content.IsArray()) {
            std::string joined;
            for (const auto& part : content.Items()) {
                if (part.IsString()) {
                    joined += part.AsString();
                } else if (part["text"].IsString()) {
                    joined += part["text"].AsString();
                }
            }
            return joined;
        }
    }
    if (root["output_text"].IsString()) {
        return root["output_text"].AsString();
    }
    return {};
}

bool StartsAt(const std::string& text, size_t position, std::string_view needle) {
    return position + needle.size() <= text.size() &&
           text.compare(position, needle.size(), needle) == 0;
}

std::string CleanTag(std::string tag) {
    tag = util::Trim(tag);
    while (!tag.empty()) {
        unsigned char c = static_cast<unsigned char>(tag.front());
        if (tag.front() == '-' || tag.front() == '*' || tag.front() == '#' || tag.front() == '"' ||
            tag.front() == '`' || tag.front() == ' ') {
            tag.erase(tag.begin());
            continue;
        }
        if (std::isdigit(c)) {
            size_t i = 0;
            while (i < tag.size() && std::isdigit(static_cast<unsigned char>(tag[i]))) ++i;
            size_t delimiterLength = 0;
            if (i < tag.size() && (tag[i] == '.' || tag[i] == ')')) {
                delimiterLength = 1;
            } else if (StartsAt(tag, i, "\xEF\xBC\x8E") || StartsAt(tag, i, "\xEF\xBC\x89") ||
                       StartsAt(tag, i, "\xE3\x80\x81")) {
                delimiterLength = 3;
            }
            if (delimiterLength > 0) {
                size_t skip = i + delimiterLength;
                if (skip < tag.size() && tag[skip] == ' ') ++skip;
                tag.erase(0, skip);
                tag = util::Trim(tag);
                continue;
            }
        }
        break;
    }
    while (!tag.empty()) {
        if (tag.back() == '"' || tag.back() == '`') {
            tag.pop_back();
            continue;
        }
        if (tag.size() >= 3 && tag.compare(tag.size() - 3, 3, "\xE3\x80\x82") == 0) {
            tag.erase(tag.size() - 3);
            continue;
        }
        break;
    }
    if (tag == "无" || tag == "暂无" || tag == "None" || tag == "N/A" || tag == "null") {
        return {};
    }
    if (tag.size() > 200) {
        return {};
    }
    return tag;
}

void ReadTagList(const jsonx::Value& root, const std::vector<std::string>& keys,
                 std::vector<std::string>& output) {
    for (const auto& key : keys) {
        const jsonx::Value& node = root[key];
        if (node.IsString()) {
            for (const auto& tag : util::SplitTags(node.AsString())) {
                std::string cleaned = CleanTag(tag);
                if (!cleaned.empty()) output.push_back(std::move(cleaned));
            }
        } else if (node.IsArray()) {
            for (const auto& item : node.Items()) {
                std::string text;
                if (item.IsString()) {
                    text = item.AsString();
                } else if (item.IsObject()) {
                    if (item["name"].IsString()) text = item["name"].AsString();
                    else if (item["tag"].IsString()) text = item["tag"].AsString();
                    else if (item["label"].IsString()) text = item["label"].AsString();
                }
                std::string cleaned = CleanTag(text);
                if (!cleaned.empty()) output.push_back(std::move(cleaned));
            }
        }
        if (!output.empty()) break;
    }
}

std::string MakeChatPayload(const Settings& settings, const std::string& systemPrompt,
                            const std::string& userPrompt, int maxTokens) {
    jsonx::Value payload = jsonx::Value::MakeObject();
    payload.Set("model", settings.model);
    jsonx::Value::Array messages;
    jsonx::Value systemMessage = jsonx::Value::MakeObject();
    systemMessage.Set("role", "system");
    systemMessage.Set("content", systemPrompt);
    messages.push_back(std::move(systemMessage));
    jsonx::Value userMessage = jsonx::Value::MakeObject();
    userMessage.Set("role", "user");
    userMessage.Set("content", userPrompt);
    messages.push_back(std::move(userMessage));
    payload.Set("messages", jsonx::Value(std::move(messages)));
    payload.Set("temperature", std::clamp(settings.temperature, 0.0, 1.5));
    payload.Set("max_tokens", maxTokens);
    payload.Set("stream", false);
    return payload.DumpCompact();
}

std::string MakeVisionChatPayload(const Settings& settings, const std::string& systemPrompt,
                                  const std::string& userPrompt, const std::string& imageDataUrl,
                                  int maxTokens) {
    jsonx::Value payload = jsonx::Value::MakeObject();
    payload.Set("model", settings.model);

    jsonx::Value::Array messages;
    jsonx::Value systemMessage = jsonx::Value::MakeObject();
    systemMessage.Set("role", "system");
    systemMessage.Set("content", systemPrompt);
    messages.push_back(std::move(systemMessage));

    jsonx::Value textPart = jsonx::Value::MakeObject();
    textPart.Set("type", "text");
    textPart.Set("text", userPrompt);
    jsonx::Value imageUrl = jsonx::Value::MakeObject();
    imageUrl.Set("url", imageDataUrl);
    imageUrl.Set("detail", "high");
    jsonx::Value imagePart = jsonx::Value::MakeObject();
    imagePart.Set("type", "image_url");
    imagePart.Set("image_url", std::move(imageUrl));
    jsonx::Value::Array userContent;
    userContent.push_back(std::move(textPart));
    userContent.push_back(std::move(imagePart));

    jsonx::Value userMessage = jsonx::Value::MakeObject();
    userMessage.Set("role", "user");
    userMessage.Set("content", jsonx::Value(std::move(userContent)));
    messages.push_back(std::move(userMessage));

    payload.Set("messages", jsonx::Value(std::move(messages)));
    payload.Set("temperature", std::min(0.2, std::clamp(settings.temperature, 0.0, 1.5)));
    payload.Set("max_tokens", maxTokens);
    payload.Set("stream", false);
    return payload.DumpCompact();
}

bool ExtractJsonObjectFromContent(const std::string& content, jsonx::Value& out,
                                  std::string& error) {
    std::string jsonText = util::Trim(content);
    if (jsonText.rfind("```", 0) == 0) {
        size_t newline = jsonText.find('\n');
        if (newline != std::string::npos) jsonText.erase(0, newline + 1);
        size_t fence = jsonText.rfind("```");
        if (fence != std::string::npos) jsonText.erase(fence);
    }
    size_t first = jsonText.find('{');
    size_t last = jsonText.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last <= first) {
        error = "AI 返回内容中没有 JSON 对象，请重试或更换模型。";
        return false;
    }
    std::string parseError;
    out = jsonx::Value::Parse(jsonText.substr(first, last - first + 1), &parseError);
    if (!out.IsObject()) {
        error = "AI 返回的 JSON 无法解析。";
        return false;
    }
    return true;
}

std::string CleanOptionText(std::string text) {
    text = util::Trim(text);
    if (text.empty()) return {};
    if (text.front() == '(' && text.size() >= 3 && text[1] >= 'A' && text[1] <= 'F' &&
        text[2] == ')') {
        text = util::Trim(text.substr(3));
    } else if (text.size() >= 2 && text[0] >= 'A' && text[0] <= 'F') {
        size_t skip = 0;
        if (text[1] == '.' || text[1] == ')' || text[1] == ' ' || text[1] == ':') {
            skip = 2;
        } else if (StartsAt(text, 1, "\xEF\xBC\x8E") || StartsAt(text, 1, "\xEF\xBC\x89") ||
                   StartsAt(text, 1, "\xE3\x80\x81")) {
            skip = 4;
        }
        if (skip > 0) text = util::Trim(text.substr(skip));
    }
    return text;
}

void DedupeTags(std::vector<std::string>& tags, size_t limit) {
    std::vector<std::string> unique;
    for (auto& tag : tags) {
        if (tag.empty()) continue;
        bool exists = std::any_of(unique.begin(), unique.end(), [&](const std::string& old) {
            return TagsEqual(old, tag);
        });
        if (!exists) unique.push_back(std::move(tag));
        if (unique.size() >= limit) break;
    }
    tags = std::move(unique);
}

std::string BuildImagePrompt(const TagLibrary& library) {
    std::ostringstream prompt;
    prompt << "你是一名资深高中数学教师。请识别图片中所有完整的数学题，完整转录题目，"
              "为每一道题独立求解，提取难度和考查标签。\n\n";
    prompt << "只输出一个合法的 JSON 对象，不要输出 Markdown 代码块、解释或其他文字。格式：\n";
    prompt << "{\n"
              "  \"questions\": [\n"
              "    {\n"
              "      \"stem\": \"题干文字，保留必要的换行\",\n"
              "      \"options\": [\"选项A内容\", \"选项B内容\"],\n"
              "      \"type\": \"choice\",\n"
              "      \"difficulty\": \"B\",\n"
              "      \"grade\": \"高一\",\n"
              "      \"answer\": \"最终答案；多小问分别写 (1) (2)\",\n"
              "      \"analysis\": \"关键推导步骤，不能为空\",\n"
              "      \"formulas\": [\"公式名\"],\n"
              "      \"concepts\": [\"知识点\"]\n"
              "    }\n"
              "  ]\n"
              "}\n\n";
    prompt << "规则：\n";
    prompt << "1. 识别图片中所有完整题目，按从上到下、从左到右排序；不要只提取第一道题。"
              "如果同一道题包含 (1)(2) 两个小问，请合并为同一道题，在 stem 中保留小问，"
              "并在 answer / analysis 中分别回答。\n";
    prompt << "2. 每道题都必须独立求解。即使图片中只有题干、没有答案和解析，也必须自己解题并在 "
              "answer 和 analysis 中写出答案与解析；answer 和 analysis 都不允许为空字符串。"
              "如果图片中已有答案，请核对后修正或补充。\n";
    prompt << "3. 题干不要包含“A．/B．”等选项字母；options 只写选项内容，没有选项时返回 []。\n";
    prompt << "4. type 只能是 choice（有选项的选择题）、fill（填空题）或 solution（解答题）。"
              "若无法判断，带选项时用 choice，否则用 solution。\n";
    prompt << "5. difficulty 只能是 A（基础）、B（提升）、C（挑战），不确定时填 B。\n";
    prompt << "6. grade 只能是 高一、高二、高三 之一，按题目主要考查内容归级；不确定时填 高一。\n";
    prompt << "7. formulas 写具体公式、定理或运算名称，最多 4 个；concepts 写知识概念，最多 5 个。"
              "每个标签不超过 16 个汉字；优先复用已有标签。\n";
    prompt << "8. 所有数学表达必须使用人类可读的 Unicode 纯文本，禁止输出 LaTeX 或 Markdown 公式："
              "不要出现 $、\\(…\\)、\\frac、\\sqrt、\\left、\\alpha、^、_ 等命令。分数写成"
              "“分子/分母”（复杂分子或分母加括号），程序会把它排版成上下分式；不要用 ÷ 或 ／。"
              "根号写成“√”，希腊字母直接写 α、β、π，上下标使用 ²、ₙ 等 Unicode 字符。\n";
    prompt << "   例如：\\cos\\left(\\alpha+\\frac{\\pi}{4}\\right)=\\frac{\\sqrt{2}}{2} "
              "必须写成 cos(α+π/4)=√2/2；\n";
    prompt << "   \\frac{\\cos 2\\alpha}{\\sin\\alpha+\\cos\\alpha} 必须写成 "
              "(cos 2α)/(sin α+cos α)。\n";
    prompt << "9. 如果图片完全无法辨认，返回 {\"error\":\"无法识别图片中的题目\"}。如果只能识别"
              "部分题目，不要编造缺失题目，只返回能够完整识别并求解的题目。\n\n";
    if (!library.formulas.empty() || !library.concepts.empty()) {
        prompt << "已有公式标签：" << util::Join(library.formulas, "、") << "\n";
        prompt << "已有知识点标签：" << util::Join(library.concepts, "、") << "\n\n";
    }
    prompt << "请直接输出 JSON。";
    return prompt.str();
}

}  // namespace

std::string BuildAiPrompt(const Question& question, const TagLibrary& library) {
    std::ostringstream prompt;
    prompt << "你是一名资深高中数学教研员。请分析下面这道高中数学题，提取它考查的公式和知识点。\n\n";
    prompt << "输出要求：\n";
    prompt << "1. 只输出一个合法的 JSON 对象，不要输出 Markdown 代码块、解释或其他任何文字。\n";
    prompt << "2. JSON 格式必须为：{\"grade\":\"高一\",\"formulas\":[\"公式1\",\"公式2\"],"
              "\"concepts\":[\"知识点1\",\"知识点2\"]}\n";
    prompt << "3. grade 只能是 高一、高二、高三 之一，按题目主要考查内容归级；无法确定时填 高一。\n";
    prompt << "4. formulas 只写具体的公式、定理或运算名称（如“余弦定理”“等差数列通项公式”），最多 4 个；"
              "concepts 写知识概念（如“解三角形”“函数的单调性”），最多 5 个。\n";
    prompt << "5. 每个标签应是简短、规范的名词，长度不超过 16 个汉字；优先复用下面的已有标签。\n";
    prompt << "6. 如果某类确实没有合适标签，返回空数组 []。不要编造题目没有涉及的内容。\n\n";
    if (!library.formulas.empty() || !library.concepts.empty()) {
        prompt << "已有公式标签：" << util::Join(library.formulas, "、") << "\n";
        prompt << "已有知识点标签：" << util::Join(library.concepts, "、") << "\n\n";
    }
    prompt << "题目类型：" << util::ToUtf8(TypeName(question.type)) << "\n";
    prompt << "题干：\n" << ClampPreview(question.stem, 6000) << "\n";
    if (!question.options.empty()) {
        prompt << "选项：\n";
        static const wchar_t* labels[] = {L"A", L"B", L"C", L"D", L"E", L"F"};
        for (size_t i = 0; i < question.options.size() && i < 6; ++i) {
            prompt << util::ToUtf8(labels[i]) << "．" << question.options[i] << "\n";
        }
    }
    if (!question.answer.empty()) {
        prompt << "参考答案：" << ClampPreview(question.answer, 800) << "\n";
    }
    prompt << "\n请直接输出 JSON。";
    return prompt.str();
}

AiTagResult AnalyzeQuestionWithAi(const Question& question, const Settings& settings,
                                  const TagLibrary& library) {
    AiTagResult result;
    if (util::Trim(settings.model).empty()) {
        result.error = "尚未填写模型名称，请先在“AI 接口”中配置。";
        return result;
    }
    std::string systemPrompt =
        "你是高中数学教研员。你的任务是为题目打上公式标签和知识点标签。"
        "严格按用户要求只返回 JSON，不要解释。";
    std::string userPrompt = BuildAiPrompt(question, library);
    std::string payload = MakeChatPayload(settings, systemPrompt, userPrompt, 1000);
    HttpResponse response = HttpPostJson(settings.apiBase, settings.apiKey, payload, kTaggingTimeoutMs);
    if (!response.transportOk) {
        result.error = response.error;
        return result;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(response.body, &parseError);
    if (!root.IsObject()) {
        result.error = "AI 返回内容不是有效 JSON（HTTP " + std::to_string(response.status) + "）。";
        return result;
    }
    if (response.status < 200 || response.status >= 300) {
        result.error = "AI 接口返回错误（HTTP " + std::to_string(response.status) + "）：" +
                       ApiErrorMessage(root, response.status, response.body);
        return result;
    }
    std::string content = ExtractMessageContent(root);
    if (util::Trim(content).empty()) {
        result.error = "AI 响应中没有文本内容。";
        return result;
    }
    result.rawContent = content;

    std::string jsonText = util::Trim(content);
    if (jsonText.rfind("```", 0) == 0) {
        size_t newline = jsonText.find('\n');
        if (newline != std::string::npos) jsonText.erase(0, newline + 1);
        size_t fence = jsonText.rfind("```");
        if (fence != std::string::npos) jsonText.erase(fence);
    }
    size_t first = jsonText.find('{');
    size_t last = jsonText.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last <= first) {
        result.error = "AI 返回的内容中没有 JSON 对象，请重试或更换模型。";
        return result;
    }
    std::string tagParseError;
    jsonx::Value tags = jsonx::Value::Parse(jsonText.substr(first, last - first + 1), &tagParseError);
    if (!tags.IsObject()) {
        result.error = "AI 返回的标签 JSON 无法解析。";
        return result;
    }

    ParseGrade(tags["grade"].AsString(), result.grade);
    ReadTagList(tags, {"formulas", "formula", "公式"}, result.formulas);
    ReadTagList(tags, {"concepts", "concept", "knowledge_points", "knowledgePoints", "知识点"},
                result.concepts);

    auto dedupe = [](std::vector<std::string>& items, size_t limit) {
        std::vector<std::string> unique;
        for (auto& item : items) {
            bool exists = std::any_of(unique.begin(), unique.end(), [&](const std::string& old) {
                return TagsEqual(old, item);
            });
            if (!exists) unique.push_back(std::move(item));
            if (unique.size() >= limit) break;
        }
        items = std::move(unique);
    };
    dedupe(result.formulas, 8);
    dedupe(result.concepts, 8);

    if (result.formulas.empty() && result.concepts.empty()) {
        result.error = "AI 没有提取到有效标签，请检查题目内容或换用更强的模型。";
        return result;
    }
    result.ok = true;
    return result;
}

AiImageQuestionResult AnalyzeQuestionImageWithAi(const std::string& imageDataUrl,
                                                 const Settings& settings,
                                                 const TagLibrary& library) {
    AiImageQuestionResult result;
    if (util::Trim(settings.model).empty()) {
        result.error = "尚未填写模型名称，请先在“AI 接口”中配置。";
        return result;
    }
    if (util::Trim(imageDataUrl).empty()) {
        result.error = "图片内容为空。";
        return result;
    }
    std::string systemPrompt =
        "你是一名资深高中数学教师，擅长从图片中准确识别数学题、独立解题并结构化输出。"
        "严格按用户要求只返回 JSON，不要解释。";
    std::string userPrompt = BuildImagePrompt(library);
    std::string payload =
        MakeVisionChatPayload(settings, systemPrompt, userPrompt, imageDataUrl, 6000);
    HttpResponse response =
        HttpPostJson(settings.apiBase, settings.apiKey, payload, kImageTimeoutMs);
    if (!response.transportOk) {
        result.error = response.error;
        return result;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(response.body, &parseError);
    if (!root.IsObject()) {
        result.error = "AI 返回内容不是有效 JSON（HTTP " + std::to_string(response.status) + "）。";
        return result;
    }
    if (response.status < 200 || response.status >= 300) {
        result.error = "AI 接口返回错误（HTTP " + std::to_string(response.status) + "）：" +
                       ApiErrorMessage(root, response.status, response.body);
        return result;
    }
    const jsonx::Value& choices = root["choices"];
    if (choices.IsArray() && choices.size() > 0 &&
        choices[0]["finish_reason"].AsString() == "length") {
        result.error = "AI 输出达到长度上限，可能没有提取完整。请减少一次上传的图片数量，"
                       "或换用支持更长输出的模型。";
        return result;
    }
    std::string content = ExtractMessageContent(root);
    if (util::Trim(content).empty()) {
        result.error = "AI 响应中没有文本内容。请确认所选模型支持图片输入。";
        return result;
    }
    result.rawContent = content;

    jsonx::Value fields;
    std::string contentError;
    if (!ExtractJsonObjectFromContent(content, fields, contentError)) {
        result.error = contentError;
        return result;
    }
    std::string topError = util::Trim(fields["error"].AsString());
    if (!topError.empty()) {
        result.error = topError;
        return result;
    }

    auto parseQuestionNode = [](const jsonx::Value& node, Question& question) -> bool {
        if (!node.IsObject()) return false;
        const jsonx::Value* payloadNode = &node;
        if (!node["stem"].IsString() && node["question"].IsObject()) {
            payloadNode = &node["question"];
        }
        std::string stem = util::Trim((*payloadNode)["stem"].AsString());
        if (stem.empty()) return false;
        question = Question{};
        question.stem = NormalizeMathText(stem);

        const jsonx::Value& options = (*payloadNode)["options"];
        if (options.IsArray()) {
            for (const auto& item : options.Items()) {
                std::string option;
                if (item.IsString()) {
                    option = item.AsString();
                } else if (item.IsObject()) {
                    if (item["text"].IsString()) option = item["text"].AsString();
                    else if (item["content"].IsString()) option = item["content"].AsString();
                    else if (item["value"].IsString()) option = item["value"].AsString();
                }
                option = NormalizeMathText(CleanOptionText(option));
                if (!option.empty()) question.options.push_back(std::move(option));
            }
        } else if (options.IsString()) {
            for (const auto& line : util::SplitLines(options.AsString())) {
                std::string option = NormalizeMathText(CleanOptionText(line));
                if (!option.empty()) question.options.push_back(std::move(option));
            }
        }

        bool parsedType = ParseQuestionType((*payloadNode)["type"].AsString(), question.type);
        if (!parsedType) {
            question.type =
                question.options.size() >= 2 ? QuestionType::Choice : QuestionType::Fill;
        }
        ParseDifficulty((*payloadNode)["difficulty"].AsString("B"), question.difficulty);
        question.answer = NormalizeMathText(util::Trim((*payloadNode)["answer"].AsString()));
        question.analysis = NormalizeMathText(util::Trim((*payloadNode)["analysis"].AsString()));
        Grade parsedGrade = Grade::Unknown;
        if (ParseGrade((*payloadNode)["grade"].AsString(), parsedGrade)) {
            question.grade = parsedGrade;
        } else {
            question.grade = InferGrade(question);
        }
        ReadTagList(*payloadNode, {"formulas", "formula", "公式"}, question.formulas);
        ReadTagList(*payloadNode,
                    {"concepts", "concept", "knowledge_points", "knowledgePoints", "知识点"},
                    question.concepts);
        DedupeTags(question.formulas, 8);
        DedupeTags(question.concepts, 8);
        question.tagged = question.HasAnyTag();
        question.id = util::NewId("q");
        question.created = util::NowIso8601();
        if (question.type != QuestionType::Choice) {
            question.options.clear();
        }
        return true;
    };

    std::vector<const jsonx::Value*> nodes;
    const char* listKeys[] = {"questions", "items", "data", "result"};
    for (const char* key : listKeys) {
        const jsonx::Value& listValue = fields[key];
        if (listValue.IsArray()) {
            for (const auto& item : listValue.Items()) nodes.push_back(&item);
            if (!nodes.empty()) break;
        }
    }
    if (nodes.empty()) {
        if (fields["question"].IsObject()) {
            nodes.push_back(&fields["question"]);
        } else if (fields["stem"].IsString()) {
            nodes.push_back(&fields);
        }
    }
    if (nodes.empty()) {
        result.error = "AI 未能从图片中识别出完整题目。请换一张更清晰的图片，"
                       "并确认所用模型支持视觉输入。";
        return result;
    }

    for (const jsonx::Value* node : nodes) {
        Question question;
        if (parseQuestionNode(*node, question)) {
            result.questions.push_back(std::move(question));
        }
        if (result.questions.size() >= 30) break;
    }
    if (result.questions.empty()) {
        result.error = "AI 未能从图片中提取出有效题目。请换一张更清晰、题目更完整的图片。";
        return result;
    }
    result.ok = true;
    return result;
}

namespace {

std::string BuildDocumentPrompt(const std::string& documentText, const TagLibrary& library) {
    std::ostringstream prompt;
    prompt << "你是一名资深高中数学教师。请从下面的文档内容中整理出所有完整的高中数学题，"
              "并独立求解。\n\n只输出 JSON 对象，不要输出 Markdown：\n"
              "{\"questions\":[{\"stem\":\"题干\",\"options\":[\"A选项\"],\"type\":\"choice\","
              "\"difficulty\":\"B\",\"grade\":\"高一\",\"answer\":\"最终答案\","
              "\"analysis\":\"关键步骤\",\"formulas\":[\"公式\"],\"concepts\":[\"知识点\"]}]}\n\n"
              "要求：\n"
              "1. 每道题都必须有 answer 和 analysis，即使原文没有答案也要自己求解。\n"
              "2. 数学表达使用可读 Unicode，不要 LaTeX；分数写成“分子/分母”，程序会排版成"
              "上下分式，不要用 ÷ 或 ／。\n"
              "3. 忽略文档中的页眉、页脚、考试说明等非题目内容；一道题包含多个小问时合并为一道。\n"
              "4. grade 只能是 高一、高二、高三 之一；type 为 choice/fill/solution；"
              "difficulty 为 A/B/C。\n"
              "5. 若文档无法识别或没有完整题目，返回 {\"error\":\"没有找到完整题目\"}。\n\n";
    if (!library.formulas.empty()) {
        prompt << "已有公式标签：" << util::Join(library.formulas, "、") << "\n";
    }
    if (!library.concepts.empty()) {
        prompt << "已有知识点标签：" << util::Join(library.concepts, "、") << "\n";
    }
    prompt << "\n文档内容：\n" << ClampPreview(documentText, 30000);
    return prompt.str();
}

}  // namespace

AiImageQuestionResult AnalyzeQuestionDocumentWithAi(const std::string& documentText,
                                                    const Settings& settings,
                                                    const TagLibrary& library) {
    AiImageQuestionResult result;
    if (util::Trim(settings.model).empty()) {
        result.error = "尚未填写模型名称，请先在“AI 接口”中配置。";
        return result;
    }
    if (util::Trim(documentText).empty()) {
        result.error = "文档中没有可读取的文字。";
        return result;
    }
    std::string systemPrompt =
        "你是一名资深高中数学教师，擅长从试卷文字中整理题目、独立求解并结构化输出。"
        "严格按用户要求只返回 JSON，不要解释。";
    AiChatResult chat = AskAiChat(settings, systemPrompt,
                                  BuildDocumentPrompt(documentText, library), 6000, 150000);
    if (!chat.ok) {
        result.error = chat.error;
        return result;
    }
    result.rawContent = chat.text;
    jsonx::Value fields;
    std::string contentError;
    if (!ExtractJsonObjectFromContent(chat.text, fields, contentError)) {
        result.error = contentError;
        return result;
    }
    std::string topError = util::Trim(fields["error"].AsString());
    if (!topError.empty()) {
        result.error = topError;
        return result;
    }

    auto parseQuestionNode = [](const jsonx::Value& node, Question& question) -> bool {
        if (!node.IsObject()) return false;
        std::string stem = util::Trim(node["stem"].AsString());
        if (stem.empty()) return false;
        question = Question{};
        question.stem = NormalizeMathText(stem);
        const jsonx::Value& options = node["options"];
        if (options.IsArray()) {
            for (const auto& item : options.Items()) {
                if (!item.IsString()) continue;
                std::string option = NormalizeMathText(CleanOptionText(item.AsString()));
                if (!option.empty()) question.options.push_back(std::move(option));
            }
        }
        bool parsedType = ParseQuestionType(node["type"].AsString(), question.type);
        if (!parsedType) {
            question.type =
                question.options.size() >= 2 ? QuestionType::Choice : QuestionType::Fill;
        }
        ParseDifficulty(node["difficulty"].AsString("B"), question.difficulty);
        Grade grade = Grade::Unknown;
        if (ParseGrade(node["grade"].AsString(), grade)) question.grade = grade;
        else question.grade = InferGrade(question);
        question.answer = NormalizeMathText(util::Trim(node["answer"].AsString()));
        question.analysis = NormalizeMathText(util::Trim(node["analysis"].AsString()));
        ReadTagList(node, {"formulas", "formula", "公式"}, question.formulas);
        ReadTagList(node, {"concepts", "concept", "knowledge_points", "knowledgePoints", "知识点"},
                    question.concepts);
        DedupeTags(question.formulas, 8);
        DedupeTags(question.concepts, 8);
        question.tagged = question.HasAnyTag();
        question.id = util::NewId("q");
        question.created = util::NowIso8601();
        if (question.type != QuestionType::Choice) question.options.clear();
        return true;
    };

    std::vector<const jsonx::Value*> nodes;
    for (const char* key : {"questions", "items", "data", "result"}) {
        const jsonx::Value& list = fields[key];
        if (!list.IsArray()) continue;
        for (const auto& item : list.Items()) nodes.push_back(&item);
        if (!nodes.empty()) break;
    }
    if (nodes.empty()) {
        if (fields["question"].IsObject()) nodes.push_back(&fields["question"]);
        else if (fields["stem"].IsString()) nodes.push_back(&fields);
    }
    for (const jsonx::Value* node : nodes) {
        Question question;
        if (parseQuestionNode(*node, question)) {
            result.questions.push_back(std::move(question));
            if (result.questions.size() >= 30) break;
        }
    }
    if (result.questions.empty()) {
        result.error = "AI 没有从文档中整理出有效题目。";
        return result;
    }
    result.ok = true;
    return result;
}

AiChatResult AskAiChat(const Settings& settings, const std::string& systemPrompt,
                       const std::string& userPrompt, int maxTokens, int timeoutMs) {
    AiChatResult result;
    if (util::Trim(settings.model).empty()) {
        result.error = "尚未填写模型名称，请先在“AI 接口”中配置。";
        return result;
    }
    std::string payload = MakeChatPayload(settings, systemPrompt, userPrompt, maxTokens);
    HttpResponse response = HttpPostJson(settings.apiBase, settings.apiKey, payload, timeoutMs);
    if (!response.transportOk) {
        result.error = response.error;
        return result;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(response.body, &parseError);
    if (!root.IsObject()) {
        result.error = "AI 返回内容不是有效 JSON（HTTP " + std::to_string(response.status) + "）。";
        return result;
    }
    if (response.status < 200 || response.status >= 300) {
        result.error = "AI 接口返回错误（HTTP " + std::to_string(response.status) + "）：" +
                       ApiErrorMessage(root, response.status, response.body);
        return result;
    }
    result.text = util::Trim(ExtractMessageContent(root));
    if (result.text.empty()) {
        result.error = "AI 响应中没有文本内容。";
        return result;
    }
    result.ok = true;
    return result;
}

AiTestResult TestAiConnection(const Settings& settings) {
    AiTestResult result;
    if (util::Trim(settings.model).empty()) {
        result.message = "模型名称不能为空。";
        return result;
    }
    std::string payload = MakeChatPayload(
        settings, "你是一个测试助手。只回复 OK，不要添加其他内容。",
        "请回复 OK。", 32);
    HttpResponse response = HttpPostJson(settings.apiBase, settings.apiKey, payload, kTestTimeoutMs);
    if (!response.transportOk) {
        result.message = response.error;
        return result;
    }
    std::string parseError;
    jsonx::Value root = jsonx::Value::Parse(response.body, &parseError);
    if (response.status < 200 || response.status >= 300) {
        if (root.IsObject()) {
            result.message = "接口返回 HTTP " + std::to_string(response.status) + "：" +
                             ApiErrorMessage(root, response.status, response.body);
        } else {
            result.message = "接口返回 HTTP " + std::to_string(response.status) + "：" +
                             ClampPreview(response.body, 300);
        }
        return result;
    }
    if (!root.IsObject()) {
        result.message = "接口已连通，但返回内容不是标准 JSON。";
        return result;
    }
    std::string content = ExtractMessageContent(root);
    result.ok = true;
    result.message = "连接成功";
    if (!util::Trim(content).empty()) {
        result.message += "，模型回复：" + ClampPreview(content, 80);
    } else {
        result.message += "（HTTP 200）";
    }
    return result;
}
