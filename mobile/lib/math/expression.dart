import 'dart:math' as math;

/// A tiny single-variable expression parser used by the mobile function plotter.
///
/// Supports x, numbers, + - * / ^, parentheses, implicit multiplication,
/// sin/cos/tan/asin/acos/atan/sqrt/abs/log/ln/exp/floor/ceil, pi and e.
class MathExpression {
  MathExpression._(this.source, this._evaluate);

  final String source;
  final double Function(double x) _evaluate;

  double evaluate(double x) => _evaluate(x);

  static MathExpression parse(String raw) {
    final source = normalize(raw);
    if (source.isEmpty) throw const FormatException('请输入函数表达式');
    final parser = _Parser(_tokenize(source));
    final evaluation = parser.parse();
    return MathExpression._(source, evaluation);
  }

  static String normalize(String raw) {
    var text = raw.trim();
    const replacements = {
      '（': '(',
      '）': ')',
      '＝': '=',
      '＋': '+',
      '－': '-',
      '−': '-',
      '²': '^2',
      '³': '^3',
      '×': '*',
      '·': '*',
      '÷': '/',
      'π': 'pi',
    };
    replacements.forEach((key, value) {
      text = text.replaceAll(key, value);
    });
    text = text.trim();
    final lower = text.toLowerCase();
    if (lower.startsWith('y')) {
      var i = 1;
      while (i < text.length && text[i].trim().isEmpty) {
        i++;
      }
      if (i < text.length && text[i] == '=') {
        text = text.substring(i + 1).trim();
      }
    }
    text = text.trim();
    final functionMatch = RegExp(r'^f\s*\(\s*x\s*\)\s*=').firstMatch(text.toLowerCase());
    if (functionMatch != null) {
      text = text.substring(functionMatch.end).trim();
    }
    return text;
  }

  static List<_Token> _tokenize(String text) {
    final tokens = <_Token>[];
    var i = 0;
    while (i < text.length) {
      final c = text[i];
      if (c.trim().isEmpty) {
        i++;
        continue;
      }
      if (_isDigit(c) || c == '.') {
        final start = i;
        var dots = 0;
        while (i < text.length && (_isDigit(text[i]) || text[i] == '.')) {
          if (text[i] == '.') dots++;
          i++;
        }
        if (dots > 1) throw const FormatException('数字格式不正确');
        // Scientific notation: 2e3, 1.5e-2
        if (i < text.length && (text[i] == 'e' || text[i] == 'E')) {
          var probe = i + 1;
          if (probe < text.length && (text[probe] == '+' || text[probe] == '-')) {
            probe++;
          }
          if (probe < text.length && _isDigit(text[probe])) {
            i = probe;
            while (i < text.length && _isDigit(text[i])) {
              i++;
            }
          }
        }
        final value = double.tryParse(text.substring(start, i));
        if (value == null) throw const FormatException('数字格式不正确');
        tokens.add(_Token(_TokenType.number, value: value));
        continue;
      }
      if (_isLetter(c)) {
        final start = i;
        while (i < text.length && _isLetter(text[i])) {
          i++;
        }
        var name = text.substring(start, i).toLowerCase();
        // Longest known function/variable names; handles inputs such as xsin(x).
        while (name.isNotEmpty) {
          final known = _knownNames.firstWhere(
            (item) => name.startsWith(item),
            orElse: () => '',
          );
          if (known.isEmpty) throw FormatException('未知函数或变量“$name”');
          tokens.add(_Token(_TokenType.name, name: known));
          name = name.substring(known.length);
        }
        continue;
      }
      switch (c) {
        case '+':
          tokens.add(const _Token(_TokenType.plus));
          break;
        case '-':
          tokens.add(const _Token(_TokenType.minus));
          break;
        case '*':
          tokens.add(const _Token(_TokenType.multiply));
          break;
        case '/':
          tokens.add(const _Token(_TokenType.divide));
          break;
        case '^':
          tokens.add(const _Token(_TokenType.power));
          break;
        case '(':
          tokens.add(const _Token(_TokenType.leftParen));
          break;
        case ')':
          tokens.add(const _Token(_TokenType.rightParen));
          break;
        case '√':
          tokens.add(const _Token(_TokenType.root));
          break;
        default:
          throw FormatException('不支持的字符“$c”');
      }
      i++;
    }
    return _insertImplicitMultiplication(tokens);
  }

  static List<_Token> _insertImplicitMultiplication(List<_Token> input) {
    final output = <_Token>[];
    for (var i = 0; i < input.length; i++) {
      final token = input[i];
      if (output.isNotEmpty) {
        final previous = output.last;
        final previousEnds = previous.type == _TokenType.number ||
            previous.type == _TokenType.name ||
            previous.type == _TokenType.rightParen;
        final currentStarts = token.type == _TokenType.number ||
            token.type == _TokenType.name ||
            token.type == _TokenType.leftParen ||
            token.type == _TokenType.root;
        final functionCall = previous.type == _TokenType.name &&
            _functionNames.contains(previous.name) &&
            (token.type == _TokenType.leftParen ||
                token.type == _TokenType.name ||
                token.type == _TokenType.number ||
                token.type == _TokenType.root);
        if (previousEnds && currentStarts && !functionCall) {
          output.add(const _Token(_TokenType.multiply));
        }
      }
      output.add(token);
    }
    return output;
  }

  static bool _isDigit(String c) => c.codeUnitAt(0) >= 48 && c.codeUnitAt(0) <= 57;
  static bool _isLetter(String c) {
    final code = c.codeUnitAt(0);
    return (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
  }

  static const _knownNames = [
    'asin', 'acos', 'atan', 'sqrt', 'floor', 'ceil', 'sin', 'cos', 'tan',
    'abs', 'log', 'ln', 'exp', 'pi', 'e', 'x',
  ];

  static const _functionNames = {
    'asin', 'acos', 'atan', 'sqrt', 'floor', 'ceil', 'sin', 'cos', 'tan',
    'abs', 'log', 'ln', 'exp',
  };
}

enum _TokenType { number, name, plus, minus, multiply, divide, power, leftParen, rightParen, root }

class _Token {
  const _Token(this.type, {this.value = 0, this.name = ''});
  final _TokenType type;
  final double value;
  final String name;
}

class _Parser {
  _Parser(this.tokens);
  final List<_Token> tokens;
  int index = 0;

  _Token get current => index < tokens.length ? tokens[index] : const _Token(_TokenType.number);

  double Function(double) parse() {
    final result = _parseExpression();
    if (index < tokens.length) throw const FormatException('表达式末尾有多余内容');
    return result;
  }

  double Function(double) _parseExpression() {
    var left = _parseTerm();
    while (current.type == _TokenType.plus || current.type == _TokenType.minus) {
      final plus = current.type == _TokenType.plus;
      index++;
      final right = _parseTerm();
      final previous = left;
      left = (x) => plus ? previous(x) + right(x) : previous(x) - right(x);
    }
    return left;
  }

  double Function(double) _parseTerm() {
    var left = _parseUnary();
    while (current.type == _TokenType.multiply || current.type == _TokenType.divide) {
      final multiply = current.type == _TokenType.multiply;
      index++;
      final right = _parseUnary();
      final previous = left;
      left = (x) {
        final a = previous(x);
        final b = right(x);
        return multiply ? a * b : (b == 0 ? double.nan : a / b);
      };
    }
    return left;
  }

  double Function(double) _parseUnary() {
    if (current.type == _TokenType.minus) {
      index++;
      final value = _parseUnary();
      return (x) => -value(x);
    }
    if (current.type == _TokenType.plus) {
      index++;
      return _parseUnary();
    }
    return _parsePower();
  }

  double Function(double) _parsePower() {
    var base = _parsePrimary();
    if (current.type == _TokenType.power) {
      index++;
      final exponent = _parseUnary();
      final previous = base;
      base = (x) {
        final a = previous(x);
        final b = exponent(x);
        if (a == 0 && b < 0) return double.nan;
        return math.pow(a, b).toDouble();
      };
    }
    return base;
  }

  double Function(double) _parsePrimary() {
    final token = current;
    if (token.type == _TokenType.number) {
      index++;
      return (_) => token.value;
    }
    if (token.type == _TokenType.name) {
      index++;
      if (token.name == 'x') return (x) => x;
      if (token.name == 'pi') return (_) => math.pi;
      if (token.name == 'e') return (_) => math.e;
      final function = _functionFor(token.name);
      if (function == null) throw FormatException('未知函数“${token.name}”');
      if (current.type != _TokenType.leftParen) {
        // Allow sin x, but consume a unary expression.
        final argument = _parseUnary();
        return (x) => function(argument(x));
      }
      index++;
      final argument = _parseExpression();
      if (current.type != _TokenType.rightParen) {
        throw const FormatException('函数参数缺少右括号');
      }
      index++;
      return (x) => function(argument(x));
    }
    if (token.type == _TokenType.leftParen) {
      index++;
      final inner = _parseExpression();
      if (current.type != _TokenType.rightParen) {
        throw const FormatException('缺少右括号');
      }
      index++;
      return inner;
    }
    if (token.type == _TokenType.root) {
      index++;
      final inner = _parsePrimary();
      return (x) {
        final value = inner(x);
        return value < 0 ? double.nan : math.sqrt(value);
      };
    }
    throw const FormatException('表达式不完整');
  }

  double Function(double)? _functionFor(String name) {
    switch (name) {
      case 'sin':
        return math.sin;
      case 'cos':
        return math.cos;
      case 'tan':
        return math.tan;
      case 'asin':
        return math.asin;
      case 'acos':
        return math.acos;
      case 'atan':
        return math.atan;
      case 'sqrt':
        return (x) => x < 0 ? double.nan : math.sqrt(x);
      case 'abs':
        return (x) => x.abs();
      case 'log':
        return (x) => x <= 0 ? double.nan : math.log(x) / math.log(10);
      case 'ln':
        return (x) => x <= 0 ? double.nan : math.log(x);
      case 'exp':
        return math.exp;
      case 'floor':
        return (x) => x.floorToDouble();
      case 'ceil':
        return (x) => x.ceilToDouble();
    }
    return null;
  }
}
