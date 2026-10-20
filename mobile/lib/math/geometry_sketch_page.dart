import 'dart:convert';
import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../ai/ai_client.dart';
import '../ai/ai_config.dart';
import '../ai/ai_settings_page.dart';

class GeometryPoint {
  GeometryPoint({
    required this.id,
    required this.label,
    required this.x,
    required this.y,
    required this.z,
  });
  final String id;
  final String label;
  final double x;
  final double y;
  final double z;
}

class GeometrySegment {
  GeometrySegment({required this.a, required this.b, this.hidden = false});
  final String a;
  final String b;
  final bool hidden;
}

class GeometryCircle {
  GeometryCircle({required this.x, required this.y, required this.r});
  final double x;
  final double y;
  final double r;
}

class GeometrySpec {
  GeometrySpec({
    required this.kind,
    required this.title,
    required this.note,
    required this.points,
    required this.segments,
    required this.faces,
    required this.circles,
  });
  final String kind;
  final String title;
  final String note;
  final List<GeometryPoint> points;
  final List<GeometrySegment> segments;
  final List<List<String>> faces;
  final List<GeometryCircle> circles;

  bool get isSolid => kind == 'solid';

  static GeometrySpec sampleTetrahedron() {
    return GeometrySpec(
      kind: 'solid',
      title: '三棱锥 P-ABC（示例）',
      note: '底面 A-B-C 向后展开，PA、PB、PC 为侧棱；被底面遮住的 BC、CA 用虚线表示。',
      points: [
        GeometryPoint(id: 'A', label: 'A', x: 0.18, y: 0.22, z: 0.18),
        GeometryPoint(id: 'B', label: 'B', x: 0.80, y: 0.22, z: 0.18),
        GeometryPoint(id: 'C', label: 'C', x: 0.42, y: 0.24, z: 0.86),
        GeometryPoint(id: 'P', label: 'P', x: 0.52, y: 0.90, z: 0.12),
      ],
      segments: [
        GeometrySegment(a: 'A', b: 'B'),
        GeometrySegment(a: 'B', b: 'C', hidden: true),
        GeometrySegment(a: 'C', b: 'A', hidden: true),
        GeometrySegment(a: 'P', b: 'A'),
        GeometrySegment(a: 'P', b: 'B'),
        GeometrySegment(a: 'P', b: 'C'),
      ],
      faces: [
        ['A', 'B', 'C'],
        ['P', 'A', 'B'],
        ['P', 'B', 'C'],
        ['P', 'C', 'A'],
      ],
      circles: [],
    );
  }
}

class GeometrySketchPage extends StatefulWidget {
  const GeometrySketchPage({super.key});

  @override
  State<GeometrySketchPage> createState() => _GeometrySketchPageState();
}

class _GeometrySketchPageState extends State<GeometrySketchPage> {
  final _input = TextEditingController(
    text: '三棱锥 P-ABC 中，PA⊥平面 ABC，AB⊥AC，PA=AB=AC=2。',
  );
  final _aiConfig = AiConfigStore();
  GeometrySpec? _spec;
  String? _error;
  bool _busy = false;

  @override
  void dispose() {
    _input.dispose();
    super.dispose();
  }

  Future<void> _generate() async {
    if (_busy) return;
    if (_input.text.trim().isEmpty) {
      setState(() => _error = '请先输入几何题描述');
      return;
    }
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      final config = await _aiConfig.load();
      if (config.baseUrl.trim().isEmpty || config.model.trim().isEmpty) {
        throw Exception('请先在 AI 供应商配置中填写 API 地址和模型');
      }
      final prompt = '''
几何题描述：
${_input.text.trim()}

请生成一个便于板书画图的平面或立体几何示意图。只输出 JSON：
{"kind":"solid","title":"三棱锥 P-ABC","points":[{"id":"A","label":"A","x":0.18,"y":0.28,"z":0.20}],"segments":[{"a":"A","b":"B","hidden":false}],"faces":[["A","B","C"]],"circles":[],"note":"关键辅助线说明"}
所有 x/y/z 使用 0 到 1 归一化坐标：x 向右，y 向上，z 为深度（0 靠前，1 靠后）。立体题必须给每个点 z；被遮挡的棱把 hidden 设为 true；faces 用点 id 表示表面。点 id 必须与线段、faces 引用一致。平面题 kind 用 plane，z 可省略。不要输出 Markdown。
''';
      final reply = await askAi(
        config: config,
        systemPrompt: '你是一名数学教师，负责把几何题转成简洁的示意图数据。只输出 JSON。',
        userPrompt: prompt,
        maxTokens: 1800,
        timeout: const Duration(seconds: 120),
      );
      if (!reply.ok) throw Exception(reply.error);
      final spec = _parse(reply.text);
      if (!mounted) return;
      setState(() => _spec = spec);
    } catch (error) {
      if (mounted) setState(() => _error = '$error');
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  GeometrySpec _parse(String text) {
    var value = text.trim();
    final first = value.indexOf('{');
    final last = value.lastIndexOf('}');
    if (first < 0 || last <= first) throw Exception('AI 返回内容不是 JSON');
    final decoded = jsonDecode(value.substring(first, last + 1));
    if (decoded is! Map<String, dynamic>) throw Exception('AI 返回的 JSON 格式不正确');
    final points = <GeometryPoint>[];
    for (final item in (decoded['points'] as List<dynamic>? ?? const [])) {
      if (item is! Map) continue;
      final id = (item['id'] ?? '').toString();
      if (id.isEmpty) continue;
      points.add(GeometryPoint(
        id: id,
        label: (item['label'] ?? id).toString(),
        x: _number(item['x'], 0.5),
        y: _number(item['y'], 0.5),
        z: _number(item['z'], 0.5),
      ));
    }
    if (points.isEmpty) throw Exception('AI 没有返回可绘制的几何点');
    final segments = <GeometrySegment>[];
    for (final item in (decoded['segments'] as List<dynamic>? ?? const [])) {
      if (item is! Map) continue;
      final a = (item['a'] ?? '').toString();
      final b = (item['b'] ?? '').toString();
      if (a.isEmpty || b.isEmpty) continue;
      segments.add(GeometrySegment(a: a, b: b, hidden: item['hidden'] == true));
    }
    final faces = <List<String>>[];
    for (final item in (decoded['faces'] as List<dynamic>? ?? const [])) {
      if (item is! List) continue;
      final face = item.map((e) => e.toString()).where((e) => e.isNotEmpty).toList();
      if (face.length >= 3) faces.add(face);
    }
    final circles = <GeometryCircle>[];
    for (final item in (decoded['circles'] as List<dynamic>? ?? const [])) {
      if (item is! Map) continue;
      circles.add(GeometryCircle(
        x: _number(item['x'], 0.5),
        y: _number(item['y'], 0.5),
        r: _number(item['r'], 0.2),
      ));
    }
    return GeometrySpec(
      kind: (decoded['kind'] ?? 'plane').toString(),
      title: (decoded['title'] ?? '几何示意图').toString(),
      note: (decoded['note'] ?? '').toString(),
      points: points,
      segments: segments,
      faces: faces,
      circles: circles,
    );
  }

  double _number(dynamic value, double fallback) {
    if (value is num) return value.toDouble();
    return double.tryParse('$value') ?? fallback;
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('几何示意图'),
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
                  const Text('几何题描述 / 已知条件', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w800)),
                  const SizedBox(height: 6),
                  Text(
                    '支持平面和立体题；立体题会返回 x/y/z、隐藏棱和表面，按斜二测投影绘制。',
                    style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant, height: 1.4),
                  ),
                  const SizedBox(height: 10),
                  TextField(
                    controller: _input,
                    minLines: 3,
                    maxLines: 6,
                    decoration: const InputDecoration(
                      hintText: '例如：三棱锥 P-ABC 中，PA⊥平面 ABC，AB⊥AC，PA=AB=AC=2。',
                    ),
                  ),
                  const SizedBox(height: 12),
                  Row(
                    children: [
                      Expanded(
                        child: FilledButton.icon(
                          onPressed: _busy ? null : _generate,
                          icon: _busy
                              ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2))
                              : const Icon(Icons.auto_awesome),
                          label: Text(_busy ? 'AI 生成中…' : 'AI 生成示意图'),
                        ),
                      ),
                      const SizedBox(width: 10),
                      OutlinedButton(
                        onPressed: _busy
                            ? null
                            : () => setState(() {
                                  _spec = GeometrySpec.sampleTetrahedron();
                                  _error = null;
                                }),
                        child: const Text('示例'),
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
          const SizedBox(height: 14),
          Card(
            clipBehavior: Clip.antiAlias,
            child: Column(
              children: [
                Padding(
                  padding: const EdgeInsets.fromLTRB(16, 14, 16, 0),
                  child: Row(
                    children: [
                      Expanded(
                        child: Text(
                          _spec?.title ?? '示意图',
                          style: const TextStyle(fontSize: 15, fontWeight: FontWeight.w800),
                        ),
                      ),
                    ],
                  ),
                ),
                AspectRatio(
                  aspectRatio: 1.08,
                  child: _spec == null
                      ? Center(
                          child: Text(
                            '点击“AI 生成示意图”或“示例”',
                            style: TextStyle(fontSize: 13, color: scheme.onSurfaceVariant),
                          ),
                        )
                      : CustomPaint(
                          painter: GeometrySketchPainter(
                            spec: _spec!,
                            lineColor: scheme.onSurface,
                            faceColor: scheme.primary,
                            textColor: scheme.onSurfaceVariant,
                          ),
                        ),
                ),
                if (_spec != null && _spec!.note.isNotEmpty)
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 0, 16, 14),
                    child: Text(
                      _spec!.note,
                      style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant, height: 1.45),
                    ),
                  ),
              ],
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '斜二测投影仅作示意图，不保证严格透视比例；请核对 AI 返回的点、棱和面关系。',
            style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant),
          ),
        ],
      ),
    );
  }
}

class GeometrySketchPainter extends CustomPainter {
  GeometrySketchPainter({
    required this.spec,
    required this.lineColor,
    required this.faceColor,
    required this.textColor,
  });

  final GeometrySpec spec;
  final Color lineColor;
  final Color faceColor;
  final Color textColor;

  @override
  void paint(Canvas canvas, Size size) {
    final view = Rect.fromLTWH(24, 18, size.width - 48, size.height - 46);
    final points = <String, ({double x, double y, double z})>{};
    for (final point in spec.points) {
      points[point.id] = (x: point.x, y: point.y, z: point.z);
    }
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (final point in spec.points) {
      final depth = spec.isSolid ? point.z : 0.0;
      final x = point.x + depth * 0.38;
      final y = point.y + depth * 0.20;
      minX = math.min(minX, x);
      maxX = math.max(maxX, x);
      minY = math.min(minY, y);
      maxY = math.max(maxY, y);
    }
    final spanX = math.max(0.001, maxX - minX);
    final spanY = math.max(0.001, maxY - minY);
    final scale = math.min(view.width / spanX, view.height / spanY);
    final offsetX = view.left + (view.width - spanX * scale) / 2;
    final offsetY = view.top + (view.height - spanY * scale) / 2;
    double sx(double x) => offsetX + (x - minX) * scale;
    double sy(double y) => offsetY + (maxY - y) * scale;
    Offset project(GeometryPoint point) {
      final depth = spec.isSolid ? point.z : 0.0;
      return Offset(sx(point.x + depth * 0.38), sy(point.y + depth * 0.20));
    }

    final screen = <String, Offset>{};
    for (final point in spec.points) {
      screen[point.id] = project(point);
    }

    // Faces, back to front.
    final faces = <({List<Offset> points, double depth})>[];
    for (final face in spec.faces) {
      final ps = <Offset>[];
      var depth = 0.0;
      for (final id in face) {
        final p = screen[id];
        final raw = points[id];
        if (p == null || raw == null) continue;
        ps.add(p);
        depth += raw.z;
      }
      if (ps.length >= 3) faces.add((points: ps, depth: depth / ps.length));
    }
    faces.sort((a, b) => b.depth.compareTo(a.depth));
    for (final face in faces) {
      final path = Path()..addPolygon(face.points, true);
      final alpha = spec.isSolid ? (24 + (1 - face.depth.clamp(0, 1)) * 26).round() : 20;
      canvas.drawPath(path, Paint()..color = faceColor.withValues(alpha: alpha / 255));
    }

    // Hidden edges dashed, then visible edges.
    for (final segment in spec.segments) {
      final a = screen[segment.a];
      final b = screen[segment.b];
      if (a == null || b == null) continue;
      if (spec.isSolid && segment.hidden) {
        _dashed(canvas, a, b, textColor.withValues(alpha: 0.65));
      } else {
        canvas.drawLine(a, b, Paint()
          ..color = lineColor.withValues(alpha: 0.8)
          ..strokeWidth = 1.7
          ..strokeCap = StrokeCap.round);
      }
    }

    for (final circle in spec.circles) {
      final center = Offset(sx(circle.x), sy(circle.y));
      canvas.drawCircle(
        center,
        circle.r * math.min(view.width, view.height),
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.4
          ..color = faceColor,
      );
    }

    for (final point in spec.points) {
      final p = screen[point.id];
      if (p == null) continue;
      final back = spec.isSolid && point.z > 0.6;
      canvas.drawCircle(
        p,
        back ? 3 : 3.6,
        Paint()..color = back ? textColor.withValues(alpha: 0.55) : faceColor,
      );
      final text = TextPainter(
        text: TextSpan(
          text: point.label.isEmpty ? point.id : point.label,
          style: TextStyle(fontSize: 12, fontWeight: FontWeight.w700, color: lineColor),
        ),
        textDirection: TextDirection.ltr,
      )..layout();
      text.paint(canvas, p + const Offset(7, -18));
    }

    if (spec.isSolid) {
      final note = TextPainter(
        text: TextSpan(
          text: '斜二测投影 · 虚线为被遮挡棱',
          style: TextStyle(fontSize: 10, color: textColor),
        ),
        textDirection: TextDirection.ltr,
      )..layout();
      note.paint(canvas, Offset(view.left + 4, view.bottom + 10));
    }
  }

  void _dashed(Canvas canvas, Offset a, Offset b, Color color) {
    final delta = b - a;
    final length = delta.distance;
    if (length <= 0.1) return;
    final direction = delta / length;
    final paint = Paint()
      ..color = color
      ..strokeWidth = 1.4
      ..strokeCap = StrokeCap.round;
    for (double t = 0; t < length; t += 13) {
      final end = math.min(length, t + 7);
      canvas.drawLine(a + direction * t, a + direction * end, paint);
    }
  }

  @override
  bool shouldRepaint(covariant GeometrySketchPainter oldDelegate) => oldDelegate.spec != spec;
}
