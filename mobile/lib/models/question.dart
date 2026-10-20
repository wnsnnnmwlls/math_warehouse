class Question {
  Question({
    required this.id,
    this.difficulty = 'B',
    this.type = 'choice',
    this.grade = '',
    this.province = '',
    this.favorite = false,
    this.stem = '',
    this.options = const [],
    this.answer = '',
    this.analysis = '',
    this.formulas = const [],
    this.concepts = const [],
    this.tagged = false,
    this.created = '',
    this.exported = false,
    this.lastExportedAt = '',
    this.exportCount = 0,
    this.imageBase64 = '',
  });

  String id;
  String difficulty;
  String type;
  String grade;
  String province;
  bool favorite;
  String stem;
  List<String> options;
  String answer;
  String analysis;
  List<String> formulas;
  List<String> concepts;
  bool tagged;
  String created;
  bool exported;
  String lastExportedAt;
  int exportCount;
  String imageBase64;

  List<String> get allTags => [...formulas, ...concepts];
  bool get hasImage => imageBase64.trim().isNotEmpty;

  bool get needsFigure {
    if (hasImage) return false;
    final text = '$stem\n${options.join('\n')}\n$analysis';
    return RegExp(r'如图|下图|图中|图示|如图所').hasMatch(text);
  }

  String get difficultyLabel => switch (difficulty) {
        'A' => 'A 基础',
        'C' => 'C 挑战',
        _ => 'B 提升',
      };

  String get typeLabel => switch (type) {
        'fill' => '填空题',
        'solution' => '解答题',
        _ => '选择题',
      };

  String get gradeLabel => switch (grade) {
        'g1' => '高一',
        'g2' => '高二',
        'g3' => '高三',
        _ => '未分级',
      };

  factory Question.fromJson(Map<String, dynamic> json) {
    final stem = (json['stem'] ?? '').toString();
    final options = (json['options'] as List<dynamic>? ?? [])
        .map((e) => e.toString())
        .where((e) => e.isNotEmpty)
        .toList();
    final formulas = (json['formulas'] as List<dynamic>? ?? [])
        .map((e) => e.toString())
        .toList();
    final concepts = (json['concepts'] as List<dynamic>? ?? [])
        .map((e) => e.toString())
        .toList();
    final grade = _normalizeGrade((json['grade'] ?? '').toString());
    final province = normalizeProvince((json['province'] ?? '').toString()).isNotEmpty
        ? normalizeProvince((json['province'] ?? '').toString())
        : inferProvince(stem, options, (json['analysis'] ?? '').toString());
    return Question(
      id: (json['id'] ?? 'q_${DateTime.now().microsecondsSinceEpoch}').toString(),
      difficulty: (json['difficulty'] ?? 'B').toString().toUpperCase(),
      type: (json['type'] ?? 'choice').toString(),
      grade: grade,
      province: province,
      favorite: json['favorite'] == true,
      stem: stem,
      options: options,
      answer: (json['answer'] ?? '').toString(),
      analysis: (json['analysis'] ?? '').toString(),
      formulas: formulas,
      concepts: concepts,
      tagged: json['tagged'] == true || formulas.isNotEmpty || concepts.isNotEmpty,
      created: (json['created'] ?? '').toString(),
      exported: json['exported'] == true,
      lastExportedAt: (json['last_exported_at'] ?? json['lastExportedAt'] ?? '').toString(),
      exportCount: int.tryParse((json['export_count'] ?? json['exportCount'] ?? '0').toString()) ?? 0,
      imageBase64: (json['image_base64'] ?? json['imageBase64'] ?? json['image'] ?? '').toString(),
    );
  }

  Map<String, dynamic> toJson() => {
        'id': id,
        'difficulty': difficulty,
        'type': type,
        'grade': grade,
        'province': province,
        'favorite': favorite,
        'stem': stem,
        'options': options,
        'answer': answer,
        'analysis': analysis,
        'formulas': formulas,
        'concepts': concepts,
        'tagged': tagged,
        'created': created,
        'exported': exported,
        'last_exported_at': lastExportedAt,
        'export_count': exportCount,
        'image_base64': imageBase64,
      };

  static String _normalizeGrade(String raw) {
    final value = raw.trim().toLowerCase();
    if (value == 'g1' || value == '高一' || value == 'high1') return 'g1';
    if (value == 'g2' || value == '高二' || value == 'high2') return 'g2';
    if (value == 'g3' || value == '高三' || value == 'high3') return 'g3';
    return '';
  }
}

const List<String> provinces = [
  '北京市', '天津市', '上海市', '重庆市',
  '河北省', '山西省', '辽宁省', '吉林省', '黑龙江省',
  '江苏省', '浙江省', '安徽省', '福建省', '江西省', '山东省',
  '河南省', '湖北省', '湖南省', '广东省', '海南省',
  '四川省', '贵州省', '云南省', '陕西省', '甘肃省', '青海省',
  '台湾省', '内蒙古自治区', '广西壮族自治区', '西藏自治区',
  '宁夏回族自治区', '新疆维吾尔自治区', '香港特别行政区', '澳门特别行政区',
];

const Map<String, String> _provinceAliases = {
  '北京': '北京市', '天津': '天津市', '上海': '上海市', '重庆': '重庆市',
  '河北': '河北省', '山西': '山西省', '辽宁': '辽宁省', '吉林': '吉林省',
  '黑龙江': '黑龙江省', '江苏': '江苏省', '浙江': '浙江省', '安徽': '安徽省',
  '福建': '福建省', '江西': '江西省', '山东': '山东省', '河南': '河南省',
  '湖北': '湖北省', '湖南': '湖南省', '广东': '广东省', '海南': '海南省',
  '四川': '四川省', '贵州': '贵州省', '云南': '云南省', '陕西': '陕西省',
  '甘肃': '甘肃省', '青海': '青海省', '台湾': '台湾省',
  '内蒙古': '内蒙古自治区', '内蒙': '内蒙古自治区',
  '广西': '广西壮族自治区', '西藏': '西藏自治区',
  '宁夏': '宁夏回族自治区', '新疆': '新疆维吾尔自治区',
  '香港': '香港特别行政区', '澳门': '澳门特别行政区',
};

String normalizeProvince(String raw) {
  final value = raw.trim();
  if (value.isEmpty) return '';
  for (final province in provinces) {
    if (value.contains(province)) return province;
  }
  for (final entry in _provinceAliases.entries) {
    if (value.contains(entry.key)) return entry.value;
  }
  return '';
}

String inferProvince(String stem, List<String> options, String analysis) {
  return normalizeProvince('$stem\n${options.join('\n')}\n$analysis');
}
