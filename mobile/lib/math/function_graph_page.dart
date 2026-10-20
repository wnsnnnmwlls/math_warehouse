import 'dart:convert';

import 'package:flutter/material.dart';

import '../ai/ai_client.dart';
import '../ai/ai_config.dart';
import '../ai/ai_settings_page.dart';
import 'expression.dart';

class FunctionGraphPage extends StatefulWidget {
  const FunctionGraphPage({super.key});

  @override
  State<FunctionGraphPage> createState() => _FunctionGraphPageState();
}

class _FunctionGraphPageState extends State<FunctionGraphPage> {
  final _input = TextEditingController(text: 'sin(x)');
  final _aiConfig = AiConfigStore();
  MathExpression? _expression;
  String? _error;
  String? _note;
  bool _aiBusy = false;

  static const _presets = <String>['x', 'x^2-4', 'sin(x)', '1/x', 'abs(x)', 'sqrt(x+1)'];

  @override
  void initState() {
    super.initState();
    _draw();
  }

  @override
  void dispose() {
    _input.dispose();
    super.dispose();
  }

  void _draw() {
    try {
      final expression = MathExpression.parse(_input.text);
      setState(() {
        _expression = expression;
        _error = null;
      });
    } catch (error) {
      setState(() {
        _expression = null;
        _error = error.toString().replaceFirst('FormatException: ', '');
      });
    }
  }

  Future<void> _aiDraw() async {
    if (_aiBusy) return;
    if (_input.text.trim().isEmpty) {
      setState(() => _error = '请先输入函数描述，例如：开口向上的二次函数，顶点在 (1,-2)');
      return;
    }
    setState(() {
      _aiBusy = true;
      _error = null;
      _note = null;
    });
    try {
      final config = await _aiConfig.load();
      if (config.baseUrl.trim().isEmpty || config.model.trim().isEmpty) {
        throw Exception('请先在“AI 供应商配置”中填写 API 地址和模型');
      }
      final prompt = '''
把下面的自然语言函数描述转换成一条可绘制的单变量函数 y=f(x)。
只输出 JSON，不要 Markdown：
{"expression":"...","note":"一句中文说明"}
expression 只能使用变量 x、数字、+ - * / ^、括号，以及 sin cos tan asin acos atan sqrt abs log ln exp floor ceil 和 pi、e。
不要写 y= 或 f(x)=。
如果描述本身已经是表达式，就原样返回。
描述：
${_input.text.trim()}
''';
      final reply = await askAi(
        config: config,
        systemPrompt: '你是一名数学函数绘图助手，只负责把描述转成单变量函数表达式。',
        userPrompt: prompt,
        maxTokens: 500,
        timeout: const Duration(seconds: 90),
      );
      if (!reply.ok) throw Exception(reply.error);
      final decoded = _parseJson(reply.text);
      final expression = (decoded['expression'] ?? '').toString().trim();
      if (expression.isEmpty) throw Exception('AI 没有返回可用的函数表达式');
      _input.text = expression;
      _draw();
      if (mounted) {
        setState(() {
          _note = (decoded['note'] ?? '').toString().trim();
          if (_error != null) {
            _note = 'AI 返回的表达式无法解析：$_error';
          }
        });
      }
    } catch (error) {
      if (mounted) setState(() => _error = '$error');
    } finally {
      if (mounted) setState(() => _aiBusy = false);
    }
  }

  Map<String, dynamic> _parseJson(String text) {
    var value = text.trim();
    if (value.startsWith('```')) {
      final first = value.indexOf('\n');
      if (first >= 0) value = value.substring(first + 1);
      final lastFence = value.lastIndexOf('```');
      if (lastFence >= 0) value = value.substring(0, lastFence);
    }
    final first = value.indexOf('{');
    final last = value.lastIndexOf('}');
    if (first < 0 || last <= first) throw Exception('AI 返回内容不是 JSON');
    final decoded = jsonDecode(value.substring(first, last + 1));
    if (decoded is! Map<String, dynamic>) throw Exception('AI 返回的 JSON 格式不正确');
    return decoded;
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('函数图像'),
        actions: [
          IconButton(
            tooltip: 'AI 供应商配置',
            onPressed: () => Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const AiSettingsPage()),
            ),
            icon: const Icon(Icons.settings_suggest_outlined),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 30),
        children: [
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('函数表达式 / 中文描述', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w800)),
                  const SizedBox(height: 6),
                  Text(
                    '支持 y=x、f(x)=x，也支持只填 x 的右边；点 AI 画图可以用中文描述。',
                    style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant, height: 1.4),
                  ),
                  const SizedBox(height: 10),
                  TextField(
                    controller: _input,
                    minLines: 1,
                    maxLines: 3,
                    style: const TextStyle(fontSize: 16),
                    decoration: const InputDecoration(
                      hintText: '例如：x^2-4、sin(x)，或“开口向上的二次函数，顶点在 (1,-2)”',
                    ),
                    onSubmitted: (_) => _draw(),
                  ),
                  const SizedBox(height: 10),
                  Wrap(
                    spacing: 7,
                    runSpacing: 7,
                    children: [
                      for (final preset in _presets)
                        ActionChip(
                          label: Text(preset, style: const TextStyle(fontSize: 11)),
                          onPressed: () {
                            _input.text = preset;
                            _draw();
                          },
                        ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  Row(
                    children: [
                      Expanded(
                        child: FilledButton.icon(
                          onPressed: _draw,
                          icon: const Icon(Icons.show_chart),
                          label: const Text('绘制'),
                        ),
                      ),
                      const SizedBox(width: 10),
                      Expanded(
                        child: OutlinedButton.icon(
                          onPressed: _aiBusy ? null : _aiDraw,
                          icon: _aiBusy
                              ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2))
                              : const Icon(Icons.auto_awesome),
                          label: Text(_aiBusy ? 'AI 生成中…' : 'AI 画图'),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ),
          if (_error != null) ...[
            const SizedBox(height: 10),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(color: scheme.errorContainer, borderRadius: BorderRadius.circular(10)),
              child: Text(
                _error!,
                style: TextStyle(fontSize: 12, color: scheme.onErrorContainer, height: 1.4),
              ),
            ),
          ],
          if (_note != null && _note!.isNotEmpty) ...[
            const SizedBox(height: 10),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(color: scheme.primaryContainer, borderRadius: BorderRadius.circular(10)),
              child: Text(
                _note!,
                style: TextStyle(fontSize: 12, color: scheme.onPrimaryContainer, height: 1.4),
              ),
            ),
          ],
          const SizedBox(height: 14),
          Card(
            clipBehavior: Clip.antiAlias,
            child: AspectRatio(
              aspectRatio: 1.15,
              child: _expression == null
                  ? Center(
                      child: Text(
                        '输入表达式后点“绘制”',
                        style: TextStyle(fontSize: 13, color: scheme.onSurfaceVariant),
                      ),
                    )
                  : CustomPaint(
                      painter: FunctionPlotPainter(
                        expression: _expression!,
                        color: scheme.primary,
                        gridColor: scheme.outlineVariant,
                        textColor: scheme.onSurfaceVariant,
                      ),
                    ),
            ),
          ),
          const SizedBox(height: 10),
          Text(
            '绘制范围：x ∈ [-10, 10]。三角函数使用弧度；图像只作示意，不替代精确计算。',
            style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant),
          ),
        ],
      ),
    );
  }
}

class FunctionPlotPainter extends CustomPainter {
  FunctionPlotPainter({
    required this.expression,
    required this.color,
    required this.gridColor,
    required this.textColor,
  });

  final MathExpression expression;
  final Color color;
  final Color gridColor;
  final Color textColor;

  static const double xMin = -10;
  static const double xMax = 10;

  @override
  void paint(Canvas canvas, Size size) {
    const padding = 38.0;
    final plot = Rect.fromLTWH(padding, padding, size.width - padding * 2, size.height - padding * 2);
    final background = Paint()..color = Colors.white.withValues(alpha: 0.0);
    canvas.drawRect(plot, background);

    final samples = <Offset>[];
    final values = <double>[];
    for (var i = 0; i <= 900; i++) {
      final x = xMin + (xMax - xMin) * i / 900;
      final y = expression.evaluate(x);
      if (y.isFinite) {
        values.add(y);
        samples.add(Offset(x, y));
      }
    }
    if (values.isEmpty) {
      _text(canvas, '这个表达式在 [-10,10] 内没有有效点', plot.center, 13, textColor);
      return;
    }
    values.sort();
    var yMin = values[(values.length * 0.02).floor().clamp(0, values.length - 1)];
    var yMax = values[(values.length * 0.98).ceil().clamp(0, values.length - 1)];
    if ((yMax - yMin).abs() < 1e-6) {
      yMin -= 1;
      yMax += 1;
    }
    final span = yMax - yMin;
    yMin -= span * 0.08;
    yMax += span * 0.08;
    if ((yMax - yMin).abs() > 1e5) {
      yMin = -10;
      yMax = 10;
    }

    final gridPaint = Paint()
      ..color = gridColor
      ..strokeWidth = 1;
    for (var i = 0; i <= 10; i++) {
      final x = plot.left + plot.width * i / 10;
      canvas.drawLine(Offset(x, plot.top), Offset(x, plot.bottom), gridPaint);
      final y = plot.top + plot.height * i / 10;
      canvas.drawLine(Offset(plot.left, y), Offset(plot.right, y), gridPaint);
    }

    double sx(double x) => plot.left + (x - xMin) / (xMax - xMin) * plot.width;
    double sy(double y) => plot.bottom - (y - yMin) / (yMax - yMin) * plot.height;

    final axisPaint = Paint()
      ..color = textColor.withValues(alpha: 0.55)
      ..strokeWidth = 1.2;
    final xAxisY = sy(0).clamp(plot.top, plot.bottom).toDouble();
    final yAxisX = sx(0).clamp(plot.left, plot.right).toDouble();
    canvas.drawLine(Offset(plot.left, xAxisY), Offset(plot.right, xAxisY), axisPaint);
    canvas.drawLine(Offset(yAxisX, plot.top), Offset(yAxisX, plot.bottom), axisPaint);

    final path = Path();
    var started = false;
    for (final point in samples) {
      if (point.dy < yMin - (yMax - yMin) * 2 || point.dy > yMax + (yMax - yMin) * 2) {
        started = false;
        continue;
      }
      final p = Offset(sx(point.dx), sy(point.dy));
      if (!started) {
        path.moveTo(p.dx, p.dy);
        started = true;
      } else {
        path.lineTo(p.dx, p.dy);
      }
    }
    canvas.drawPath(
      path,
      Paint()
        ..color = color
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.2
        ..strokeCap = StrokeCap.round
        ..strokeJoin = StrokeJoin.round,
    );

    // roots
    final rootPaint = Paint()..color = color;
    for (var i = 1; i < samples.length; i++) {
      final a = samples[i - 1];
      final b = samples[i];
      if (a.dy == 0 || (a.dy < 0) != (b.dy < 0)) {
        final rootX = a.dx - a.dy * (b.dx - a.dx) / (b.dy - a.dy);
        if (rootX.isFinite && rootX >= xMin && rootX <= xMax) {
          canvas.drawCircle(Offset(sx(rootX), sy(0)), 3.2, rootPaint);
        }
      }
    }

    _text(canvas, 'O', Offset(yAxisX - 12, xAxisY + 12), 11, textColor);
    _text(canvas, 'x', Offset(plot.right - 8, xAxisY - 18), 12, textColor);
    _text(canvas, 'y', Offset(yAxisX + 9, plot.top + 2), 12, textColor);
    _text(canvas, yMax.toStringAsFixed(1), Offset(plot.left - 26, plot.top - 3), 9, textColor);
    _text(canvas, yMin.toStringAsFixed(1), Offset(plot.left - 26, plot.bottom - 10), 9, textColor);
    _text(canvas, '-10', Offset(plot.left - 8, plot.bottom + 8), 9, textColor);
    _text(canvas, '10', Offset(plot.right - 8, plot.bottom + 8), 9, textColor);
  }

  void _text(Canvas canvas, String text, Offset center, double size, Color color) {
    final painter = TextPainter(
      text: TextSpan(text: text, style: TextStyle(fontSize: size, color: color)),
      textDirection: TextDirection.ltr,
    )..layout();
    painter.paint(canvas, center - Offset(painter.width / 2, painter.height / 2));
  }

  @override
  bool shouldRepaint(covariant FunctionPlotPainter oldDelegate) =>
      oldDelegate.expression != expression || oldDelegate.color != color;
}
