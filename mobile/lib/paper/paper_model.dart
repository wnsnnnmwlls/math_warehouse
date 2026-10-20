import '../models/question.dart';

class PaperQuestion {
  PaperQuestion({required this.question, required this.points});
  final Question question;
  final int points;
}

class PaperSection {
  PaperSection({
    required this.title,
    required this.instruction,
    required this.questions,
    required this.pointsPerQuestion,
  });

  final String title;
  final String instruction;
  final List<PaperQuestion> questions;
  final int pointsPerQuestion;

  int get totalPoints =>
      questions.fold(0, (sum, item) => sum + item.points);
}

class Paper {
  Paper({
    required this.title,
    required this.minutes,
    required this.sections,
    this.note = '',
  });

  final String title;
  final int minutes;
  final List<PaperSection> sections;
  final String note;

  int get totalPoints => sections.fold(0, (sum, s) => sum + s.totalPoints);
  int get questionCount =>
      sections.fold(0, (sum, s) => sum + s.questions.length);
}

List<PaperSection> buildPaperSections({
  required List<Question> choice,
  required List<Question> fill,
  required List<Question> solution,
}) {
  final sections = <PaperSection>[];
  if (choice.isNotEmpty) {
    sections.add(PaperSection(
      title: '一、选择题',
      instruction: '本题共${choice.length}小题，每小题5分，共${choice.length * 5}分。在每小题给出的四个选项中，只有一项是符合题目要求的。',
      questions: [
        for (final q in choice) PaperQuestion(question: q, points: 5),
      ],
      pointsPerQuestion: 5,
    ));
  }
  if (fill.isNotEmpty) {
    sections.add(PaperSection(
      title: '二、填空题',
      instruction: '本题共${fill.length}小题，每小题5分，共${fill.length * 5}分。',
      questions: [
        for (final q in fill) PaperQuestion(question: q, points: 5),
      ],
      pointsPerQuestion: 5,
    ));
  }
  if (solution.isNotEmpty) {
    // 12/4/6 标准卷：解答题共 70 分，第一题 10 分，其余每题 12 分。
    var points = 0;
    final paperQuestions = <PaperQuestion>[];
    for (var i = 0; i < solution.length; i++) {
      final value = i == 0 ? 10 : 12;
      points += value;
      paperQuestions.add(PaperQuestion(question: solution[i], points: value));
    }
    sections.add(PaperSection(
      title: '三、解答题',
      instruction: '本题共${solution.length}小题，共$points分。解答应写出文字说明、证明过程或演算步骤。',
      questions: paperQuestions,
      pointsPerQuestion: 12,
    ));
  }
  return sections;
}
