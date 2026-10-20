import 'dart:convert';
import 'dart:io';

import 'package:archive/archive.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter/material.dart';
import 'package:math_paper_mobile/data/question_store.dart';
import 'package:math_paper_mobile/main.dart';
import 'package:math_paper_mobile/models/question.dart';
import 'package:math_paper_mobile/paper/paper_docx.dart';
import 'package:math_paper_mobile/paper/paper_model.dart';
import 'package:math_paper_mobile/paper/manual_paper_page.dart';
import 'package:math_paper_mobile/paper/paper_page.dart';

void main() {
  test('province aliases normalize to province-level names', () {
    expect(normalizeProvince('广东汕头'), '广东省');
    expect(normalizeProvince('北京海淀'), '北京市');
    expect(normalizeProvince('内蒙古呼和浩特'), '内蒙古自治区');
  });

  test('question JSON keeps province and parses options', () {
    final question = Question.fromJson({
      'id': 'q1',
      'grade': 'g1',
      'type': 'choice',
      'options': ['A', 'B', 'C', 'D'],
      'stem': '（2024·广东汕头·一模）测试题',
      'answer': 'A',
      'analysis': '解析',
    });
    expect(question.province, '广东省');
    expect(question.options.length, 4);
    expect(question.gradeLabel, '高一');
  });

  test('paper docx contains questions, options and answer page', () {
    final choice = Question(
      id: 'c1',
      type: 'choice',
      stem: '已知集合 A={1,2}，则 A 的子集个数是（ ）',
      options: const ['2', '3', '4', '5'],
      answer: 'C',
      analysis: '2²=4。',
    );
    final fill = Question(
      id: 'f1',
      type: 'fill',
      stem: '计算 sin30°=______。',
      answer: '1/2',
      analysis: '特殊角。',
    );
    final solution = Question(
      id: 's1',
      type: 'solution',
      stem: '解不等式 x²-4>0。',
      answer: 'x<-2 或 x>2',
      analysis: '因式分解。',
    );
    final sections = buildPaperSections(
      choice: [choice],
      fill: [fill],
      solution: [solution],
    );
    final paper = Paper(
      title: '测试数学试卷',
      minutes: 120,
      sections: sections,
      note: '测试用卷',
    );
    final bytes = buildDocxBytes(paper);
    expect(bytes.length, greaterThan(1000));
    final archive = ZipDecoder().decodeBytes(bytes);
    final names = archive.files.map((f) => f.name).toSet();
    expect(names, contains('word/document.xml'));
    expect(names, contains('[Content_Types].xml'));
    final document = utf8.decode(
      archive.files.firstWhere((f) => f.name == 'word/document.xml').content as List<int>,
    );
    expect(document, contains('测试数学试卷'));
    expect(document, contains('已知集合'));
    expect(document, contains('参考答案与解析'));
    expect(paper.totalPoints, 20);
    Directory('build').createSync(recursive: true);
    File('build/test-paper.docx').writeAsBytesSync(bytes);
  });

  test('paper docx embeds an attached figure', () {
    final icon = File('android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png').readAsBytesSync();
    final question = Question(
      id: 'fig1',
      type: 'solution',
      stem: '如图，在三角形 ABC 中，求角 A。',
      answer: '60°',
      imageBase64: base64Encode(icon),
    );
    final paper = Paper(
      title: '带图试卷',
      minutes: 120,
      sections: buildPaperSections(choice: const [], fill: const [], solution: [question]),
    );
    final bytes = buildDocxBytes(paper);
    final archive = ZipDecoder().decodeBytes(bytes);
    expect(archive.files.map((f) => f.name), contains('word/media/image1.png'));
    final document = utf8.decode(
      archive.files.firstWhere((f) => f.name == 'word/document.xml').content as List<int>,
    );
    expect(document, contains('r:embed="rIdImage1"'));
    Directory('build').createSync(recursive: true);
    File('build/test-paper-image.docx').writeAsBytesSync(bytes);
  });

  testWidgets('paper composer fits a phone width', (tester) async {
    tester.view.physicalSize = const Size(390, 844);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    await tester.pumpWidget(MaterialApp(home: PaperPage(store: QuestionStore())));
    await tester.pump();
    expect(find.text('AI 生成试卷'), findsOneWidget);
  });

  testWidgets('manual composer fits a phone width', (tester) async {
    tester.view.physicalSize = const Size(390, 844);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    await tester.pumpWidget(MaterialApp(home: ManualPaperPage(store: QuestionStore())));
    await tester.pump();
    expect(find.text('手动组卷'), findsWidgets);
  });

  testWidgets('bank app bar fits a phone width', (tester) async {
    tester.view.physicalSize = const Size(390, 844);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    await tester.pumpWidget(MaterialApp(home: BankPage(store: QuestionStore())));
    await tester.pump();
    expect(find.text('数学题库'), findsOneWidget);
  });

  test('every built-in figure question has a bundled figure', () {
    final raw = jsonDecode(File('assets/questions.json').readAsStringSync());
    final rawQuestions = (raw is Map<String, dynamic> ? raw['questions'] : raw) as List<dynamic>;
    final mapRaw = jsonDecode(File('assets/figure_map.json').readAsStringSync());
    final figures = (mapRaw as Map<String, dynamic>)['figures'] as Map<String, dynamic>;
    var missing = 0;
    for (final item in rawQuestions.whereType<Map<String, dynamic>>()) {
      final question = Question.fromJson(item);
      if (question.needsFigure) {
        missing++;
        expect(figures.containsKey(question.id), isTrue, reason: question.id);
      }
    }
    expect(missing, 31);
    for (final path in figures.values) {
      expect(File(path.toString()).existsSync(), isTrue, reason: path.toString());
    }
  });

  test('export markers survive question JSON round trip', () {
    final question = Question(
      id: 'e1',
      stem: '测试',
      exported: true,
      lastExportedAt: '2026-10-20T20:00:00.000',
      exportCount: 2,
    );
    final restored = Question.fromJson(question.toJson());
    expect(restored.exported, isTrue);
    expect(restored.lastExportedAt, question.lastExportedAt);
    expect(restored.exportCount, 2);
  });
}
