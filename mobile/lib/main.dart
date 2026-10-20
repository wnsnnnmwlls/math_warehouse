import 'dart:convert';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'package:shared_preferences/shared_preferences.dart';

import 'ai/ai_settings_page.dart';
import 'data/question_store.dart';
import 'models/question.dart';
import 'paper/manual_paper_page.dart';
import 'paper/paper_page.dart';

Future<void> importQuestionsFromFile(BuildContext context, QuestionStore store) async {
  try {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['json'],
      withData: true,
    );
    if (result == null || result.files.isEmpty) return;
    final file = result.files.single;
    final bytes = file.bytes ??
        (file.path == null ? null : await File(file.path!).readAsBytes());
    if (bytes == null) {
      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('无法读取这个文件')));
      }
      return;
    }
    final added = await store.importJson(utf8.decode(bytes, allowMalformed: true));
    if (!context.mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(added > 0 ? '成功导入 $added 道题' : '没有新题，可能全部重复')),
    );
  } catch (error) {
    if (!context.mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('导入失败：$error')),
    );
  }
}

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Render the first Flutter frame immediately. Plugin initialization must not
  // block runApp(), otherwise some Android devices stay on the splash icon.
  runApp(MathPaperApp(store: QuestionStore()));
}

class MathPaperApp extends StatefulWidget {
  const MathPaperApp({super.key, required this.store});

  final QuestionStore store;

  @override
  State<MathPaperApp> createState() => _MathPaperAppState();
}

class _MathPaperAppState extends State<MathPaperApp> {
  ThemeMode _mode = ThemeMode.light;

  @override
  void initState() {
    super.initState();
    widget.store.load();
    _loadThemePreference();
  }

  Future<void> _loadThemePreference() async {
    try {
      final prefs = await SharedPreferences.getInstance();
      if (!mounted) return;
      setState(() {
        _mode = prefs.getBool('dark_mode') == true
            ? ThemeMode.dark
            : ThemeMode.light;
      });
    } catch (_) {
      // A failed preference read should not block the app; keep light mode.
    }
  }

  ThemeData _theme(Brightness brightness) {
    final scheme = ColorScheme.fromSeed(
      seedColor: const Color(0xFF5B4BE8),
      brightness: brightness,
    );
    return ThemeData(
      useMaterial3: true,
      colorScheme: scheme,
      scaffoldBackgroundColor:
          brightness == Brightness.dark ? const Color(0xFF0D1118) : const Color(0xFFF6F7FB),
      appBarTheme: AppBarTheme(
        backgroundColor:
            brightness == Brightness.dark ? const Color(0xFF0D1118) : const Color(0xFFF6F7FB),
        surfaceTintColor: Colors.transparent,
        elevation: 0,
        titleTextStyle: TextStyle(
          color: brightness == Brightness.dark ? Colors.white : const Color(0xFF17202E),
          fontSize: 22,
          fontWeight: FontWeight.w800,
        ),
      ),
      cardTheme: CardThemeData(
        color: brightness == Brightness.dark ? const Color(0xFF171D27) : Colors.white,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(18),
          side: BorderSide(
            color: brightness == Brightness.dark
                ? const Color(0xFF283142)
                : const Color(0xFFE5E8F0),
          ),
        ),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: brightness == Brightness.dark ? const Color(0xFF171D27) : Colors.white,
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: BorderSide.none,
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: BorderSide(
            color: brightness == Brightness.dark
                ? const Color(0xFF283142)
                : const Color(0xFFE5E8F0),
          ),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: BorderSide(color: scheme.primary, width: 1.5),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '数研组卷',
      debugShowCheckedModeBanner: false,
      theme: _theme(Brightness.light),
      darkTheme: _theme(Brightness.dark),
      themeMode: _mode,
      home: AnimatedBuilder(
        animation: widget.store,
        builder: (context, _) {
          if (!widget.store.loaded) {
            return const Scaffold(
              body: Center(child: CircularProgressIndicator()),
            );
          }
          return HomeShell(
            store: widget.store,
            darkMode: _mode == ThemeMode.dark,
            onThemeChanged: (dark) {
              setState(() => _mode = dark ? ThemeMode.dark : ThemeMode.light);
              SharedPreferences.getInstance()
                  .then((prefs) => prefs.setBool('dark_mode', dark))
                  .catchError((_) => false);
            },
          );
        },
      ),
    );
  }
}

class HomeShell extends StatefulWidget {
  const HomeShell({
    super.key,
    required this.store,
    required this.darkMode,
    required this.onThemeChanged,
  });

  final QuestionStore store;
  final bool darkMode;
  final ValueChanged<bool> onThemeChanged;

  @override
  State<HomeShell> createState() => _HomeShellState();
}

class _HomeShellState extends State<HomeShell> {
  int _index = 0;

  @override
  Widget build(BuildContext context) {
    final pages = [
      BankPage(store: widget.store),
      TagsPage(store: widget.store),
      PaperPage(store: widget.store),
      RecentPage(store: widget.store),
      SettingsPage(
        store: widget.store,
        darkMode: widget.darkMode,
        onThemeChanged: widget.onThemeChanged,
      ),
    ];
    return Scaffold(
      body: IndexedStack(index: _index, children: pages),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (value) => setState(() => _index = value),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.menu_book_outlined), selectedIcon: Icon(Icons.menu_book), label: '题库'),
          NavigationDestination(icon: Icon(Icons.sell_outlined), selectedIcon: Icon(Icons.sell), label: '标签'),
          NavigationDestination(icon: Icon(Icons.auto_awesome_outlined), selectedIcon: Icon(Icons.auto_awesome), label: 'AI 组卷'),
          NavigationDestination(icon: Icon(Icons.history_outlined), selectedIcon: Icon(Icons.history), label: '最近'),
          NavigationDestination(icon: Icon(Icons.settings_outlined), selectedIcon: Icon(Icons.settings), label: '设置'),
        ],
      ),
    );
  }
}

class BankPage extends StatefulWidget {
  const BankPage({super.key, required this.store});
  final QuestionStore store;

  @override
  State<BankPage> createState() => _BankPageState();
}

class _BankPageState extends State<BankPage> {
  final _search = TextEditingController();
  String _grade = '';
  String _difficulty = '';
  String _province = '';
  bool _favorites = false;
  bool _exportedOnly = false;
  bool _missingFigureOnly = false;

  @override
  void dispose() {
    _search.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final store = widget.store;
    final results = store.filter(
      search: _search.text,
      grade: _grade,
      difficulty: _difficulty,
      province: _province,
      favoritesOnly: _favorites,
      exportedOnly: _exportedOnly,
      missingFigureOnly: _missingFigureOnly,
    );
    final provinces = store.provinceCounts.keys.toList();
    return Scaffold(
      appBar: AppBar(
        title: const Text('数学题库'),
        actions: [
          IconButton(
            onPressed: () => Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => ManualPaperPage(store: widget.store)),
            ),
            tooltip: '手动组卷',
            icon: const Icon(Icons.playlist_add),
          ),
          IconButton(
            onPressed: () => importQuestionsFromFile(context, widget.store),
            tooltip: '导入 JSON',
            icon: const Icon(Icons.upload_file_outlined),
          ),
          Padding(
            padding: const EdgeInsets.only(right: 8),
            child: FilledButton.tonalIcon(
              onPressed: () => _openEditor(context, null),
              icon: const Icon(Icons.add, size: 18),
              label: const Text('新增'),
            ),
          ),
        ],
      ),
      body: Column(
        children: [
          _hero(context, store),
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 4, 16, 8),
            child: TextField(
              controller: _search,
              onChanged: (_) => setState(() {}),
              decoration: InputDecoration(
                hintText: '搜索题干、答案、知识点或省份',
                prefixIcon: const Icon(Icons.search),
                suffixIcon: _search.text.isEmpty
                    ? null
                    : IconButton(
                        onPressed: () {
                          _search.clear();
                          setState(() {});
                        },
                        icon: const Icon(Icons.close),
                      ),
              ),
            ),
          ),
          SizedBox(
            height: 42,
            child: ListView(
              scrollDirection: Axis.horizontal,
              padding: const EdgeInsets.symmetric(horizontal: 16),
              children: [
                _chip('全部', _grade.isEmpty && _difficulty.isEmpty && _province.isEmpty && !_favorites && !_exportedOnly && !_missingFigureOnly, () {
                  setState(() {
                    _grade = '';
                    _difficulty = '';
                    _province = '';
                    _favorites = false;
                    _exportedOnly = false;
                    _missingFigureOnly = false;
                  });
                }),
                _chip('★ 收藏', _favorites, () => setState(() => _favorites = !_favorites)),
                _chip('已导出', _exportedOnly, () => setState(() => _exportedOnly = !_exportedOnly)),
                _chip('缺图', _missingFigureOnly, () => setState(() => _missingFigureOnly = !_missingFigureOnly)),
                _chip('高一', _grade == 'g1', () => setState(() => _grade = _grade == 'g1' ? '' : 'g1')),
                _chip('高二', _grade == 'g2', () => setState(() => _grade = _grade == 'g2' ? '' : 'g2')),
                _chip('高三', _grade == 'g3', () => setState(() => _grade = _grade == 'g3' ? '' : 'g3')),
                _chip('A 基础', _difficulty == 'A', () => setState(() => _difficulty = _difficulty == 'A' ? '' : 'A')),
                _chip('B 提升', _difficulty == 'B', () => setState(() => _difficulty = _difficulty == 'B' ? '' : 'B')),
                _chip('C 挑战', _difficulty == 'C', () => setState(() => _difficulty = _difficulty == 'C' ? '' : 'C')),
                if (provinces.isNotEmpty)
                  PopupMenuButton<String>(
                    onSelected: (value) => setState(() => _province = value),
                    itemBuilder: (context) => [
                      const PopupMenuItem(value: '', child: Text('全部省份')),
                      ...provinces.map((p) => PopupMenuItem(value: p, child: Text(p))),
                    ],
                    child: _chipLike(
                      _province.isEmpty ? '省份 ▾' : '$_province ▾',
                      _province.isNotEmpty,
                    ),
                  ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(18, 6, 18, 2),
            child: Row(
              children: [
                Text(
                  '共 ${results.length} 道',
                  style: TextStyle(
                    color: Theme.of(context).colorScheme.onSurfaceVariant,
                    fontSize: 12,
                  ),
                ),
                const Spacer(),
                if (_grade.isNotEmpty || _difficulty.isNotEmpty || _province.isNotEmpty || _favorites || _exportedOnly || _missingFigureOnly)
                  TextButton(
                    onPressed: () => setState(() {
                      _grade = '';
                      _difficulty = '';
                      _province = '';
                      _favorites = false;
                      _exportedOnly = false;
                      _missingFigureOnly = false;
                    }),
                    child: const Text('清除筛选'),
                  ),
              ],
            ),
          ),
          Expanded(
            child: results.isEmpty
                ? const _EmptyBank()
                : ListView.builder(
                    padding: const EdgeInsets.fromLTRB(12, 4, 12, 20),
                    itemCount: results.length,
                    itemBuilder: (context, index) {
                      final question = results[index];
                      return _QuestionCard(
                        question: question,
                        onTap: () => _openDetail(context, question),
                        onFavorite: () => store.toggleFavorite(question.id),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }

  Widget _hero(BuildContext context, QuestionStore store) {
    final scheme = Theme.of(context).colorScheme;
    return Container(
      margin: const EdgeInsets.fromLTRB(16, 4, 16, 12),
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(22),
        gradient: LinearGradient(
          colors: [scheme.primary, scheme.primary.withValues(alpha: 0.72)],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
      ),
      child: Row(
        children: [
          Container(
            width: 52,
            height: 52,
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.16),
              borderRadius: BorderRadius.circular(16),
            ),
            child: const Icon(Icons.auto_stories, color: Colors.white, size: 27),
          ),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '${store.count} 道题 · ${store.favoriteCount} 收藏 · ${store.exportedCount} 已导出',
                  style: const TextStyle(color: Colors.white, fontSize: 19, fontWeight: FontWeight.w800),
                ),
                const SizedBox(height: 4),
                Text(
                  store.missingFigureCount > 0
                      ? '本地题库 · ${store.missingFigureCount} 道缺图待补 · 离线可用'
                      : '本地题库 · 随时筛选 · 离线可用',
                  style: TextStyle(color: Colors.white.withValues(alpha: 0.78), fontSize: 12),
                ),
              ],
            ),
          ),
          IconButton(
            onPressed: () => _openEditor(context, null),
            style: IconButton.styleFrom(
              backgroundColor: Colors.white.withValues(alpha: 0.16),
              foregroundColor: Colors.white,
            ),
            icon: const Icon(Icons.add),
          ),
        ],
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
        labelStyle: TextStyle(fontSize: 12, color: selected ? Theme.of(context).colorScheme.primary : null),
      ),
    );
  }

  Widget _chipLike(String label, bool selected) {
    final scheme = Theme.of(context).colorScheme;
    return Container(
      margin: const EdgeInsets.only(right: 8),
      padding: const EdgeInsets.symmetric(horizontal: 14),
      height: 34,
      alignment: Alignment.center,
      decoration: BoxDecoration(
        color: selected ? scheme.primaryContainer : scheme.surface,
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: selected ? scheme.primary : scheme.outlineVariant),
      ),
      child: Text(label, style: TextStyle(fontSize: 12, color: selected ? scheme.primary : scheme.onSurfaceVariant)),
    );
  }

  Future<void> _openDetail(BuildContext context, Question question) async {
    await Navigator.of(context).push(
      MaterialPageRoute(
        builder: (_) => QuestionDetailPage(
          question: question,
          onEdit: () => _openEditor(context, question),
          onDelete: () async {
            await widget.store.delete(question.id);
            if (context.mounted) Navigator.of(context).pop();
          },
        ),
      ),
    );
    if (mounted) setState(() {});
  }

  Future<void> _openEditor(BuildContext context, Question? question) async {
    final result = await Navigator.of(context).push<Question>(
      MaterialPageRoute(builder: (_) => QuestionEditorPage(question: question)),
    );
    if (result != null) {
      await widget.store.upsert(result);
      if (mounted) setState(() {});
    }
  }
}

class RecentPage extends StatelessWidget {
  const RecentPage({super.key, required this.store});
  final QuestionStore store;

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: store,
      builder: (context, _) {
        final questions = store.recentQuestions;
        return Scaffold(
          appBar: AppBar(
            title: const Text('最近做过'),
            actions: [
              Padding(
                padding: const EdgeInsets.only(right: 14),
                child: Center(
                  child: Text(
                    '${questions.length} 道',
                    style: TextStyle(
                      fontSize: 12,
                      color: Theme.of(context).colorScheme.onSurfaceVariant,
                    ),
                  ),
                ),
              ),
            ],
          ),
          body: questions.isEmpty
              ? const _EmptyRecent()
              : ListView.builder(
                  padding: const EdgeInsets.fromLTRB(12, 4, 12, 24),
                  itemCount: questions.length,
                  itemBuilder: (context, index) {
                    final question = questions[index];
                    return _QuestionCard(
                      question: question,
                      onTap: () => _openDetail(context, question),
                      onFavorite: () => store.toggleFavorite(question.id),
                    );
                  },
                ),
        );
      },
    );
  }

  Future<void> _openDetail(BuildContext context, Question question) async {
    await Navigator.of(context).push(
      MaterialPageRoute(
        builder: (_) => QuestionDetailPage(
          question: question,
          onEdit: () async {
            final updated = await Navigator.of(context).push<Question>(
              MaterialPageRoute(builder: (_) => QuestionEditorPage(question: question)),
            );
            if (updated != null) await store.upsert(updated);
          },
          onDelete: () async {
            await store.delete(question.id);
            if (context.mounted) Navigator.of(context).pop();
          },
        ),
      ),
    );
  }
}

class _EmptyRecent extends StatelessWidget {
  const _EmptyRecent();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.history, size: 56, color: Theme.of(context).colorScheme.outline),
            const SizedBox(height: 12),
            const Text('最近做过还是空的', style: TextStyle(fontSize: 17, fontWeight: FontWeight.w700)),
            const SizedBox(height: 6),
            const Text(
              '导出过的题会自动打上“已导出”标记，并出现在这里。',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 12),
            ),
          ],
        ),
      ),
    );
  }
}

class _QuestionCard extends StatelessWidget {
  const _QuestionCard({
    required this.question,
    required this.onTap,
    required this.onFavorite,
  });

  final Question question;
  final VoidCallback onTap;
  final VoidCallback onFavorite;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Card(
      margin: const EdgeInsets.only(bottom: 10),
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(18),
        child: Padding(
          padding: const EdgeInsets.fromLTRB(16, 14, 12, 12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  _pill(question.difficultyLabel, scheme.primaryContainer, scheme.primary),
                  const SizedBox(width: 6),
                  _pill(question.typeLabel, scheme.surfaceContainerHighest, scheme.onSurfaceVariant),
                  const SizedBox(width: 6),
                  _pill(question.gradeLabel, scheme.tertiaryContainer, scheme.tertiary),
                  if (question.province.isNotEmpty) ...[
                    const SizedBox(width: 6),
                    _pill(question.province, scheme.secondaryContainer, scheme.secondary, maxWidth: 100),
                  ],
                  const Spacer(),
                  IconButton(
                    onPressed: onFavorite,
                    visualDensity: VisualDensity.compact,
                    icon: Icon(
                      question.favorite ? Icons.star_rounded : Icons.star_border_rounded,
                      color: question.favorite ? const Color(0xFFF4A21A) : scheme.outline,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 10),
              Text(
                question.stem,
                maxLines: 3,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(
                  fontSize: 15.5,
                  height: 1.45,
                  color: scheme.onSurface,
                  fontWeight: FontWeight.w600,
                ),
              ),
              if (question.allTags.isNotEmpty) ...[
                const SizedBox(height: 10),
                Wrap(
                  spacing: 6,
                  runSpacing: 6,
                  children: [
                    for (final tag in question.allTags.take(5))
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                        decoration: BoxDecoration(
                          color: scheme.surfaceContainerHighest,
                          borderRadius: BorderRadius.circular(6),
                        ),
                        child: Text(tag, style: TextStyle(fontSize: 10, color: scheme.onSurfaceVariant)),
                      ),
                  ],
                ),
              ],
              if (question.exported) ...[
                const SizedBox(height: 8),
                Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.check_circle, size: 13, color: scheme.tertiary),
                    const SizedBox(width: 4),
                    Text(
                      '已导出 · 最近做过',
                      style: TextStyle(fontSize: 10, color: scheme.tertiary, fontWeight: FontWeight.w600),
                    ),
                  ],
                ),
              ],
              if (question.needsFigure) ...[
                const SizedBox(height: 8),
                Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.image_not_supported_outlined, size: 13, color: scheme.error),
                    const SizedBox(width: 4),
                    Text(
                      '题干提到“如图”，但还没有配图',
                      style: TextStyle(fontSize: 10, color: scheme.error),
                    ),
                  ],
                ),
              ] else if (question.hasImage) ...[
                const SizedBox(height: 8),
                Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.image_outlined, size: 13, color: scheme.primary),
                    const SizedBox(width: 4),
                    Text('已配图', style: TextStyle(fontSize: 10, color: scheme.primary)),
                  ],
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _pill(String text, Color background, Color foreground, {double maxWidth = 90}) {
    return Container(
      constraints: BoxConstraints(maxWidth: maxWidth),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
      decoration: BoxDecoration(color: background, borderRadius: BorderRadius.circular(7)),
      child: Text(
        text,
        maxLines: 1,
        overflow: TextOverflow.ellipsis,
        style: TextStyle(fontSize: 10, color: foreground, fontWeight: FontWeight.w600),
      ),
    );
  }
}

class _EmptyBank extends StatelessWidget {
  const _EmptyBank();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.search_off, size: 54, color: Theme.of(context).colorScheme.outline),
            const SizedBox(height: 12),
            const Text('没有匹配的题目', style: TextStyle(fontSize: 17, fontWeight: FontWeight.w700)),
            const SizedBox(height: 6),
            const Text('换一个关键词或清除筛选条件试试', style: TextStyle(fontSize: 12)),
          ],
        ),
      ),
    );
  }
}

class QuestionDetailPage extends StatelessWidget {
  const QuestionDetailPage({
    super.key,
    required this.question,
    required this.onEdit,
    required this.onDelete,
  });

  final Question question;
  final VoidCallback onEdit;
  final Future<void> Function() onDelete;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('题目详情'),
        actions: [
          IconButton(onPressed: onEdit, icon: const Icon(Icons.edit_outlined)),
          IconButton(
            onPressed: () => showDialog<void>(
              context: context,
              builder: (context) => AlertDialog(
                title: const Text('删除这道题？'),
                content: const Text('删除后无法恢复，但不会影响其他题目。'),
                actions: [
                  TextButton(onPressed: () => Navigator.pop(context), child: const Text('取消')),
                  FilledButton(
                    onPressed: () {
                      Navigator.pop(context);
                      onDelete();
                    },
                    child: const Text('删除'),
                  ),
                ],
              ),
            ),
            icon: const Icon(Icons.delete_outline),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 30),
        children: [
          Wrap(
            spacing: 6,
            runSpacing: 6,
            children: [
              Chip(label: Text(question.difficultyLabel), visualDensity: VisualDensity.compact),
              Chip(label: Text(question.typeLabel), visualDensity: VisualDensity.compact),
              Chip(label: Text(question.gradeLabel), visualDensity: VisualDensity.compact),
              if (question.province.isNotEmpty)
                Chip(label: Text(question.province), visualDensity: VisualDensity.compact),
              if (question.exported)
                Chip(
                  avatar: Icon(Icons.check_circle, size: 16, color: scheme.tertiary),
                  label: const Text('已导出'),
                  visualDensity: VisualDensity.compact,
                ),
            ],
          ),
          const SizedBox(height: 12),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(18),
              child: SelectableText(
                question.stem,
                style: const TextStyle(fontSize: 17, height: 1.6, fontWeight: FontWeight.w600),
              ),
            ),
          ),
          QuestionFigure(question: question),
          if (question.options.isNotEmpty) ...[
            const SizedBox(height: 14),
            const _SectionTitle('选项'),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  children: [
                    for (var i = 0; i < question.options.length; i++)
                      Padding(
                        padding: const EdgeInsets.only(bottom: 10),
                        child: Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            CircleAvatar(
                              radius: 12,
                              backgroundColor: scheme.primaryContainer,
                              child: Text(String.fromCharCode(65 + i),
                                  style: TextStyle(fontSize: 11, color: scheme.primary, fontWeight: FontWeight.w700)),
                            ),
                            const SizedBox(width: 10),
                            Expanded(child: Text(question.options[i], style: const TextStyle(fontSize: 14.5, height: 1.4))),
                          ],
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ],
          const SizedBox(height: 14),
          const _SectionTitle('答案'),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: SelectableText(question.answer.isEmpty ? '暂无答案' : question.answer,
                  style: TextStyle(fontSize: 15, height: 1.55, color: scheme.primary, fontWeight: FontWeight.w600)),
            ),
          ),
          const SizedBox(height: 14),
          const _SectionTitle('解析'),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: SelectableText(question.analysis.isEmpty ? '暂无解析' : question.analysis,
                  style: const TextStyle(fontSize: 14, height: 1.65)),
            ),
          ),
          if (question.allTags.isNotEmpty) ...[
            const SizedBox(height: 14),
            const _SectionTitle('标签'),
            Wrap(
              spacing: 7,
              runSpacing: 7,
              children: [
                for (final tag in question.allTags)
                  Chip(label: Text(tag), visualDensity: VisualDensity.compact),
              ],
            ),
          ],
        ],
      ),
    );
  }
}

class _SectionTitle extends StatelessWidget {
  const _SectionTitle(this.text);
  final String text;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(left: 2, bottom: 7),
      child: Text(text, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.w800)),
    );
  }
}

class QuestionFigure extends StatelessWidget {
  const QuestionFigure({super.key, required this.question});
  final Question question;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    if (question.hasImage) {
      return Padding(
        padding: const EdgeInsets.only(top: 12),
        child: Card(
          clipBehavior: Clip.antiAlias,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Padding(
                padding: const EdgeInsets.fromLTRB(14, 12, 14, 0),
                child: Text('题目配图', style: TextStyle(fontSize: 12, fontWeight: FontWeight.w700, color: scheme.primary)),
              ),
              Padding(
                padding: const EdgeInsets.all(12),
                child: ClipRRect(
                  borderRadius: BorderRadius.circular(8),
                  child: Image.memory(
                    base64Decode(question.imageBase64),
                    width: double.infinity,
                    fit: BoxFit.contain,
                    errorBuilder: (context, error, stack) => const Padding(
                      padding: EdgeInsets.all(16),
                      child: Text('配图数据损坏，请重新添加'),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      );
    }
    if (question.needsFigure) {
      return Padding(
        padding: const EdgeInsets.only(top: 12),
        child: Container(
          padding: const EdgeInsets.all(14),
          decoration: BoxDecoration(
            color: scheme.errorContainer,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Icon(Icons.image_not_supported_outlined, size: 18, color: scheme.onErrorContainer),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  '题干提到“如图/下图”，但题库里没有配图。点右上角编辑，可给这道题补一张图。',
                  style: TextStyle(fontSize: 12, color: scheme.onErrorContainer, height: 1.45),
                ),
              ),
            ],
          ),
        ),
      );
    }
    return const SizedBox.shrink();
  }
}

class QuestionEditorPage extends StatefulWidget {
  const QuestionEditorPage({super.key, this.question});
  final Question? question;

  @override
  State<QuestionEditorPage> createState() => _QuestionEditorPageState();
}

class _QuestionEditorPageState extends State<QuestionEditorPage> {
  late final TextEditingController _stem;
  late final TextEditingController _answer;
  late final TextEditingController _analysis;
  late final TextEditingController _formulas;
  late final TextEditingController _concepts;
  late final List<TextEditingController> _options;
  String _difficulty = 'B';
  String _type = 'choice';
  String _grade = '';
  String _province = '';
  String _imageBase64 = '';

  @override
  void initState() {
    super.initState();
    final q = widget.question;
    _stem = TextEditingController(text: q?.stem ?? '');
    _answer = TextEditingController(text: q?.answer ?? '');
    _analysis = TextEditingController(text: q?.analysis ?? '');
    _formulas = TextEditingController(text: q?.formulas.join('、') ?? '');
    _concepts = TextEditingController(text: q?.concepts.join('、') ?? '');
    _options = List.generate(4, (i) => TextEditingController(text: (q?.options != null && q!.options.length > i) ? q.options[i] : ''));
    _difficulty = q?.difficulty ?? 'B';
    _type = q?.type ?? 'choice';
    _grade = q?.grade ?? '';
    _province = q?.province ?? '';
    _imageBase64 = q?.imageBase64 ?? '';
  }

  @override
  void dispose() {
    _stem.dispose();
    _answer.dispose();
    _analysis.dispose();
    _formulas.dispose();
    _concepts.dispose();
    for (final c in _options) {
      c.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final isNew = widget.question == null;
    return Scaffold(
      appBar: AppBar(
        title: Text(isNew ? '新增题目' : '编辑题目'),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 12),
            child: FilledButton(onPressed: _save, child: const Text('保存')),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 30),
        children: [
          TextField(
            controller: _stem,
            minLines: 4,
            maxLines: 10,
            decoration: const InputDecoration(labelText: '题干', hintText: '输入题目内容'),
          ),
          const SizedBox(height: 10),
          Card(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(14, 12, 14, 10),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.image_outlined, size: 18),
                      const SizedBox(width: 6),
                      const Text('题目配图', style: TextStyle(fontSize: 14, fontWeight: FontWeight.w800)),
                      const Spacer(),
                      if (_imageBase64.isNotEmpty)
                        IconButton(
                          tooltip: '删除配图',
                          onPressed: () => setState(() => _imageBase64 = ''),
                          icon: const Icon(Icons.delete_outline, size: 18),
                        ),
                    ],
                  ),
                  if (_imageBase64.isNotEmpty)
                    ClipRRect(
                      borderRadius: BorderRadius.circular(8),
                      child: Image.memory(
                        base64Decode(_imageBase64),
                        height: 150,
                        fit: BoxFit.contain,
                        errorBuilder: (context, error, stack) => const Text('配图数据损坏'),
                      ),
                    )
                  else
                    Text(
                      '题干里有“如图/下图”时，建议补一张配图，避免详情和导出试卷缺图。',
                      style: TextStyle(
                        fontSize: 11,
                        color: Theme.of(context).colorScheme.onSurfaceVariant,
                        height: 1.4,
                      ),
                    ),
                  const SizedBox(height: 6),
                  OutlinedButton.icon(
                    onPressed: _pickImage,
                    icon: const Icon(Icons.add_photo_alternate_outlined, size: 18),
                    label: Text(_imageBase64.isEmpty ? '从相册选择图片' : '更换图片'),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(child: _dropdown('难度', _difficulty, const {'A': 'A 基础', 'B': 'B 提升', 'C': 'C 挑战'}, (v) => setState(() => _difficulty = v))),
              const SizedBox(width: 10),
              Expanded(child: _dropdown('题型', _type, const {'choice': '选择题', 'fill': '填空题', 'solution': '解答题'}, (v) => setState(() => _type = v))),
            ],
          ),
          const SizedBox(height: 10),
          Row(
            children: [
              Expanded(child: _dropdown('年级', _grade, const {'': '未分级', 'g1': '高一', 'g2': '高二', 'g3': '高三'}, (v) => setState(() => _grade = v))),
              const SizedBox(width: 10),
              Expanded(
                child: DropdownButtonFormField<String>(
                  initialValue: _province.isEmpty ? null : _province,
                  isExpanded: true,
                  decoration: const InputDecoration(labelText: '省份'),
                  hint: const Text('未识别'),
                  items: [
                    const DropdownMenuItem(value: '', child: Text('未识别')),
                    ...provinces.map((p) => DropdownMenuItem(value: p, child: Text(p))),
                  ],
                  onChanged: (v) => setState(() => _province = v ?? ''),
                ),
              ),
            ],
          ),
          if (_type == 'choice') ...[
            const SizedBox(height: 16),
            const Text('选项', style: TextStyle(fontSize: 14, fontWeight: FontWeight.w800)),
            const SizedBox(height: 8),
            for (var i = 0; i < 4; i++) ...[
              TextField(
                controller: _options[i],
                decoration: InputDecoration(labelText: '选项 ${String.fromCharCode(65 + i)}'),
              ),
              const SizedBox(height: 8),
            ],
          ],
          const SizedBox(height: 8),
          TextField(
            controller: _answer,
            minLines: 2,
            maxLines: 5,
            decoration: const InputDecoration(labelText: '答案'),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _analysis,
            minLines: 3,
            maxLines: 8,
            decoration: const InputDecoration(labelText: '解析'),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _formulas,
            decoration: const InputDecoration(labelText: '公式标签（用、或逗号分隔）'),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _concepts,
            decoration: const InputDecoration(labelText: '知识点标签（用、或逗号分隔）'),
          ),
        ],
      ),
    );
  }

  Widget _dropdown(String label, String value, Map<String, String> entries, ValueChanged<String> onChanged) {
    return DropdownButtonFormField<String>(
      initialValue: value,
      isExpanded: true,
      decoration: InputDecoration(labelText: label),
      items: entries.entries.map((e) => DropdownMenuItem(value: e.key, child: Text(e.value))).toList(),
      onChanged: (v) => onChanged(v ?? ''),
    );
  }

  Future<void> _pickImage() async {
    try {
      final result = await FilePicker.platform.pickFiles(
        type: FileType.image,
        withData: true,
      );
      if (result == null || result.files.isEmpty) return;
      final file = result.files.single;
      final bytes = file.bytes ??
          (file.path == null ? null : await File(file.path!).readAsBytes());
      if (bytes == null) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('无法读取图片')));
        }
        return;
      }
      final decoded = img.decodeImage(bytes);
      if (decoded == null) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('不支持的图片格式')));
        }
        return;
      }
      final resized = decoded.width > 1400
          ? img.copyResize(decoded, width: 1400, interpolation: img.Interpolation.linear)
          : decoded;
      final png = img.encodePng(resized);
      if (!mounted) return;
      setState(() => _imageBase64 = base64Encode(png));
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('选择图片失败：$error')));
    }
  }

  Future<void> _save() async {
    final stem = _stem.text.trim();
    if (stem.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('题干不能为空')));
      return;
    }
    final options = _type == 'choice'
        ? _options.map((c) => c.text.trim()).where((e) => e.isNotEmpty).toList()
        : <String>[];
    final question = Question(
      id: widget.question?.id ?? 'q_${DateTime.now().microsecondsSinceEpoch}',
      difficulty: _difficulty,
      type: _type,
      grade: _grade,
      province: _province.isNotEmpty ? _province : inferProvince(stem, options, _analysis.text),
      favorite: widget.question?.favorite ?? false,
      stem: stem,
      options: options,
      answer: _answer.text.trim(),
      analysis: _analysis.text.trim(),
      formulas: _splitTags(_formulas.text),
      concepts: _splitTags(_concepts.text),
      tagged: true,
      created: widget.question?.created ?? DateTime.now().toIso8601String(),
      exported: widget.question?.exported ?? false,
      lastExportedAt: widget.question?.lastExportedAt ?? '',
      exportCount: widget.question?.exportCount ?? 0,
      imageBase64: _imageBase64,
    );
    Navigator.of(context).pop(question);
  }

  List<String> _splitTags(String raw) {
    return raw
        .split(RegExp(r'[、,，\n]'))
        .map((e) => e.trim())
        .where((e) => e.isNotEmpty)
        .toSet()
        .toList();
  }
}

class TagsPage extends StatelessWidget {
  const TagsPage({super.key, required this.store});
  final QuestionStore store;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(title: const Text('标签容器')),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 4, 16, 30),
        children: [
          Row(
            children: [
              _stat(context, '公式', store.formulaCounts.length, scheme.primary),
              const SizedBox(width: 10),
              _stat(context, '知识点', store.conceptCounts.length, scheme.tertiary),
              const SizedBox(width: 10),
              _stat(context, '省份', store.provinceCounts.length, scheme.secondary),
            ],
          ),
          const SizedBox(height: 18),
          _tagSection(context, '公式标签', store.formulaCounts, scheme.primaryContainer, scheme.primary),
          const SizedBox(height: 18),
          _tagSection(context, '知识点标签', store.conceptCounts, scheme.tertiaryContainer, scheme.tertiary),
          const SizedBox(height: 18),
          _tagSection(context, '省份标签', store.provinceCounts, scheme.secondaryContainer, scheme.secondary),
        ],
      ),
    );
  }

  Widget _stat(BuildContext context, String label, int value, Color color) {
    return Expanded(
      child: Card(
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 12),
          child: Column(
            children: [
              Text('$value', style: TextStyle(fontSize: 25, fontWeight: FontWeight.w800, color: color)),
              const SizedBox(height: 4),
              Text(label, style: const TextStyle(fontSize: 11)),
            ],
          ),
        ),
      ),
    );
  }

  Widget _tagSection(BuildContext context, String title, Map<String, int> tags, Color background, Color foreground) {
    if (tags.isEmpty) return const SizedBox.shrink();
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: const TextStyle(fontSize: 15, fontWeight: FontWeight.w800)),
        const SizedBox(height: 8),
        Wrap(
          spacing: 8,
          runSpacing: 8,
          children: tags.entries
              .map((e) => Container(
                    padding: const EdgeInsets.symmetric(horizontal: 11, vertical: 7),
                    decoration: BoxDecoration(color: background, borderRadius: BorderRadius.circular(9)),
                    child: Text(
                      '${e.key}  ${e.value}',
                      style: TextStyle(fontSize: 12, color: foreground, fontWeight: FontWeight.w600),
                    ),
                  ))
              .toList(),
        ),
      ],
    );
  }
}

class SettingsPage extends StatelessWidget {
  const SettingsPage({
    super.key,
    required this.store,
    required this.darkMode,
    required this.onThemeChanged,
  });

  final QuestionStore store;
  final bool darkMode;
  final ValueChanged<bool> onThemeChanged;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('设置')),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 4, 16, 30),
        children: [
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  value: darkMode,
                  onChanged: onThemeChanged,
                  secondary: const Icon(Icons.dark_mode_outlined),
                  title: const Text('夜间模式'),
                  subtitle: const Text('深色外观，适合晚上刷题'),
                ),
                const Divider(height: 1),
                ListTile(
                  leading: const Icon(Icons.storage_outlined),
                  title: const Text('本地题库'),
                  subtitle: Text('${store.count} 道题 · ${store.favoriteCount} 道收藏'),
                ),
                const Divider(height: 1),
                const ListTile(
                  leading: Icon(Icons.offline_bolt_outlined),
                  title: Text('离线优先'),
                  subtitle: Text('题库保存在本机，不联网也能筛选和编辑'),
                ),
                const Divider(height: 1),
                ListTile(
                  leading: const Icon(Icons.upload_file_outlined),
                  title: const Text('导入题库 JSON'),
                  subtitle: const Text('从桌面版或备份文件合并题目'),
                  onTap: () => importQuestionsFromFile(context, store),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Card(
            child: Column(
              children: [
                ListTile(
                  leading: const Icon(Icons.auto_awesome_outlined),
                  title: const Text('AI 供应商配置'),
                  subtitle: const Text('API 地址、模型、密钥与连接测试'),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.of(context).push(
                    MaterialPageRoute(builder: (_) => const AiSettingsPage()),
                  ),
                ),
                const Divider(height: 1),
                const ListTile(
                  leading: Icon(Icons.document_scanner_outlined),
                  title: Text('AI 识题 / AI 标注'),
                  subtitle: Text('配置已完成，识题功能在下一阶段接入'),
                ),
                const Divider(height: 1),
                const ListTile(
                  leading: Icon(Icons.description_outlined),
                  title: Text('试卷导出'),
                  subtitle: Text('下一步接入：DOCX / PDF 与系统分享'),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Center(
            child: Text(
              '数研组卷 Mobile · 0.6.0',
              style: TextStyle(fontSize: 11, color: Theme.of(context).colorScheme.outline),
            ),
          ),
        ],
      ),
    );
  }
}
