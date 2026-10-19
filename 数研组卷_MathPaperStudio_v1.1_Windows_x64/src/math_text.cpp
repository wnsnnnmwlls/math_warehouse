#include "math_text.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace {

bool IsAsciiSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

std::string TrimAsciiSpace(const std::string& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && IsAsciiSpace(text[begin])) ++begin;
    while (end > begin && IsAsciiSpace(text[end - 1])) --end;
    return text.substr(begin, end - begin);
}

bool StartsAt(const std::string& text, size_t position, std::string_view needle) {
    return position + needle.size() <= text.size() &&
           text.compare(position, needle.size(), needle) == 0;
}

bool IsAsciiAlpha(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
}

std::string CleanSpaces(const std::string& source) {
    std::string collapsed;
    collapsed.reserve(source.size());
    bool pendingSpace = false;
    for (unsigned char c : source) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
            if (!collapsed.empty()) pendingSpace = true;
            continue;
        }
        if (pendingSpace) {
            collapsed.push_back(' ');
            pendingSpace = false;
        }
        collapsed.push_back(static_cast<char>(c));
    }

    std::string result;
    result.reserve(collapsed.size());
    for (size_t i = 0; i < collapsed.size(); ++i) {
        char c = collapsed[i];
        if (c != ' ') {
            result.push_back(c);
            continue;
        }
        char previous = result.empty() ? '\0' : result.back();
        char next = i + 1 < collapsed.size() ? collapsed[i + 1] : '\0';
        bool drop = previous == '(' || previous == '[' || previous == '/' || previous == '{';
        if (next == ')' || next == ']' || next == ',' || next == '.' || next == ';' ||
            next == ':' || next == '/' || next == '(' || next == '[' || next == '%') {
            drop = true;
        }
        if (next == '\0') drop = true;
        if (!drop) {
            result.push_back(' ');
        }
    }
    return TrimAsciiSpace(result);
}

const std::unordered_map<std::string, std::string>& SymbolMap() {
    static const std::unordered_map<std::string, std::string> values = {
        {"alpha", "α"},       {"beta", "β"},        {"gamma", "γ"},
        {"delta", "δ"},       {"epsilon", "ε"},     {"varepsilon", "ε"},
        {"zeta", "ζ"},        {"eta", "η"},         {"theta", "θ"},
        {"vartheta", "ϑ"},    {"iota", "ι"},        {"kappa", "κ"},
        {"lambda", "λ"},      {"mu", "μ"},          {"nu", "ν"},
        {"xi", "ξ"},          {"omicron", "ο"},     {"pi", "π"},
        {"varpi", "ϖ"},       {"rho", "ρ"},         {"varrho", "ϱ"},
        {"sigma", "σ"},       {"varsigma", "ς"},    {"tau", "τ"},
        {"upsilon", "υ"},     {"phi", "φ"},         {"varphi", "ϕ"},
        {"chi", "χ"},         {"psi", "ψ"},         {"omega", "ω"},
        {"Gamma", "Γ"},       {"Delta", "Δ"},       {"Theta", "Θ"},
        {"Lambda", "Λ"},      {"Xi", "Ξ"},          {"Pi", "Π"},
        {"Sigma", "Σ"},       {"Upsilon", "Υ"},     {"Phi", "Φ"},
        {"Psi", "Ψ"},         {"Omega", "Ω"},
        {"cdot", "·"},        {"times", "×"},       {"div", "÷"},
        {"pm", "±"},          {"mp", "∓"},          {"ast", "∗"},
        {"star", "⋆"},        {"bullet", "•"},
        {"le", "≤"},          {"leq", "≤"},         {"leqslant", "≤"},
        {"ge", "≥"},          {"geq", "≥"},         {"geqslant", "≥"},
        {"ne", "≠"},          {"neq", "≠"},         {"approx", "≈"},
        {"equiv", "≡"},       {"sim", "∼"},         {"simeq", "≃"},
        {"propto", "∝"},      {"in", "∈"},          {"notin", "∉"},
        {"ni", "∋"},          {"subset", "⊂"},      {"subseteq", "⊆"},
        {"supset", "⊃"},      {"supseteq", "⊇"},    {"cup", "∪"},
        {"cap", "∩"},         {"emptyset", "∅"},    {"varnothing", "∅"},
        {"forall", "∀"},      {"exists", "∃"},      {"nexists", "∄"},
        {"therefore", "∴"},   {"because", "∵"},     {"infty", "∞"},
        {"partial", "∂"},     {"nabla", "∇"},       {"angle", "∠"},
        {"measuredangle", "∡"}, {"triangle", "△"},  {"perp", "⊥"},
        {"parallel", "∥"},    {"cong", "≅"},        {"mid", "∣"},
        {"colon", ":"},       {"prime", "′"},       {"degree", "°"},
        {"circ", "°"},        {"ell", "ℓ"},         {"square", "□"},
        {"Box", "□"},         {"to", "→"},          {"rightarrow", "→"},
        {"leftarrow", "←"},   {"leftrightarrow", "↔"}, {"Rightarrow", "⇒"},
        {"Leftarrow", "⇐"},   {"Leftrightarrow", "⇔"}, {"mapsto", "↦"},
        {"uparrow", "↑"},     {"downarrow", "↓"},   {"sum", "∑"},
        {"prod", "∏"},        {"int", "∫"},         {"iint", "∬"},
        {"oint", "∮"},        {"ldots", "…"},       {"dots", "…"},
        {"cdots", "…"},       {"vdots", "⋮"},       {"ddots", "⋱"},
        {"backslash", "\\"},  {"lvert", "|"},       {"rvert", "|"},
        {"vert", "|"},
    };
    return values;
}

const std::unordered_set<std::string>& FunctionNames() {
    static const std::unordered_set<std::string> names = {
        "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan",
        "arccot", "sinh", "cosh", "tanh", "coth", "log", "ln", "lg", "exp",
        "lim", "limsup", "liminf", "max", "min", "sup", "inf", "det", "gcd",
        "deg", "dim", "ker", "hom", "Pr",
    };
    return names;
}

std::string MapSuperscript(const std::string& content) {
    static const std::unordered_map<char, std::string> map = {
        {'0', "⁰"}, {'1', "¹"}, {'2', "²"}, {'3', "³"}, {'4', "⁴"},
        {'5', "⁵"}, {'6', "⁶"}, {'7', "⁷"}, {'8', "⁸"}, {'9', "⁹"},
        {'+', "⁺"}, {'-', "⁻"}, {'=', "⁼"}, {'(', "⁽"}, {')', "⁾"},
        {'a', "ᵃ"}, {'b', "ᵇ"}, {'c', "ᶜ"}, {'d', "ᵈ"}, {'e', "ᵉ"},
        {'f', "ᶠ"}, {'g', "ᵍ"}, {'h', "ʰ"}, {'i', "ⁱ"}, {'j', "ʲ"},
        {'k', "ᵏ"}, {'l', "ˡ"}, {'m', "ᵐ"}, {'n', "ⁿ"}, {'o', "ᵒ"},
        {'p', "ᵖ"}, {'r', "ʳ"}, {'s', "ˢ"}, {'t', "ᵗ"}, {'u', "ᵘ"},
        {'v', "ᵛ"}, {'w', "ʷ"}, {'x', "ˣ"}, {'y', "ʸ"}, {'z', "ᶻ"},
        {'A', "ᴬ"}, {'B', "ᴮ"}, {'D', "ᴰ"}, {'E', "ᴱ"}, {'G', "ᴳ"},
        {'H', "ᴴ"}, {'I', "ᴵ"}, {'J', "ᴶ"}, {'K', "ᴷ"}, {'L', "ᴸ"},
        {'M', "ᴹ"}, {'N', "ᴺ"}, {'O', "ᴼ"}, {'P', "ᴾ"}, {'R', "ᴿ"},
        {'T', "ᵀ"}, {'U', "ᵁ"}, {'V', "ⱽ"}, {'W', "ᵂ"},
    };
    if (content == "°") return "°";
    if (content.empty()) return {};
    std::string result;
    for (char c : content) {
        auto it = map.find(c);
        if (it == map.end()) return {};
        result += it->second;
    }
    return result;
}

std::string MapSubscript(const std::string& content) {
    static const std::unordered_map<char, std::string> map = {
        {'0', "₀"}, {'1', "₁"}, {'2', "₂"}, {'3', "₃"}, {'4', "₄"},
        {'5', "₅"}, {'6', "₆"}, {'7', "₇"}, {'8', "₈"}, {'9', "₉"},
        {'+', "₊"}, {'-', "₋"}, {'=', "₌"}, {'(', "₍"}, {')', "₎"},
        {'a', "ₐ"}, {'e', "ₑ"}, {'h', "ₕ"}, {'i', "ᵢ"}, {'j', "ⱼ"},
        {'k', "ₖ"}, {'l', "ₗ"}, {'m', "ₘ"}, {'n', "ₙ"}, {'o', "ₒ"},
        {'p', "ₚ"}, {'r', "ᵣ"}, {'s', "ₛ"}, {'t', "ₜ"}, {'u', "ᵤ"},
        {'v', "ᵥ"}, {'x', "ₓ"},
    };
    if (content.empty()) return {};
    std::string result;
    for (char c : content) {
        auto it = map.find(c);
        if (it == map.end()) return {};
        result += it->second;
    }
    return result;
}

bool NeedsGrouping(const std::string& expression) {
    std::string text = TrimAsciiSpace(expression);
    if (text.size() >= 2 && text.front() == '(' && text.back() == ')') {
        return false;
    }
    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '+' || c == '-' || c == '=' || c == '/' ||
            c == '<' || c == '>' || c == ',') {
            return true;
        }
    }
    return text.find("≤") != std::string::npos || text.find("≥") != std::string::npos ||
           text.find("≠") != std::string::npos;
}

std::string WrapIfNeeded(const std::string& expression) {
    std::string text = TrimAsciiSpace(expression);
    if (text.empty()) return text;
    if (NeedsGrouping(text)) return "(" + text + ")";
    return text;
}

std::string FormatScript(const std::string& content, bool superscript) {
    std::string trimmed = TrimAsciiSpace(content);
    if (trimmed.empty()) return {};
    std::string mapped = superscript ? MapSuperscript(trimmed) : MapSubscript(trimmed);
    if (!mapped.empty()) return mapped;
    return std::string(superscript ? "^" : "_") + "(" + trimmed + ")";
}

class MathParser {
public:
    explicit MathParser(const std::string& input) : input_(input) {}

    std::string Parse() {
        std::string result = ParseSequence('\0');
        return CleanSpaces(result);
    }

private:
    const std::string& input_;
    size_t position_ = 0;

    void SkipSpaces() {
        while (position_ < input_.size() && IsAsciiSpace(input_[position_])) {
            ++position_;
        }
    }

    std::string ParseSequence(char terminator) {
        std::string output;
        while (position_ < input_.size()) {
            char c = input_[position_];
            if (terminator != '\0' && c == terminator) {
                break;
            }
            if (c == '}') {
                if (terminator == '}') break;
                output.push_back(c);
                ++position_;
                continue;
            }
            if (c == '$') {
                ++position_;
                continue;
            }
            if (c == '\\') {
                output += ParseCommand();
                continue;
            }
            if (c == '{') {
                // Plain braces are usually literal set braces in model output. LaTeX grouping
                // braces that delimit command arguments are consumed by ParseArgument instead.
                output.push_back('{');
                ++position_;
                continue;
            }
            if (c == '^' || c == '_') {
                ++position_;
                output += ParseScript(c == '^');
                continue;
            }
            if (IsAsciiSpace(c)) {
                while (position_ < input_.size() && IsAsciiSpace(input_[position_])) {
                    ++position_;
                }
                if (!output.empty() && output.back() != ' ') output.push_back(' ');
                continue;
            }
            output.push_back(c);
            ++position_;
        }
        return output;
    }

    std::string ParseArgument() {
        SkipSpaces();
        if (position_ >= input_.size()) return {};
        if (input_[position_] == '{') {
            ++position_;
            std::string inner = ParseSequence('}');
            if (position_ < input_.size() && input_[position_] == '}') {
                ++position_;
            }
            return inner;
        }
        if (input_[position_] == '\\') {
            return ParseCommand();
        }
        size_t start = position_;
        unsigned char lead = static_cast<unsigned char>(input_[position_]);
        size_t length = 1;
        if ((lead & 0xE0) == 0xC0) length = 2;
        else if ((lead & 0xF0) == 0xE0) length = 3;
        else if ((lead & 0xF8) == 0xF0) length = 4;
        while (position_ < input_.size() && position_ < start + length) ++position_;
        return input_.substr(start, position_ - start);
    }

    std::string ParseScript(bool superscript) {
        std::string content = ParseArgument();
        std::string trimmed = TrimAsciiSpace(content);
        if (trimmed.empty()) return {};
        if (superscript && trimmed == "°") return "°";
        return FormatScript(trimmed, superscript);
    }

    std::string ParseFraction() {
        std::string numerator = CleanSpaces(ParseArgument());
        std::string denominator = CleanSpaces(ParseArgument());
        if (numerator.empty() || denominator.empty()) {
            return numerator + denominator;
        }
        return WrapIfNeeded(numerator) + "/" + WrapIfNeeded(denominator);
    }

    std::string ParseRoot() {
        SkipSpaces();
        std::string rootIndex;
        if (position_ < input_.size() && input_[position_] == '[') {
            size_t close = input_.find(']', position_ + 1);
            if (close != std::string::npos) {
                rootIndex = TrimAsciiSpace(input_.substr(position_ + 1, close - position_ - 1));
                position_ = close + 1;
            }
        }
        std::string content = CleanSpaces(ParseArgument());
        if (content.empty()) return "√";
        std::string prefix = "√";
        if (rootIndex == "3") prefix = "∛";
        else if (rootIndex == "4") prefix = "∜";
        else if (!rootIndex.empty()) {
            std::string mapped = MapSuperscript(rootIndex);
            prefix = (mapped.empty() ? "[" + rootIndex + "]" : mapped) + "√";
        }
        return prefix + WrapIfNeeded(content);
    }

    std::string ParseCommand() {
        ++position_;  // backslash
        if (position_ >= input_.size()) return {};
        if (!IsAsciiAlpha(input_[position_])) {
            char escaped = input_[position_++];
            switch (escaped) {
                case '{': return "{";
                case '}': return "}";
                case '%': return "%";
                case '&': return "&";
                case '_': return "_";
                case '#': return "#";
                case '$': return "$";
                case '\\': return " ";
                case ',': case ';': case ':': case ' ': return " ";
                case '!': return {};
                default: return std::string(1, escaped);
            }
        }
        size_t start = position_;
        while (position_ < input_.size() && IsAsciiAlpha(input_[position_])) {
            ++position_;
        }
        std::string name = input_.substr(start, position_ - start);
        if (position_ < input_.size() && input_[position_] == '*') ++position_;
        if (name == "frac" || name == "dfrac" || name == "tfrac") {
            return ParseFraction();
        }
        if (name == "sqrt") {
            return ParseRoot();
        }
        if (name == "left" || name == "right") {
            SkipSpaces();
            if (position_ < input_.size() && input_[position_] == '.') ++position_;
            return {};
        }
        if (name == "big" || name == "Big" || name == "bigg" || name == "Bigg" ||
            name == "bigl" || name == "bigr" || name == "Bigl" || name == "Bigr" ||
            name == "biggl" || name == "biggr" || name == "Biggl" || name == "Biggr") {
            return {};
        }
        if (name == "text" || name == "textrm" || name == "textbf" || name == "textit" ||
            name == "mathrm" || name == "mathbf" || name == "mathit" || name == "mathsf" ||
            name == "mathtt" || name == "mathcal" || name == "mathfrak" ||
            name == "mathbb" || name == "textnormal" || name == "textsf") {
            std::string content = ParseArgument();
            if (name == "mathbb") {
                std::string mapped;
                for (char c : content) {
                    switch (c) {
                        case 'R': mapped += "ℝ"; break;
                        case 'N': mapped += "ℕ"; break;
                        case 'Z': mapped += "ℤ"; break;
                        case 'Q': mapped += "ℚ"; break;
                        case 'C': mapped += "ℂ"; break;
                        default: mapped.push_back(c); break;
                    }
                }
                return mapped;
            }
            return content;
        }
        if (name == "operatorname") {
            return ParseArgument() + " ";
        }
        if (name == "overline" || name == "bar") {
            return ParseArgument();
        }
        if (name == "underline") {
            return ParseArgument();
        }
        if (name == "vec" || name == "overrightarrow") {
            return "向量" + ParseArgument();
        }
        if (name == "hat" || name == "widehat") {
            return ParseArgument() + "̂";
        }
        if (name == "begin" || name == "end") {
            ParseArgument();
            return " ";
        }
        if (name == "quad" || name == "qquad" || name == "enspace" || name == "space") {
            return " ";
        }
        if (name == "displaystyle" || name == "textstyle" || name == "scriptstyle" ||
            name == "scriptscriptstyle" || name == "limits" || name == "nolimits") {
            return {};
        }
        if (name == "not") {
            return "¬";
        }
        auto functionIt = FunctionNames().find(name);
        if (functionIt != FunctionNames().end()) {
            return name + " ";
        }
        auto symbolIt = SymbolMap().find(name);
        if (symbolIt != SymbolMap().end()) {
            return symbolIt->second;
        }
        return name;
    }
};

bool LooksLikeLatex(const std::string& text) {
    static const char* markers[] = {
        "\\frac", "\\dfrac", "\\sqrt", "\\alpha", "\\beta", "\\pi", "\\theta",
        "\\cos", "\\sin", "\\tan", "\\left", "\\right", "\\begin", "\\mathrm",
        "\\text", "^{", "_{",
    };
    for (const char* marker : markers) {
        if (text.find(marker) != std::string::npos) return true;
    }
    return false;
}

std::string FormatMath(const std::string& source) {
    return MathParser(source).Parse();
}

}  // namespace

std::string NormalizeMathText(const std::string& text) {
    if (text.empty()) return text;
    std::string output;
    output.reserve(text.size());
    size_t position = 0;
    bool foundMath = false;

    while (position < text.size()) {
        if (text[position] == '$') {
            const bool display = position + 1 < text.size() && text[position + 1] == '$';
            const std::string closing = display ? "$$" : "$";
            const size_t contentStart = position + (display ? 2 : 1);
            size_t end = text.find(closing, contentStart);
            if (end != std::string::npos) {
                output += FormatMath(text.substr(contentStart, end - contentStart));
                position = end + closing.size();
                foundMath = true;
                continue;
            }
            ++position;
            continue;
        }
        if (StartsAt(text, position, "\\(")) {
            size_t end = text.find("\\)", position + 2);
            if (end != std::string::npos) {
                output += FormatMath(text.substr(position + 2, end - position - 2));
                position = end + 2;
                foundMath = true;
                continue;
            }
        }
        if (StartsAt(text, position, "\\[")) {
            size_t end = text.find("\\]", position + 2);
            if (end != std::string::npos) {
                output += FormatMath(text.substr(position + 2, end - position - 2));
                position = end + 2;
                foundMath = true;
                continue;
            }
        }
        output.push_back(text[position]);
        ++position;
    }

    if (!foundMath && LooksLikeLatex(output)) {
        output = FormatMath(output);
    }

    // Keep fractions machine-typesettable and human-readable: the UI draws
    // "a/b" as a stacked fraction, while division signs and full-width slashes
    // would otherwise stay as inline glyphs.
    std::string normalized;
    normalized.reserve(output.size());
    for (size_t i = 0; i < output.size();) {
        if (output.compare(i, 2, "\xC3\xB7") == 0) {  // U+00F7 ÷
            normalized.push_back('/');
            i += 2;
        } else if (output.compare(i, 3, "\xEF\xBC\x8F") == 0) {  // U+FF0F ／
            normalized.push_back('/');
            i += 3;
        } else if (output.compare(i, 3, "\xE2\x88\x92") == 0) {  // U+2212 −
            normalized.push_back('-');
            i += 3;
        } else {
            normalized.push_back(output[i]);
            ++i;
        }
    }
    return normalized;
}
