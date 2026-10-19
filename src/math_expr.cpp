#include "math_expr.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mathexpr {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kE = 2.718281828459045235360287471352662498;

double QuietNaN() {
    return std::numeric_limits<double>::quiet_NaN();
}

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

enum class TokenKind {
    Number,
    Identifier,
    Plus,
    Minus,
    Star,
    Slash,
    Caret,
    LeftParen,
    RightParen,
    Root,
    End,
};

enum class Symbol {
    X,
    Pi,
    E,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Sqrt,
    Abs,
    Log10,
    Ln,
    Exp,
    Floor,
    Ceil,
};

struct Token {
    TokenKind kind = TokenKind::End;
    Symbol symbol = Symbol::X;
    double number = 0.0;
    std::size_t position = 0;
};

struct NameEntry {
    std::string_view name;
    Symbol symbol;
};

// Longest match wins, so identifiers may be concatenated: xcos(x) is x*cos(x).
constexpr NameEntry kNameTable[] = {
    {"asin", Symbol::Asin}, {"acos", Symbol::Acos}, {"atan", Symbol::Atan},
    {"sqrt", Symbol::Sqrt}, {"floor", Symbol::Floor}, {"ceil", Symbol::Ceil},
    {"sin", Symbol::Sin},   {"cos", Symbol::Cos},   {"tan", Symbol::Tan},
    {"abs", Symbol::Abs},   {"log", Symbol::Log10}, {"exp", Symbol::Exp},
    {"ln", Symbol::Ln},     {"pi", Symbol::Pi},     {"e", Symbol::E},
    {"x", Symbol::X},
};

std::string_view SymbolName(Symbol symbol) {
    switch (symbol) {
        case Symbol::X:
            return "x";
        case Symbol::Pi:
            return "pi";
        case Symbol::E:
            return "e";
        case Symbol::Sin:
            return "sin";
        case Symbol::Cos:
            return "cos";
        case Symbol::Tan:
            return "tan";
        case Symbol::Asin:
            return "asin";
        case Symbol::Acos:
            return "acos";
        case Symbol::Atan:
            return "atan";
        case Symbol::Sqrt:
            return "sqrt";
        case Symbol::Abs:
            return "abs";
        case Symbol::Log10:
            return "log";
        case Symbol::Ln:
            return "ln";
        case Symbol::Exp:
            return "exp";
        case Symbol::Floor:
            return "floor";
        case Symbol::Ceil:
            return "ceil";
    }
    return "?";
}

bool IsFunction(Symbol symbol) {
    switch (symbol) {
        case Symbol::Sin:
        case Symbol::Cos:
        case Symbol::Tan:
        case Symbol::Asin:
        case Symbol::Acos:
        case Symbol::Atan:
        case Symbol::Sqrt:
        case Symbol::Abs:
        case Symbol::Log10:
        case Symbol::Ln:
        case Symbol::Exp:
        case Symbol::Floor:
        case Symbol::Ceil:
            return true;
        case Symbol::X:
        case Symbol::Pi:
        case Symbol::E:
            return false;
    }
    return false;
}

bool IsAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

bool IsAsciiLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsAsciiSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

bool StartsWith(const std::string& text, std::size_t position, std::string_view prefix) {
    return position <= text.size() && prefix.size() <= text.size() - position &&
           text.compare(position, prefix.size(), prefix.data(), prefix.size()) == 0;
}

std::size_t MatchKnownName(const std::string& text, std::size_t position, Symbol& symbol) {
    std::size_t bestLength = 0;
    Symbol bestSymbol = Symbol::X;
    for (const NameEntry& entry : kNameTable) {
        if (entry.name.size() <= bestLength || entry.name.size() > text.size() - position) {
            continue;
        }
        bool matches = true;
        for (std::size_t i = 0; i < entry.name.size(); ++i) {
            char c = text[position + i];
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
            if (c != entry.name[i]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            bestLength = entry.name.size();
            bestSymbol = entry.symbol;
        }
    }
    if (bestLength != 0) {
        symbol = bestSymbol;
    }
    return bestLength;
}

bool LexError(std::string* error, std::size_t position, const std::string& message) {
    if (error != nullptr) {
        *error = message + " at position " + std::to_string(position);
    }
    return false;
}

bool ScanNumber(const std::string& text, std::size_t& index, double& output,
                std::string* error) {
    const std::size_t start = index;
    bool anyDigit = false;
    double value = 0.0;

    while (index < text.size() && IsAsciiDigit(text[index])) {
        value = value * 10.0 + static_cast<double>(text[index] - '0');
        anyDigit = true;
        ++index;
    }

    if (index < text.size() && text[index] == '.') {
        ++index;
        double place = 0.1;
        while (index < text.size() && IsAsciiDigit(text[index])) {
            value += static_cast<double>(text[index] - '0') * place;
            place *= 0.1;
            anyDigit = true;
            ++index;
        }
    }

    if (!anyDigit) {
        return LexError(error, start, "invalid number");
    }

    // Scientific notation is accepted when 'e' is followed by an exponent.
    // Otherwise a bare 'e' remains the constant e (for example, 2e = 2*e).
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        std::size_t probe = index + 1;
        int exponentSign = 1;
        if (probe < text.size() && (text[probe] == '+' || text[probe] == '-')) {
            if (text[probe] == '-') {
                exponentSign = -1;
            }
            ++probe;
        }
        if (probe < text.size() && IsAsciiDigit(text[probe])) {
            index = probe;
            int exponent = 0;
            while (index < text.size() && IsAsciiDigit(text[index])) {
                if (exponent < 100000) {
                    exponent = exponent * 10 + (text[index] - '0');
                }
                ++index;
            }
            if (value != 0.0) {
                value *= std::pow(10.0, static_cast<double>(exponentSign * exponent));
            }
        }
    }

    output = value;
    return true;
}

bool Tokenize(const std::string& text, std::vector<Token>& tokens, std::string* error) {
    tokens.clear();
    std::size_t index = 0;

    while (index < text.size()) {
        const char c = text[index];
        if (IsAsciiSpace(c)) {
            ++index;
            continue;
        }

        Token token;
        token.position = index;

        if (IsAsciiDigit(c) || c == '.') {
            if (!ScanNumber(text, index, token.number, error)) {
                return false;
            }
            token.kind = TokenKind::Number;
            tokens.push_back(token);
            continue;
        }

        if (IsAsciiLetter(c)) {
            Symbol symbol = Symbol::X;
            const std::size_t length = MatchKnownName(text, index, symbol);
            if (length == 0) {
                std::size_t end = index;
                while (end < text.size() && IsAsciiLetter(text[end])) {
                    ++end;
                }
                return LexError(error, index,
                                "unknown identifier \"" + text.substr(index, end - index) + "\"");
            }
            token.kind = TokenKind::Identifier;
            token.symbol = symbol;
            index += length;
            tokens.push_back(token);
            continue;
        }

        if (StartsWith(text, index, "\xCF\x80")) {  // U+03C0 GREEK SMALL LETTER PI
            token.kind = TokenKind::Identifier;
            token.symbol = Symbol::Pi;
            index += 2;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xCE\xA0")) {  // U+03A0 GREEK CAPITAL LETTER PI
            token.kind = TokenKind::Identifier;
            token.symbol = Symbol::Pi;
            index += 2;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xE2\x88\x9A")) {  // U+221A SQUARE ROOT
            token.kind = TokenKind::Root;
            index += 3;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xC3\x97") ||   // U+00D7 MULTIPLICATION SIGN
            StartsWith(text, index, "\xC2\xB7")) {  // U+00B7 MIDDLE DOT
            token.kind = TokenKind::Star;
            index += 2;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xC3\xB7")) {  // U+00F7 DIVISION SIGN
            token.kind = TokenKind::Slash;
            index += 2;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xE2\x88\x92")) {  // U+2212 MINUS SIGN
            token.kind = TokenKind::Minus;
            index += 3;
            tokens.push_back(token);
            continue;
        }
        if (StartsWith(text, index, "\xC2\xB0")) {  // U+00B0 DEGREE SIGN
            return LexError(error, index,
                            "degree signs are not supported; trigonometric functions use radians");
        }

        switch (c) {
            case '+':
                token.kind = TokenKind::Plus;
                break;
            case '-':
                token.kind = TokenKind::Minus;
                break;
            case '*':
                token.kind = TokenKind::Star;
                break;
            case '/':
                token.kind = TokenKind::Slash;
                break;
            case '^':
                token.kind = TokenKind::Caret;
                break;
            case '(':
                token.kind = TokenKind::LeftParen;
                break;
            case ')':
                token.kind = TokenKind::RightParen;
                break;
            default:
                return LexError(error, index, "unexpected or unsupported character");
        }

        ++index;
        tokens.push_back(token);
    }

    Token end;
    end.kind = TokenKind::End;
    end.position = text.size();
    tokens.push_back(end);
    return true;
}

bool CanEndImplicitProduct(const Token& token) {
    if (token.kind == TokenKind::Number || token.kind == TokenKind::RightParen) {
        return true;
    }
    return token.kind == TokenKind::Identifier && !IsFunction(token.symbol);
}

bool CanStartImplicitProduct(const Token& token) {
    return token.kind == TokenKind::Number || token.kind == TokenKind::Identifier ||
           token.kind == TokenKind::LeftParen || token.kind == TokenKind::Root;
}

std::vector<Token> InsertImplicitMultiplication(const std::vector<Token>& tokens) {
    std::vector<Token> result;
    result.reserve(tokens.size() * 2);

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            const Token& previous = tokens[i - 1];
            const Token& current = tokens[i];
            // Adjacent numeric literals are not implicit multiplication; they are
            // almost always a typo (for example "2..5").
            const bool bothNumbers =
                previous.kind == TokenKind::Number && current.kind == TokenKind::Number;
            if (!bothNumbers && CanEndImplicitProduct(previous) &&
                CanStartImplicitProduct(current)) {
                Token star;
                star.kind = TokenKind::Star;
                star.position = current.position;
                result.push_back(star);
            }
        }
        result.push_back(tokens[i]);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Expression tree
// ---------------------------------------------------------------------------

enum class NodeType {
    Number,
    Variable,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
    Function,
};

enum class Function {
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Sqrt,
    Abs,
    Log10,
    Ln,
    Exp,
    Floor,
    Ceil,
};

struct Node {
    NodeType type = NodeType::Number;
    double number = 0.0;
    Function function = Function::Sin;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

using NodePtr = std::unique_ptr<Node>;

NodePtr MakeNumber(double value) {
    auto node = std::make_unique<Node>();
    node->type = NodeType::Number;
    node->number = value;
    return node;
}

NodePtr MakeVariable() {
    auto node = std::make_unique<Node>();
    node->type = NodeType::Variable;
    return node;
}

NodePtr MakeUnary(NodeType type, NodePtr operand) {
    auto node = std::make_unique<Node>();
    node->type = type;
    node->left = std::move(operand);
    return node;
}

NodePtr MakeBinary(NodeType type, NodePtr left, NodePtr right) {
    auto node = std::make_unique<Node>();
    node->type = type;
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
}

NodePtr MakeFunction(Function function, NodePtr argument) {
    auto node = std::make_unique<Node>();
    node->type = NodeType::Function;
    node->function = function;
    node->left = std::move(argument);
    return node;
}

Function ToFunction(Symbol symbol) {
    switch (symbol) {
        case Symbol::Sin:
            return Function::Sin;
        case Symbol::Cos:
            return Function::Cos;
        case Symbol::Tan:
            return Function::Tan;
        case Symbol::Asin:
            return Function::Asin;
        case Symbol::Acos:
            return Function::Acos;
        case Symbol::Atan:
            return Function::Atan;
        case Symbol::Sqrt:
            return Function::Sqrt;
        case Symbol::Abs:
            return Function::Abs;
        case Symbol::Log10:
            return Function::Log10;
        case Symbol::Ln:
            return Function::Ln;
        case Symbol::Exp:
            return Function::Exp;
        case Symbol::Floor:
            return Function::Floor;
        case Symbol::Ceil:
            return Function::Ceil;
        case Symbol::X:
        case Symbol::Pi:
        case Symbol::E:
            break;
    }
    return Function::Sin;
}

// ---------------------------------------------------------------------------
// Recursive-descent parser
// ---------------------------------------------------------------------------

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    NodePtr Parse() {
        NodePtr root = ParseExpression();
        if (!root) {
            return nullptr;
        }
        if (Peek().kind != TokenKind::End) {
            SetError(Peek(), "unexpected token after expression");
            return nullptr;
        }
        return root;
    }

    const std::string& error() const { return error_; }

private:
    const Token& Peek() const { return tokens_[position_]; }

    const Token& Advance() { return tokens_[position_++]; }

    bool Match(TokenKind kind) {
        if (Peek().kind != kind) {
            return false;
        }
        ++position_;
        return true;
    }

    void SetError(const Token& token, const std::string& message) {
        if (!error_.empty()) {
            return;
        }
        error_ = message + " at position " + std::to_string(token.position);
        if (token.kind == TokenKind::End) {
            error_ += " (end of expression)";
        }
    }

    NodePtr ParseExpression() {
        NodePtr left = ParseTerm();
        if (!left) {
            return nullptr;
        }
        while (Peek().kind == TokenKind::Plus || Peek().kind == TokenKind::Minus) {
            const NodeType type =
                Peek().kind == TokenKind::Plus ? NodeType::Add : NodeType::Subtract;
            Advance();
            NodePtr right = ParseTerm();
            if (!right) {
                return nullptr;
            }
            left = MakeBinary(type, std::move(left), std::move(right));
        }
        return left;
    }

    NodePtr ParseTerm() {
        NodePtr left = ParseUnary();
        if (!left) {
            return nullptr;
        }
        while (Peek().kind == TokenKind::Star || Peek().kind == TokenKind::Slash) {
            const NodeType type =
                Peek().kind == TokenKind::Star ? NodeType::Multiply : NodeType::Divide;
            Advance();
            NodePtr right = ParseUnary();
            if (!right) {
                return nullptr;
            }
            left = MakeBinary(type, std::move(left), std::move(right));
        }
        return left;
    }

    NodePtr ParseUnary() {
        if (Match(TokenKind::Plus)) {
            return ParseUnary();
        }
        if (Match(TokenKind::Minus)) {
            NodePtr operand = ParseUnary();
            if (!operand) {
                return nullptr;
            }
            return MakeUnary(NodeType::Negate, std::move(operand));
        }
        return ParsePower();
    }

    NodePtr ParsePower() {
        NodePtr left = ParsePrimary();
        if (!left) {
            return nullptr;
        }
        if (Match(TokenKind::Caret)) {
            NodePtr right = ParseUnary();
            if (!right) {
                return nullptr;
            }
            left = MakeBinary(NodeType::Power, std::move(left), std::move(right));
        }
        return left;
    }

    NodePtr ParsePrimary() {
        const Token token = Peek();
        switch (token.kind) {
            case TokenKind::Number:
                Advance();
                return MakeNumber(token.number);

            case TokenKind::LeftParen: {
                Advance();
                NodePtr inner = ParseExpression();
                if (!inner) {
                    return nullptr;
                }
                if (!Match(TokenKind::RightParen)) {
                    SetError(Peek(), "expected ')' to close parenthesized expression");
                    return nullptr;
                }
                return inner;
            }

            case TokenKind::Identifier: {
                Advance();
                switch (token.symbol) {
                    case Symbol::X:
                        return MakeVariable();
                    case Symbol::Pi:
                        return MakeNumber(kPi);
                    case Symbol::E:
                        return MakeNumber(kE);
                    default:
                        break;
                }
                return ParseFunctionCall(token.symbol);
            }

            case TokenKind::Root:
                Advance();
                return ParseRootArgument();

            default:
                SetError(token, token.kind == TokenKind::End ? "unexpected end of expression"
                                                             : "expected an expression");
                return nullptr;
        }
    }

    NodePtr ParseFunctionCall(Symbol symbol) {
        const std::string name(SymbolName(symbol));
        if (!Match(TokenKind::LeftParen)) {
            SetError(Peek(), "expected '(' after function '" + name + "'");
            return nullptr;
        }
        if (Peek().kind == TokenKind::RightParen) {
            SetError(Peek(), "missing argument for function '" + name + "'");
            return nullptr;
        }
        NodePtr argument = ParseExpression();
        if (!argument) {
            return nullptr;
        }
        if (!Match(TokenKind::RightParen)) {
            SetError(Peek(), "expected ')' after argument of function '" + name + "'");
            return nullptr;
        }
        return MakeFunction(ToFunction(symbol), std::move(argument));
    }

    NodePtr ParseRootArgument() {
        // √ binds to the next primary expression. Parentheses extend it:
        // √(x+1) is sqrt(x+1), and √x + 1 is sqrt(x) + 1.
        NodePtr argument = ParsePrimary();
        if (!argument) {
            return nullptr;
        }
        return MakeFunction(Function::Sqrt, std::move(argument));
    }

    const std::vector<Token>& tokens_;
    std::size_t position_ = 0;
    std::string error_;
};

// ---------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------

double EvaluateNode(const Node& node, double x) {
    switch (node.type) {
        case NodeType::Number:
            return node.number;
        case NodeType::Variable:
            return x;
        case NodeType::Negate: {
            const double value = EvaluateNode(*node.left, x);
            return -value;
        }
        case NodeType::Add:
            return EvaluateNode(*node.left, x) + EvaluateNode(*node.right, x);
        case NodeType::Subtract:
            return EvaluateNode(*node.left, x) - EvaluateNode(*node.right, x);
        case NodeType::Multiply:
            return EvaluateNode(*node.left, x) * EvaluateNode(*node.right, x);
        case NodeType::Divide: {
            const double numerator = EvaluateNode(*node.left, x);
            const double denominator = EvaluateNode(*node.right, x);
            if (denominator == 0.0) {
                return QuietNaN();
            }
            return numerator / denominator;
        }
        case NodeType::Power: {
            const double base = EvaluateNode(*node.left, x);
            const double exponent = EvaluateNode(*node.right, x);
            if (base == 0.0 && exponent < 0.0) {
                return QuietNaN();
            }
            return std::pow(base, exponent);
        }
        case NodeType::Function: {
            const double value = EvaluateNode(*node.left, x);
            switch (node.function) {
                case Function::Sin:
                    return std::sin(value);
                case Function::Cos:
                    return std::cos(value);
                case Function::Tan:
                    return std::tan(value);
                case Function::Asin:
                    if (value < -1.0 || value > 1.0) {
                        return QuietNaN();
                    }
                    return std::asin(value);
                case Function::Acos:
                    if (value < -1.0 || value > 1.0) {
                        return QuietNaN();
                    }
                    return std::acos(value);
                case Function::Atan:
                    return std::atan(value);
                case Function::Sqrt:
                    if (value < 0.0) {
                        return QuietNaN();
                    }
                    return std::sqrt(value);
                case Function::Abs:
                    return std::fabs(value);
                case Function::Log10:
                    if (value <= 0.0) {
                        return QuietNaN();
                    }
                    return std::log10(value);
                case Function::Ln:
                    if (value <= 0.0) {
                        return QuietNaN();
                    }
                    return std::log(value);
                case Function::Exp:
                    return std::exp(value);
                case Function::Floor:
                    return std::floor(value);
                case Function::Ceil:
                    return std::ceil(value);
            }
            return QuietNaN();
        }
    }
    return QuietNaN();
}

// ---------------------------------------------------------------------------
// Root finding
// ---------------------------------------------------------------------------

constexpr std::size_t kMaxRoots = 100;
constexpr int kMaxBisectionIterations = 120;
constexpr double kSampleZeroTolerance = 1e-9;
constexpr double kRootResidualTolerance = 1e-7;
constexpr double kRootDedupRelativeTolerance = 1e-7;
constexpr double kRootXRelativeTolerance = 1e-12;

bool OppositeSigns(double a, double b) {
    return (a < 0.0 && b > 0.0) || (a > 0.0 && b < 0.0);
}

bool SameRootX(double a, double b) {
    const double scale = std::max({1.0, std::fabs(a), std::fabs(b)});
    return std::fabs(a - b) <= kRootDedupRelativeTolerance * scale;
}

void AppendRoot(std::vector<RootInfo>& roots, double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    for (RootInfo& root : roots) {
        if (SameRootX(root.x, x)) {
            if (std::fabs(y) < std::fabs(root.y)) {
                root = RootInfo{x, y};
            }
            return;
        }
    }
    if (roots.size() >= kMaxRoots) {
        return;
    }
    roots.push_back(RootInfo{x, y});
}

bool RefineSignChange(const Expression& expression, double left, double leftValue,
                      double right, double rightValue, RootInfo* result) {
    if (!std::isfinite(left) || !std::isfinite(right) || !std::isfinite(leftValue) ||
        !std::isfinite(rightValue) || !OppositeSigns(leftValue, rightValue)) {
        return false;
    }

    double bestX = left;
    double bestY = leftValue;
    auto consider = [&](double x, double y) {
        if (std::fabs(y) < std::fabs(bestY)) {
            bestX = x;
            bestY = y;
        }
    };
    consider(right, rightValue);

    for (int iteration = 0; iteration < kMaxBisectionIterations; ++iteration) {
        const double scale = std::max({1.0, std::fabs(left), std::fabs(right)});
        if (std::fabs(right - left) <= kRootXRelativeTolerance * scale) {
            break;
        }

        const double middle = left + (right - left) * 0.5;
        if (middle == left || middle == right) {
            break;
        }

        const double middleValue = expression.Evaluate(middle);
        if (!std::isfinite(middleValue)) {
            // A domain hole or a pole inside the interval: this is not a root.
            return false;
        }
        consider(middle, middleValue);

        if (middleValue == 0.0) {
            break;
        }
        if (OppositeSigns(leftValue, middleValue)) {
            right = middle;
            rightValue = middleValue;
        } else if (OppositeSigns(middleValue, rightValue)) {
            left = middle;
            leftValue = middleValue;
        } else {
            return false;
        }
    }

    if (std::fabs(bestY) > kRootResidualTolerance) {
        return false;
    }
    *result = RootInfo{bestX, bestY};
    return true;
}

bool RefineDomainBoundary(const Expression& expression, double finiteX, double finiteValue,
                          double invalidX, RootInfo* result) {
    if (!std::isfinite(finiteX) || !std::isfinite(finiteValue) ||
        !std::isfinite(invalidX)) {
        return false;
    }

    for (int iteration = 0; iteration < kMaxBisectionIterations; ++iteration) {
        // Keep walking towards the edge until the two doubles are adjacent (or
        // the iteration budget is exhausted). Stopping while the finite value
        // is still sqrt(1e-12) away would reject a legitimate boundary root.
        const double middle = finiteX + (invalidX - finiteX) * 0.5;
        if (middle == finiteX || middle == invalidX) {
            break;
        }

        const double middleValue = expression.Evaluate(middle);
        if (std::isfinite(middleValue)) {
            finiteX = middle;
            finiteValue = middleValue;
        } else {
            invalidX = middle;
        }
    }

    if (!std::isfinite(finiteValue) || std::fabs(finiteValue) > kRootResidualTolerance) {
        return false;
    }
    *result = RootInfo{finiteX, finiteValue};
    return true;
}

}  // namespace

struct Expression::Impl {
    NodePtr root;
};

bool Expression::Parse(const std::string& utf8Text, std::string* error) {
    text_ = utf8Text;
    impl_.reset();
    ok_ = false;
    if (error != nullptr) {
        error->clear();
    }

    std::vector<Token> rawTokens;
    std::string lexError;
    if (!Tokenize(text_, rawTokens, &lexError)) {
        if (error != nullptr) {
            *error = std::move(lexError);
        }
        return false;
    }

    const std::vector<Token> tokens = InsertImplicitMultiplication(rawTokens);
    Parser parser(tokens);
    NodePtr root = parser.Parse();
    if (!root) {
        if (error != nullptr) {
            *error = parser.error();
        }
        return false;
    }

    auto implementation = std::make_shared<Impl>();
    implementation->root = std::move(root);
    impl_ = std::move(implementation);
    ok_ = true;
    return true;
}

double Expression::Evaluate(double x) const {
    if (!ok_ || !impl_ || !impl_->root) {
        return QuietNaN();
    }
    return EvaluateNode(*impl_->root, x);
}

std::vector<RootInfo> FindRoots(const Expression& expression, double xMin, double xMax,
                                int samples) {
    std::vector<RootInfo> roots;
    if (!expression.ok() || !std::isfinite(xMin) || !std::isfinite(xMax) || xMin > xMax) {
        return roots;
    }

    if (xMin == xMax) {
        const double y = expression.Evaluate(xMin);
        if (std::isfinite(y) && std::fabs(y) <= kSampleZeroTolerance) {
            roots.push_back(RootInfo{xMin, y});
        }
        return roots;
    }

    const double span = xMax - xMin;
    if (!std::isfinite(span) || span <= 0.0) {
        return roots;
    }

    // Avoid pathological allocations from an accidentally enormous sample count.
    samples = std::min(std::max(samples, 2), 1000000);
    const std::size_t pointCount = static_cast<std::size_t>(samples) + 1;
    std::vector<double> xs(pointCount);
    std::vector<double> ys(pointCount);

    for (int i = 0; i <= samples; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        if (i == 0) {
            xs[index] = xMin;
        } else if (i == samples) {
            xs[index] = xMax;
        } else {
            xs[index] = xMin + span * (static_cast<double>(i) / static_cast<double>(samples));
        }
        ys[index] = expression.Evaluate(xs[index]);
    }

    // Exact and near-exact zeros at the sample points.
    for (std::size_t i = 0; i < pointCount; ++i) {
        if (std::isfinite(ys[i]) && std::fabs(ys[i]) <= kSampleZeroTolerance) {
            AppendRoot(roots, xs[i], ys[i]);
        }
    }

    for (int i = 0; i < samples; ++i) {
        const std::size_t left = static_cast<std::size_t>(i);
        const std::size_t right = left + 1;
        const bool leftFinite = std::isfinite(ys[left]);
        const bool rightFinite = std::isfinite(ys[right]);

        if (leftFinite && rightFinite) {
            if (OppositeSigns(ys[left], ys[right])) {
                RootInfo root;
                if (RefineSignChange(expression, xs[left], ys[left], xs[right], ys[right],
                                     &root)) {
                    AppendRoot(roots, root.x, root.y);
                }
            }
        } else if (leftFinite != rightFinite) {
            // A function such as sqrt(x-1) can touch zero exactly at the edge
            // of its domain, where no sign change is available.
            const double finiteX = leftFinite ? xs[left] : xs[right];
            const double finiteValue = leftFinite ? ys[left] : ys[right];
            const double invalidX = leftFinite ? xs[right] : xs[left];
            RootInfo root;
            if (RefineDomainBoundary(expression, finiteX, finiteValue, invalidX, &root)) {
                AppendRoot(roots, root.x, root.y);
            }
        }
    }

    std::sort(roots.begin(), roots.end(),
              [](const RootInfo& a, const RootInfo& b) { return a.x < b.x; });
    return roots;
}

}  // namespace mathexpr
