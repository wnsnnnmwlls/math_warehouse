import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:share_plus/share_plus.dart';

import '../ai/ai_client.dart';
import '../ai/ai_config.dart';
import '../ai/ai_settings_page.dart';
import '../data/question_store.dart';
import '../models/question.dart';
import 'manual_paper_page.dart';
import 'paper_docx.dart';
import 'paper_model.dart';

class PaperPage extends StatefulWidget {
  const PaperPage({super.key, required this.store});
  final QuestionStore store;

  @override
  State<PaperPage> createState() => _PaperPageState();
}

class _PaperPageState extends State<PaperPage> {
  final _weakness = TextEditingController();
  final _title = TextEditingController(text: '高中数学标准模拟卷');
  final _minutesController = TextEditingController(text: '120');
  final _aiConfig = AiConfigStore();
  final _scroll = ScrollController();
  final _previewKey = GlobalKey();
  int _choiceCount = 12;
  int _fillCount = 4;
  int _solutionCount = 6;
  int _minutes = 120;
  bool _includeAnswers = true;
  bool _busy = false;
  bool _exporting = false;
  String? _status;
  bool _statusError = false;
  Paper? _paper;
  String? _shortage;
  int _skippedMissing = 0;

  @override
  void dispose() {
    _weakness.dispose();
    _title.dispose();
    _minutesController.dispose();
    _scroll.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(title: const Text('AI 组卷')),
      body: ListView(
        controller: _scroll,
        padding: const EdgeInsets.fromLTRB(16, 4, 16, 30),
        children: [
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('试卷信息', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w800)),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _title,
                    decoration: const InputDecoration(labelText: '试卷标题'),
                  ),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _weakness,
                    minLines: 3,
                    maxLines: 6,
                    decoration: const InputDecoration(
                      labelText: '薄弱点 / 组卷要求',
                      hintText: '例如：导数含参分类讨论容易漏情况，圆锥曲线联立后算不出来；想要标准 12+4+6 结构。',
                    ),
                  ),
                  const SizedBox(height: 14),
                  Row(
                    children: [
                      Expanded(child: _counter('选择题', _choiceCount, 0, 20, (v) => setState(() => _choiceCount = v))),
                      const SizedBox(width: 8),
                      Expanded(child: _counter('填空题', _fillCount, 0, 12, (v) => setState(() => _fillCount = v))),
                      const SizedBox(width: 8),
                      Expanded(child: _counter('解答题', _solutionCount, 0, 10, (v) => setState(() => _solutionCount = v))),
                    ],
                  ),
                  const SizedBox(height: 10),
                  Row(
                    children: [
                      Expanded(
                        child: TextField(
                          keyboardType: TextInputType.number,
                          controller: _minutesController,
                          decoration: const InputDecoration(labelText: '考试时间（分钟）'),
                          onChanged: (value) {
                            final parsed = int.tryParse(value);
                            if (parsed != null) _minutes = parsed.clamp(20, 240);
                          },
                        ),
                      ),
                      const SizedBox(width: 10),
                      Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Text('附答案解析', style: TextStyle(fontSize: 11)),
                          Switch(
                            value: _includeAnswers,
                            onChanged: (v) => setState(() => _includeAnswers = v),
                          ),
                        ],
                      ),
                    ],
                  ),
                  const SizedBox(height: 6),
                  Text(
                    '标准结构：选择题 12 × 5 分，填空题 4 × 5 分，解答题 6 题共 70 分，总分 150。'
                    '题库不足时按实际题量生成，不编造题目。',
                    style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant, height: 1.45),
                  ),
                ],
              ),
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
                  label: Text(_busy ? 'AI 组卷中…' : 'AI 生成试卷'),
                ),
              ),
              const SizedBox(width: 10),
              OutlinedButton(
                onPressed: _busy ? null : _generateLocally,
                child: const Text('题库自动组卷'),
              ),
            ],
          ),
          const SizedBox(height: 6),
          Text(
            '题库自动组卷：不调用 AI、不需要 Key，直接从题库按上面的题型数量自动选题。',
            style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant),
          ),
          Align(
            alignment: Alignment.centerRight,
            child: TextButton.icon(
              onPressed: () => Navigator.of(context).push(
                MaterialPageRoute(builder: (_) => ManualPaperPage(store: widget.store)),
              ),
              icon: const Icon(Icons.playlist_add, size: 18),
              label: const Text('手动逐题组卷'),
            ),
          ),
          if (_status != null) ...[
            const SizedBox(height: 10),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: _statusError ? scheme.errorContainer : scheme.primaryContainer,
                borderRadius: BorderRadius.circular(10),
              ),
              child: Text(
                _status!,
                style: TextStyle(
                  fontSize: 12,
                  color: _statusError ? scheme.onErrorContainer : scheme.onPrimaryContainer,
                  height: 1.45,
                ),
              ),
            ),
          ],
          if (_shortage != null) ...[
            const SizedBox(height: 10),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: scheme.tertiaryContainer,
                borderRadius: BorderRadius.circular(10),
              ),
              child: Text(
                _shortage!,
                style: TextStyle(fontSize: 12, color: scheme.onTertiaryContainer),
              ),
            ),
          ],
          if (_paper != null) ...[
            const SizedBox(height: 18),
            KeyedSubtree(key: _previewKey, child: _paperPreview(context, _paper!)),
            const SizedBox(height: 14),
            FilledButton.icon(
              onPressed: _exporting ? null : _export,
              icon: _exporting
                  ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2))
                  : const Icon(Icons.ios_share),
              label: Text(_exporting ? '正在生成…' : '导出 Word 试卷并分享'),
            ),
          ],
          const SizedBox(height: 14),
          Card(
            child: ListTile(
              leading: const Icon(Icons.settings_suggest_outlined),
              title: const Text('AI 供应商设置'),
              subtitle: const Text('未配置或连接失败时，会自动使用本地题库组卷'),
              trailing: const Icon(Icons.chevron_right),
              onTap: () => Navigator.of(context).push(
                MaterialPageRoute(builder: (_) => const AiSettingsPage()),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _counter(String label, int value, int min, int max, ValueChanged<int> onChanged) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(11),
      ),
      child: Column(
        children: [
          Text(label, style: const TextStyle(fontSize: 10)),
          const SizedBox(height: 2),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              _stepIcon(
                Icons.remove_circle_outline,
                value > min ? () => onChanged(value - 1) : null,
              ),
              Text('$value', style: const TextStyle(fontSize: 17, fontWeight: FontWeight.w800)),
              _stepIcon(
                Icons.add_circle_outline,
                value < max ? () => onChanged(value + 1) : null,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _stepIcon(IconData icon, VoidCallback? onTap) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(18),
      child: Padding(
        padding: const EdgeInsets.all(2),
        child: Icon(
          icon,
          size: 18,
          color: onTap == null
              ? Theme.of(context).colorScheme.outlineVariant
              : Theme.of(context).colorScheme.primary,
        ),
      ),
    );
  }

  Widget _paperPreview(BuildContext context, Paper paper) {
    final scheme = Theme.of(context).colorScheme;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(paper.title, style: const TextStyle(fontSize: 17, fontWeight: FontWeight.w800)),
            const SizedBox(height: 4),
            Text(
              '${paper.questionCount} 题 · ${paper.minutes} 分钟 · 满分 ${paper.totalPoints} 分',
              style: TextStyle(fontSize: 12, color: scheme.onSurfaceVariant),
            ),
            const SizedBox(height: 12),
            for (final section in paper.sections) ...[
              Padding(
                padding: const EdgeInsets.only(top: 6, bottom: 6),
                child: Row(
                  children: [
                    Expanded(
                      child: Text(
                        '${section.title}  ${section.questions.length} 题 / ${section.totalPoints} 分',
                        style: const TextStyle(fontSize: 13, fontWeight: FontWeight.w800),
                      ),
                    ),
                  ],
                ),
              ),
              for (var i = 0; i < section.questions.length; i++)
                Padding(
                  padding: const EdgeInsets.only(bottom: 8, left: 4),
                  child: Text(
                    '${i + 1}. ${section.questions[i].question.stem}',
                    maxLines: 2,
                    overflow: TextOverflow.ellipsis,
                    style: const TextStyle(fontSize: 12.5, height: 1.4),
                  ),
                ),
              const Divider(height: 18),
            ],
          ],
        ),
      ),
    );
  }

  Future<void> _generate() async {
    final total = _choiceCount + _fillCount + _solutionCount;
    if (total == 0) {
      setState(() {
        _status = '至少选择一种题型。';
        _statusError = true;
      });
      return;
    }
    setState(() {
      _busy = true;
      _status = null;
      _shortage = null;
      _paper = null;
    });
    final config = await _aiConfig.load();
    if (config.baseUrl.trim().isEmpty || config.model.trim().isEmpty) {
      await _buildLocalPaper('未配置 AI 供应商，已使用本地题库组卷。');
      return;
    }
    final usable = widget.store.questions.where((q) => !q.needsFigure).toList();
    _skippedMissing = widget.store.questions.length - usable.length;
    final catalog = StringBuffer();
    for (final q in usable) {
      catalog.write('[${q.id}] ${_typeKey(q.type)} ${q.difficulty} ${q.gradeLabel} ');
      if (q.province.isNotEmpty) catalog.write('${q.province} ');
      final tags = [...q.formulas, ...q.concepts].take(3).join('、');
      if (tags.isNotEmpty) catalog.write('| $tags ');
      final stem = q.stem.replaceAll('\n', ' ').trim();
      catalog.writeln('| ${stem.length > 110 ? '${stem.substring(0, 110)}…' : stem}');
    }
    final prompt = '''
你是一名高中数学教研员。请根据学生的薄弱点和题库目录，组一份标准高中数学试卷。

目标题量：
- 选择题：$_choiceCount 题
- 填空题：$_fillCount 题
- 解答题：$_solutionCount 题

要求：
1. 只能使用目录中真实存在的 id，不要编造 id。
2. 优先覆盖学生薄弱点，并保持 A/B/C 难度梯度。
3. 某一题型题库不足时，能选多少选多少，不要为了凑数编题。
4. 只输出 JSON，不要 Markdown：
{"title":"试卷标题","minutes":120,"ids":["id1","id2"],"note":"组卷说明"}
5. ids 按“选择题 → 填空题 → 解答题”排列。

学生薄弱点/要求：
${_weakness.text.trim().isEmpty ? '按标准高中数学试卷组卷' : _weakness.text.trim()}

题库目录（id 题型 难度 年级 省份 | 标签 | 题干）：
${catalog.toString()}
''';
    final reply = await askAi(
      config: config,
      systemPrompt: '你是一名高中数学教研员，只返回用户要求的 JSON。',
      userPrompt: prompt,
      maxTokens: 1800,
      timeout: const Duration(seconds: 120),
    );
    if (!reply.ok) {
      await _buildLocalPaper('AI 组卷失败：${reply.error}\n已自动使用本地题库组卷。', isError: true);
      return;
    }
    final parsed = _parseReply(reply.text);
    if (parsed == null) {
      await _buildLocalPaper('AI 返回格式无法解析，已自动使用本地题库组卷。', isError: true);
      return;
    }
    final title = (parsed['title']?.toString().trim().isNotEmpty ?? false)
        ? parsed['title'].toString().trim()
        : _title.text.trim();
    final minutes = int.tryParse(parsed['minutes']?.toString() ?? '') ?? _minutes;
    final ids = (parsed['ids'] as List<dynamic>? ?? []).map((e) => e.toString()).toList();
    final selected = _selectQuestions(ids);
    await _finishPaper(
      title: title.isEmpty ? '高中数学标准模拟卷' : title,
      minutes: minutes,
      selected: selected,
      source: 'AI 组卷完成',
      note: parsed['note']?.toString() ?? '',
    );
  }

  Future<void> _generateLocally() async {
    setState(() {
      _paper = null;
      _status = null;
      _shortage = null;
      _skippedMissing = widget.store.questions.where((q) => q.needsFigure).length;
    });
    await _buildLocalPaper('题库自动组卷完成');
  }

  Future<void> _buildLocalPaper(String message, {bool isError = false}) async {
    final selected = _selectQuestions(const []);
    await _finishPaper(
      title: _title.text.trim().isEmpty ? '高中数学标准模拟卷' : _title.text.trim(),
      minutes: _minutes,
      selected: selected,
      source: message,
      note: '',
      isError: isError,
    );
  }

  Map<String, dynamic>? _parseReply(String text) {
    var value = text.trim();
    if (value.startsWith('```')) {
      final first = value.indexOf('\n');
      if (first >= 0) value = value.substring(first + 1);
      final fence = value.lastIndexOf('```');
      if (fence >= 0) value = value.substring(0, fence);
    }
    final first = value.indexOf('{');
    final last = value.lastIndexOf('}');
    if (first < 0 || last <= first) return null;
    try {
      final decoded = jsonDecode(value.substring(first, last + 1));
      return decoded is Map<String, dynamic> ? decoded : null;
    } catch (_) {
      return null;
    }
  }

  List<Question> _selectQuestions(List<String> aiIds) {
    final usable = widget.store.questions.where((q) => !q.needsFigure).toList();
    final byId = {for (final q in usable) q.id: q};
    final used = <String>{};
    final result = <Question>[];
    void pickType(String type, int target, {bool useAi = true}) {
      if (target <= 0) return;
      var count = 0;
      if (useAi) {
        for (final id in aiIds) {
          if (count >= target) break;
          final q = byId[id];
          if (q == null || q.type != type || used.contains(q.id)) continue;
          used.add(q.id);
          result.add(q);
          count++;
        }
      }
      if (count >= target) return;
      final pool = usable
          .where((q) => q.type == type && !used.contains(q.id))
          .toList()
        ..sort((a, b) {
          final order = {'A': 0, 'B': 1, 'C': 2};
          return (order[a.difficulty] ?? 1).compareTo(order[b.difficulty] ?? 1);
        });
      for (final q in pool) {
        if (count >= target) break;
        used.add(q.id);
        result.add(q);
        count++;
      }
    }

    pickType('choice', _choiceCount);
    pickType('fill', _fillCount);
    pickType('solution', _solutionCount);
    return result;
  }

  Future<void> _finishPaper({
    required String title,
    required int minutes,
    required List<Question> selected,
    required String source,
    required String note,
    bool isError = false,
  }) async {
    final choice = selected.where((q) => q.type == 'choice').toList();
    final fill = selected.where((q) => q.type == 'fill').toList();
    final solution = selected.where((q) => q.type == 'solution').toList();
    final sections = buildPaperSections(choice: choice, fill: fill, solution: solution);
    final paper = Paper(
      title: title,
      minutes: minutes.clamp(20, 240),
      sections: sections,
      note: note,
    );
    final missing = <String>[];
    if (choice.length < _choiceCount) missing.add('选择题 ${choice.length}/$_choiceCount');
    if (fill.length < _fillCount) missing.add('填空题 ${fill.length}/$_fillCount');
    if (solution.length < _solutionCount) missing.add('解答题 ${solution.length}/$_solutionCount');
    if (!mounted) return;
    setState(() {
      _busy = false;
      _paper = paper;
      _status = '$source：已生成 ${paper.questionCount} 题 · ${paper.totalPoints} 分。'
          '${_skippedMissing > 0 ? ' 已跳过 $_skippedMissing 道缺图题。' : ''}';
      _statusError = isError;
      _shortage = missing.isEmpty ? null : '题库数量不足：${missing.join('，')}。已按实际题量生成。';
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final target = _previewKey.currentContext;
      if (target != null) {
        Scrollable.ensureVisible(
          target,
          duration: const Duration(milliseconds: 350),
          curve: Curves.easeOutCubic,
          alignment: 0.04,
        );
      }
    });
  }

  Future<void> _export() async {
    final paper = _paper;
    if (paper == null) return;
    setState(() {
      _exporting = true;
      _status = null;
    });
    try {
      final file = await writePaperDocx(paper, includeAnswers: _includeAnswers);
      final ids = paper.sections
          .expand((section) => section.questions)
          .map((item) => item.question.id)
          .toSet();
      final marked = await widget.store.markExported(ids);
      await Share.shareXFiles(
        [XFile(file.path, mimeType: 'application/vnd.openxmlformats-officedocument.wordprocessingml.document')],
        subject: paper.title,
        text: '${paper.title} · ${paper.questionCount} 题 · ${paper.totalPoints} 分',
      );
      if (!mounted) return;
      setState(() {
        _exporting = false;
        _status = 'Word 试卷已生成，已把 $marked 道题标记为“已导出”并放入最近做过。';
        _statusError = false;
      });
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _exporting = false;
        _status = '导出失败：$error';
        _statusError = true;
      });
    }
  }

  String _typeKey(String type) => switch (type) {
        'fill' => '填空',
        'solution' => '解答',
        _ => '选择',
      };
}
