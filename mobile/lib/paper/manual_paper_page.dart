import 'package:flutter/material.dart';
import 'package:share_plus/share_plus.dart';

import '../data/question_store.dart';
import '../models/question.dart';
import 'paper_docx.dart';
import 'paper_model.dart';

class ManualPaperPage extends StatefulWidget {
  const ManualPaperPage({super.key, required this.store});
  final QuestionStore store;

  @override
  State<ManualPaperPage> createState() => _ManualPaperPageState();
}

class _ManualPaperPageState extends State<ManualPaperPage> {
  final _search = TextEditingController();
  final _title = TextEditingController(text: '手动组卷 · 高中数学');
  final List<String> _selectedIds = [];
  String _type = '';
  bool _exportedOnly = false;
  bool _missingOnly = false;

  @override
  void dispose() {
    _search.dispose();
    _title.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final query = _search.text.trim().toLowerCase();
    final questions = widget.store.questions.where((q) {
      if (_type.isNotEmpty && q.type != _type) return false;
      if (_exportedOnly && !q.exported) return false;
      if (_missingOnly && !q.needsFigure) return false;
      if (query.isEmpty) return true;
      final text = [q.stem, q.answer, q.analysis, ...q.formulas, ...q.concepts]
          .join('\n')
          .toLowerCase();
      return text.contains(query);
    }).toList();

    return Scaffold(
      appBar: AppBar(
        title: const Text('手动组卷'),
        actions: [
          if (_selectedIds.isNotEmpty)
            TextButton(
              onPressed: () => setState(_selectedIds.clear),
              child: const Text('清空'),
            ),
        ],
      ),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 4, 16, 8),
            child: Column(
              children: [
                TextField(
                  controller: _title,
                  decoration: const InputDecoration(labelText: '试卷标题'),
                ),
                const SizedBox(height: 8),
                TextField(
                  controller: _search,
                  onChanged: (_) => setState(() {}),
                  decoration: const InputDecoration(
                    hintText: '搜索题干、答案、知识点',
                    prefixIcon: Icon(Icons.search),
                  ),
                ),
              ],
            ),
          ),
          SizedBox(
            height: 38,
            child: ListView(
              scrollDirection: Axis.horizontal,
              padding: const EdgeInsets.symmetric(horizontal: 16),
              children: [
                _chip('全部', _type.isEmpty && !_exportedOnly && !_missingOnly, () {
                  setState(() {
                    _type = '';
                    _exportedOnly = false;
                    _missingOnly = false;
                  });
                }),
                _chip('选择题', _type == 'choice', () => setState(() => _type = _type == 'choice' ? '' : 'choice')),
                _chip('填空题', _type == 'fill', () => setState(() => _type = _type == 'fill' ? '' : 'fill')),
                _chip('解答题', _type == 'solution', () => setState(() => _type = _type == 'solution' ? '' : 'solution')),
                _chip('已导出', _exportedOnly, () => setState(() => _exportedOnly = !_exportedOnly)),
                _chip('缺图', _missingOnly, () => setState(() => _missingOnly = !_missingOnly)),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(18, 4, 18, 4),
            child: Row(
              children: [
                Text(
                  '${questions.length} 道题 · 已选 ${_selectedIds.length} 道',
                  style: TextStyle(
                    fontSize: 11,
                    color: Theme.of(context).colorScheme.onSurfaceVariant,
                  ),
                ),
              ],
            ),
          ),
          Expanded(
            child: questions.isEmpty
                ? const Center(child: Text('没有匹配的题目'))
                : ListView.builder(
                    padding: const EdgeInsets.fromLTRB(12, 4, 12, 100),
                    itemCount: questions.length,
                    itemBuilder: (context, index) {
                      final q = questions[index];
                      final selected = _selectedIds.contains(q.id);
                      return Card(
                        margin: const EdgeInsets.only(bottom: 8),
                        child: InkWell(
                          onTap: () => _toggle(q),
                          borderRadius: BorderRadius.circular(18),
                          child: Padding(
                            padding: const EdgeInsets.fromLTRB(8, 10, 12, 10),
                            child: Row(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Checkbox(
                                  value: selected,
                                  onChanged: (_) => _toggle(q),
                                ),
                                Expanded(
                                  child: Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      Row(
                                        children: [
                                          Text(
                                            q.typeLabel,
                                            style: TextStyle(
                                              fontSize: 10,
                                              fontWeight: FontWeight.w700,
                                              color: Theme.of(context).colorScheme.primary,
                                            ),
                                          ),
                                          const SizedBox(width: 8),
                                          Text(
                                            q.difficultyLabel,
                                            style: TextStyle(
                                              fontSize: 10,
                                              color: Theme.of(context).colorScheme.onSurfaceVariant,
                                            ),
                                          ),
                                          if (q.exported) ...[
                                            const SizedBox(width: 6),
                                            Text(
                                              '已导出',
                                              style: TextStyle(
                                                fontSize: 10,
                                                color: Theme.of(context).colorScheme.tertiary,
                                              ),
                                            ),
                                          ],
                                          if (q.needsFigure) ...[
                                            const SizedBox(width: 6),
                                            Text(
                                              '缺图',
                                              style: TextStyle(
                                                fontSize: 10,
                                                color: Theme.of(context).colorScheme.error,
                                              ),
                                            ),
                                          ],
                                        ],
                                      ),
                                      const SizedBox(height: 4),
                                      Text(
                                        q.stem,
                                        maxLines: 2,
                                        overflow: TextOverflow.ellipsis,
                                        style: const TextStyle(fontSize: 13, height: 1.4),
                                      ),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
      bottomNavigationBar: SafeArea(
        child: Padding(
          padding: const EdgeInsets.fromLTRB(16, 8, 16, 10),
          child: Row(
            children: [
              Expanded(
                child: Text(
                  _selectedIds.isEmpty ? '从上面勾选题目' : '已选 ${_selectedIds.length} 道题',
                  style: const TextStyle(fontSize: 12),
                ),
              ),
              FilledButton.icon(
                onPressed: _selectedIds.isEmpty ? null : _generate,
                icon: const Icon(Icons.article_outlined),
                label: const Text('生成试卷'),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _chip(String label, bool selected, VoidCallback onTap) {
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: ChoiceChip(
        label: Text(label),
        selected: selected,
        onSelected: (_) => onTap(),
        showCheckmark: false,
        labelStyle: const TextStyle(fontSize: 11),
      ),
    );
  }

  void _toggle(Question question) {
    setState(() {
      if (_selectedIds.contains(question.id)) {
        _selectedIds.remove(question.id);
      } else {
        _selectedIds.add(question.id);
      }
    });
  }

  void _generate() {
    final byId = {for (final q in widget.store.questions) q.id: q};
    final selected = _selectedIds.map((id) => byId[id]).whereType<Question>().toList();
    final choice = selected.where((q) => q.type == 'choice').toList();
    final fill = selected.where((q) => q.type == 'fill').toList();
    final solution = selected.where((q) => q.type == 'solution').toList();
    final sections = buildPaperSections(choice: choice, fill: fill, solution: solution);
    final paper = Paper(
      title: _title.text.trim().isEmpty ? '手动组卷 · 高中数学' : _title.text.trim(),
      minutes: 120,
      sections: sections,
      note: '手动选题组卷',
    );
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (_) => PaperResultPage(store: widget.store, paper: paper),
      ),
    );
  }
}

class PaperResultPage extends StatefulWidget {
  const PaperResultPage({super.key, required this.store, required this.paper});
  final QuestionStore store;
  final Paper paper;

  @override
  State<PaperResultPage> createState() => _PaperResultPageState();
}

class _PaperResultPageState extends State<PaperResultPage> {
  bool _includeAnswers = true;
  bool _exporting = false;
  String? _status;

  @override
  Widget build(BuildContext context) {
    final paper = widget.paper;
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(title: const Text('试卷预览')),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 30),
        children: [
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(paper.title, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w800)),
                  const SizedBox(height: 4),
                  Text(
                    '${paper.questionCount} 题 · ${paper.minutes} 分钟 · 满分 ${paper.totalPoints} 分',
                    style: TextStyle(fontSize: 12, color: scheme.onSurfaceVariant),
                  ),
                  if (paper.note.isNotEmpty) ...[
                    const SizedBox(height: 4),
                    Text(paper.note, style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant)),
                  ],
                ],
              ),
            ),
          ),
          const SizedBox(height: 10),
          for (final section in paper.sections) ...[
            Card(
              child: Padding(
                padding: const EdgeInsets.all(14),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      '${section.title} · ${section.questions.length} 题 / ${section.totalPoints} 分',
                      style: const TextStyle(fontSize: 14, fontWeight: FontWeight.w800),
                    ),
                    const SizedBox(height: 8),
                    for (var i = 0; i < section.questions.length; i++)
                      Padding(
                        padding: const EdgeInsets.only(bottom: 10),
                        child: Text(
                          '${i + 1}. ${section.questions[i].question.stem}',
                          maxLines: 3,
                          overflow: TextOverflow.ellipsis,
                          style: const TextStyle(fontSize: 12.5, height: 1.4),
                        ),
                      ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 8),
          ],
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            value: _includeAnswers,
            onChanged: (value) => setState(() => _includeAnswers = value),
            title: const Text('导出时附答案解析', style: TextStyle(fontSize: 13)),
          ),
          if (_status != null) ...[
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: scheme.primaryContainer,
                borderRadius: BorderRadius.circular(10),
              ),
              child: Text(
                _status!,
                style: TextStyle(fontSize: 12, color: scheme.onPrimaryContainer, height: 1.45),
              ),
            ),
          ],
          const SizedBox(height: 12),
          FilledButton.icon(
            onPressed: _exporting ? null : _export,
            icon: _exporting
                ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2))
                : const Icon(Icons.ios_share),
            label: Text(_exporting ? '正在生成…' : '导出 Word 试卷并分享'),
          ),
        ],
      ),
    );
  }

  Future<void> _export() async {
    setState(() {
      _exporting = true;
      _status = null;
    });
    try {
      final file = await writePaperDocx(widget.paper, includeAnswers: _includeAnswers);
      final ids = widget.paper.sections
          .expand((section) => section.questions)
          .map((item) => item.question.id)
          .toSet();
      final marked = await widget.store.markExported(ids);
      await Share.shareXFiles(
        [
          XFile(
            file.path,
            mimeType: 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
          )
        ],
        subject: widget.paper.title,
        text: '${widget.paper.title} · ${widget.paper.questionCount} 题 · ${widget.paper.totalPoints} 分',
      );
      if (!mounted) return;
      setState(() {
        _exporting = false;
        _status = 'Word 已生成，$marked 道题已标记为“已导出”并放入最近做过。';
      });
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _exporting = false;
        _status = '导出失败：$error';
      });
    }
  }
}
