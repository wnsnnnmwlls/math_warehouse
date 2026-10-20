import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:path_provider/path_provider.dart';

import '../models/question.dart';

class QuestionStore extends ChangeNotifier {
  final List<Question> _questions = [];
  Directory? _dataDir;
  bool _loaded = false;
  bool _figuresSeeded = false;
  String? loadError;

  List<Question> get questions => List.unmodifiable(_questions);
  bool get loaded => _loaded;
  int get count => _questions.length;
  int get favoriteCount => _questions.where((q) => q.favorite).length;
  int get exportedCount => _questions.where((q) => q.exported).length;
  int get missingFigureCount => _questions.where((q) => q.needsFigure).length;

  List<Question> get recentQuestions {
    final result = _questions.where((q) => q.exported).toList()
      ..sort((a, b) {
        final byTime = b.lastExportedAt.compareTo(a.lastExportedAt);
        return byTime != 0 ? byTime : a.id.compareTo(b.id);
      });
    return result;
  }

  Map<String, int> get formulaCounts => _counts((q) => q.formulas);
  Map<String, int> get conceptCounts => _counts((q) => q.concepts);

  Map<String, int> get provinceCounts {
    final result = <String, int>{};
    for (final q in _questions) {
      if (q.province.isNotEmpty) {
        result[q.province] = (result[q.province] ?? 0) + 1;
      }
    }
    return _sortedCounts(result);
  }

  Map<String, int> _counts(List<String> Function(Question) select) {
    final result = <String, int>{};
    for (final q in _questions) {
      for (final tag in select(q)) {
        result[tag] = (result[tag] ?? 0) + 1;
      }
    }
    return _sortedCounts(result);
  }

  Map<String, int> _sortedCounts(Map<String, int> input) {
    final entries = input.entries.toList()
      ..sort((a, b) {
        final byValue = b.value.compareTo(a.value);
        return byValue != 0 ? byValue : a.key.compareTo(b.key);
      });
    return {for (final entry in entries) entry.key: entry.value};
  }

  Future<void> load() async {
    try {
      final dir = await getApplicationDocumentsDirectory();
      _dataDir = dir;
      final file = File('${dir.path}/questions.json');
      String raw;
      if (await file.exists()) {
        raw = await file.readAsString();
      } else {
        raw = await rootBundle.loadString('assets/questions.json');
      }
      _replaceFromJson(raw);
      await _seedBundledFigures();
      _loaded = true;
      if (!await file.exists()) {
        await save();
      }
    } catch (error) {
      loadError = error.toString();
      try {
        _replaceFromJson(await rootBundle.loadString('assets/questions.json'));
      } catch (_) {
        _questions.clear();
      }
      _loaded = true;
    }
    notifyListeners();
  }

  void _replaceFromJson(String raw) {
    final decoded = jsonDecode(raw.replaceFirst('\uFEFF', ''));
    _figuresSeeded =
        decoded is Map<String, dynamic> && decoded['figures_seeded'] == true;
    final list = decoded is Map<String, dynamic>
        ? (decoded['questions'] as List<dynamic>? ?? [])
        : decoded is List<dynamic>
            ? decoded
            : <dynamic>[];
    _questions
      ..clear()
      ..addAll(list.whereType<Map<String, dynamic>>().map(Question.fromJson));
  }

  Future<void> _seedBundledFigures() async {
    if (_figuresSeeded) return;
    var changed = false;
    try {
      final raw = await rootBundle.loadString('assets/figure_map.json');
      final decoded = jsonDecode(raw);
      final map = decoded is Map<String, dynamic>
          ? (decoded['figures'] as Map<String, dynamic>? ?? {})
          : <String, dynamic>{};
      for (final question in _questions) {
        if (question.hasImage) continue;
        final assetPath = map[question.id]?.toString();
        if (assetPath == null || assetPath.isEmpty) continue;
        final data = await rootBundle.load(assetPath);
        question.imageBase64 = base64Encode(data.buffer.asUint8List());
        changed = true;
      }
    } catch (_) {
      // A missing figure bundle must not prevent the question bank loading.
    }
    _figuresSeeded = true;
    if (changed || _dataDir != null) {
      await save();
    }
  }

  Future<int> importJson(String raw) async {
    final decoded = jsonDecode(raw.replaceFirst('\uFEFF', ''));
    final list = decoded is Map<String, dynamic>
        ? (decoded['questions'] as List<dynamic>? ?? [])
        : decoded is List<dynamic>
            ? decoded
            : <dynamic>[];
    final existingIds = _questions.map((q) => q.id).toSet();
    var added = 0;
    for (final item in list.whereType<Map<String, dynamic>>()) {
      final question = Question.fromJson(item);
      if (existingIds.contains(question.id)) continue;
      existingIds.add(question.id);
      _questions.insert(0, question);
      added++;
    }
    if (added > 0) {
      await save();
      notifyListeners();
    }
    return added;
  }

  Future<void> save() async {
    _dataDir ??= await getApplicationDocumentsDirectory();
    final file = File('${_dataDir!.path}/questions.json');
    final payload = jsonEncode({
      'version': 1,
      'updated': DateTime.now().toIso8601String(),
      'figures_seeded': _figuresSeeded,
      'questions': _questions.map((q) => q.toJson()).toList(),
    });
    final temp = File('${file.path}.tmp');
    await temp.writeAsString(payload, flush: true);
    if (await file.exists()) await file.delete();
    await temp.rename(file.path);
  }

  Future<void> upsert(Question question) async {
    final index = _questions.indexWhere((q) => q.id == question.id);
    if (index >= 0) {
      _questions[index] = question;
    } else {
      _questions.insert(0, question);
    }
    await save();
    notifyListeners();
  }

  Future<void> delete(String id) async {
    _questions.removeWhere((q) => q.id == id);
    await save();
    notifyListeners();
  }

  Future<void> toggleFavorite(String id) async {
    final index = _questions.indexWhere((q) => q.id == id);
    if (index < 0) return;
    _questions[index].favorite = !_questions[index].favorite;
    await save();
    notifyListeners();
  }

  Future<int> markExported(Iterable<String> ids) async {
    final wanted = ids.toSet();
    if (wanted.isEmpty) return 0;
    final now = DateTime.now().toIso8601String();
    var count = 0;
    for (final question in _questions) {
      if (!wanted.contains(question.id)) continue;
      question.exported = true;
      question.lastExportedAt = now;
      question.exportCount += 1;
      count++;
    }
    if (count > 0) {
      await save();
      notifyListeners();
    }
    return count;
  }

  List<Question> filter({
    String search = '',
    String grade = '',
    String difficulty = '',
    String province = '',
    bool favoritesOnly = false,
    bool exportedOnly = false,
    bool missingFigureOnly = false,
  }) {
    final query = search.trim().toLowerCase();
    return _questions.where((q) {
      if (grade.isNotEmpty && q.grade != grade) return false;
      if (difficulty.isNotEmpty && q.difficulty != difficulty) return false;
      if (province.isNotEmpty && q.province != province) return false;
      if (favoritesOnly && !q.favorite) return false;
      if (exportedOnly && !q.exported) return false;
      if (missingFigureOnly && !q.needsFigure) return false;
      if (query.isNotEmpty) {
        final haystack = [
          q.stem,
          q.answer,
          q.analysis,
          q.province,
          ...q.options,
          ...q.formulas,
          ...q.concepts,
        ].join('\n').toLowerCase();
        if (!haystack.contains(query)) return false;
      }
      return true;
    }).toList();
  }
} 
