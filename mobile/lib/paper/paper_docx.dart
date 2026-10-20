import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:archive/archive.dart';
import 'package:image/image.dart' as img;
import 'package:path_provider/path_provider.dart';

import 'paper_model.dart';

class _DocxImage {
  const _DocxImage({
    required this.number,
    required this.bytes,
    required this.widthEmu,
    required this.heightEmu,
  });
  final int number;
  final Uint8List bytes;
  final int widthEmu;
  final int heightEmu;

  String get relationshipId => 'rIdImage$number';
  String get fileName => 'image$number.png';
}

String _xmlEscape(String value) {
  return value
      .replaceAll('&', '&amp;')
      .replaceAll('<', '&lt;')
      .replaceAll('>', '&gt;')
      .replaceAll('"', '&quot;')
      .replaceAll("'", '&apos;');
}

String _runs(String text, {int size = 24, bool bold = false, String font = '宋体'}) {
  final props =
      '<w:rPr><w:rFonts w:ascii="Times New Roman" w:hAnsi="Times New Roman" w:eastAsia="$font"/>'
      '${bold ? '<w:b/>' : ''}<w:sz w:val="$size"/><w:szCs w:val="$size"/></w:rPr>';
  final pieces = text.split('\n');
  final buffer = StringBuffer();
  for (var i = 0; i < pieces.length; i++) {
    if (i > 0) buffer.write('<w:r>$props<w:br/></w:r>');
    buffer.write('<w:r>$props<w:t xml:space="preserve">${_xmlEscape(pieces[i])}</w:t></w:r>');
  }
  return buffer.toString();
}

String _p(
  String text, {
  int size = 24,
  bool bold = false,
  String font = '宋体',
  String align = 'left',
  int after = 100,
  int before = 0,
  int line = 360,
}) {
  return '<w:p><w:pPr><w:spacing w:before="$before" w:after="$after" w:line="$line" '
      'w:lineRule="auto"/><w:jc w:val="$align"/></w:pPr>'
      '${_runs(text, size: size, bold: bold, font: font)}</w:p>';
}

String _blank({int after = 120}) => '<w:p><w:pPr><w:spacing w:after="$after"/></w:pPr></w:p>';

String _optionParagraph(String a, String b) {
  return '<w:p><w:pPr><w:tabs><w:tab w:val="left" w:pos="4200"/></w:tabs>'
      '<w:spacing w:after="80" w:line="360" w:lineRule="auto"/></w:pPr>'
      '${_runs(a)}<w:r><w:tab/></w:r>${_runs(b)}</w:p>';
}

String _imageParagraph(_DocxImage image) {
  final n = image.number;
  return '<w:p><w:pPr><w:jc w:val="center"/><w:spacing w:before="60" w:after="100"/></w:pPr>'
      '<w:r><w:drawing><wp:inline distT="0" distB="0" distL="0" distR="0">'
      '<wp:extent cx="${image.widthEmu}" cy="${image.heightEmu}"/>'
      '<wp:docPr id="$n" name="Picture $n" descr="题目配图"/>'
      '<a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture">'
      '<pic:pic><pic:nvPicPr><pic:cNvPr id="$n" name="${image.fileName}"/><pic:cNvPicPr/></pic:nvPicPr>'
      '<pic:blipFill><a:blip r:embed="${image.relationshipId}"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>'
      '<pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="${image.widthEmu}" cy="${image.heightEmu}"/></a:xfrm>'
      '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr></pic:pic>'
      '</a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p>';
}

String _buildDocument(
  Paper paper, {
  bool includeAnswers = true,
  Map<String, _DocxImage> images = const {},
}) {
  final body = StringBuffer();
  body.write(_p(paper.title, size: 36, bold: true, font: '黑体', align: 'center', after: 100));
  body.write(_p('考试时间：${paper.minutes}分钟    满分：${paper.totalPoints}分',
      size: 21, align: 'center', after: 60));
  body.write(_p('班级：____________    姓名：____________    学号：____________',
      size: 21, align: 'center', after: 220));
  if (paper.note.trim().isNotEmpty) {
    body.write(_p(paper.note.trim(), size: 20, align: 'left', after: 180));
  }

  var number = 1;
  for (final section in paper.sections) {
    body.write(_p(section.title, size: 28, bold: true, font: '黑体', after: 60, before: 100));
    body.write(_p(section.instruction, size: 20, after: 120));
    for (final item in section.questions) {
      final q = item.question;
      body.write(_p('$number．${q.stem}', size: 24, after: 80));
      number++;
      final image = images[q.id];
      if (image != null) {
        body.write(_imageParagraph(image));
      } else if (q.needsFigure) {
        body.write(_p('【配图缺失：原题提到“如图”，题库中未收录配图】',
            size: 20, align: 'center', after: 100));
      }
      if (q.options.isNotEmpty) {
        final options = [
          for (var i = 0; i < q.options.length; i++)
            '${String.fromCharCode(65 + i)}．${q.options[i]}',
        ];
        for (var i = 0; i < options.length; i += 2) {
          body.write(_optionParagraph(
            options[i],
            i + 1 < options.length ? options[i + 1] : '',
          ));
        }
      }
      if (q.type == 'solution') {
        body.write(_blank(after: 420));
        body.write(_blank(after: 420));
      } else if (q.type == 'fill') {
        body.write(_blank(after: 260));
      }
    }
  }

  if (includeAnswers) {
    body.write('<w:p><w:r><w:br w:type="page"/></w:r></w:p>');
    body.write(_p('参考答案与解析', size: 32, bold: true, font: '黑体', align: 'center', after: 180));
    var answerNumber = 1;
    for (final section in paper.sections) {
      body.write(_p(section.title, size: 26, bold: true, font: '黑体', after: 80, before: 100));
      for (final item in section.questions) {
        final q = item.question;
        body.write(_p('$answerNumber．答案：${q.answer.isEmpty ? '略' : q.answer}',
            size: 22, bold: true, after: 40));
        if (q.analysis.trim().isNotEmpty) {
          body.write(_p('解析：${q.analysis.trim()}', size: 21, after: 140));
        }
        answerNumber++;
      }
    }
  }

  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" '
      'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
      'xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing" '
      'xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
      'xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">'
      '<w:body>${body.toString()}'
      '<w:sectPr><w:pgSz w:w="11906" w:h="16838"/>'
      '<w:pgMar w:top="1134" w:right="1134" w:bottom="1134" w:left="1134" '
      'w:header="720" w:footer="720" w:gutter="0"/></w:sectPr>'
      '</w:body></w:document>';
}

String _contentTypes() {
  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
      '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
      '<Default Extension="xml" ContentType="application/xml"/>'
      '<Default Extension="png" ContentType="image/png"/>'
      '<Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>'
      '<Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>'
      '<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>'
      '<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>'
      '</Types>';
}

String _rootRels() {
  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
      '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>'
      '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>'
      '<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>'
      '</Relationships>';
}

String _documentRels(List<_DocxImage> images) {
  final buffer = StringBuffer('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
      '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>');
  for (final image in images) {
    buffer.write('<Relationship Id="${image.relationshipId}" '
        'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" '
        'Target="media/${image.fileName}"/>');
  }
  buffer.write('</Relationships>');
  return buffer.toString();
}

String _styles() {
  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">'
      '<w:docDefaults><w:rPrDefault><w:rPr>'
      '<w:rFonts w:ascii="Times New Roman" w:hAnsi="Times New Roman" w:eastAsia="宋体"/>'
      '<w:sz w:val="24"/><w:szCs w:val="24"/></w:rPr></w:rPrDefault>'
      '<w:pPrDefault><w:pPr><w:spacing w:after="100" w:line="360" w:lineRule="auto"/></w:pPr></w:pPrDefault>'
      '</w:docDefaults>'
      '<w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>'
      '</w:styles>';
}

String _coreProps(String title) {
  final now = DateTime.now().toUtc().toIso8601String();
  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" '
      'xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" '
      'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">'
      '<dc:title>${_xmlEscape(title)}</dc:title><dc:creator>数研组卷</dc:creator>'
      '<cp:lastModifiedBy>数研组卷</cp:lastModifiedBy>'
      '<dcterms:created xsi:type="dcterms:W3CDTF">$now</dcterms:created>'
      '<dcterms:modified xsi:type="dcterms:W3CDTF">$now</dcterms:modified>'
      '</cp:coreProperties>';
}

String _appProps() {
  return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
      '<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" '
      'xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">'
      '<Application>数研组卷 Mobile</Application></Properties>';
}

Uint8List buildDocxBytes(Paper paper, {bool includeAnswers = true}) {
  final archive = Archive();
  final images = <String, _DocxImage>{};
  var imageNumber = 1;
  for (final section in paper.sections) {
    for (final item in section.questions) {
      final question = item.question;
      if (!question.hasImage || images.containsKey(question.id)) continue;
      try {
        final decoded = img.decodeImage(base64Decode(question.imageBase64));
        if (decoded == null || decoded.width == 0 || decoded.height == 0) continue;
        final naturalWidth = decoded.width * 9525.0;
        final naturalHeight = decoded.height * 9525.0;
        const maxWidth = 4800000.0;
        const maxHeight = 3400000.0;
        final scale = math.min(1.0, math.min(maxWidth / naturalWidth, maxHeight / naturalHeight));
        images[question.id] = _DocxImage(
          number: imageNumber,
          bytes: Uint8List.fromList(img.encodePng(decoded)),
          widthEmu: (naturalWidth * scale).round(),
          heightEmu: (naturalHeight * scale).round(),
        );
        imageNumber++;
      } catch (_) {
        // Keep the paper exportable even if one embedded image is corrupt.
      }
    }
  }

  void add(String name, String content) {
    final bytes = utf8.encode(content);
    archive.addFile(ArchiveFile(name, bytes.length, bytes));
  }

  void addBytes(String name, List<int> bytes) {
    archive.addFile(ArchiveFile(name, bytes.length, bytes));
  }

  add('[Content_Types].xml', _contentTypes());
  add('_rels/.rels', _rootRels());
  add('word/document.xml',
      _buildDocument(paper, includeAnswers: includeAnswers, images: images));
  add('word/styles.xml', _styles());
  add('word/_rels/document.xml.rels', _documentRels(images.values.toList()));
  for (final image in images.values) {
    addBytes('word/media/${image.fileName}', image.bytes);
  }
  add('docProps/core.xml', _coreProps(paper.title));
  add('docProps/app.xml', _appProps());
  final encoded = ZipEncoder().encode(archive);
  return Uint8List.fromList(encoded);
}

Future<File> writePaperDocx(Paper paper, {bool includeAnswers = true}) async {
  final dir = await getTemporaryDirectory();
  final safe = paper.title.replaceAll(RegExp(r'[\\/:*?"<>|]'), '_').trim();
  final file = File('${dir.path}/${safe.isEmpty ? '数学试卷' : safe}.docx');
  await file.writeAsBytes(buildDocxBytes(paper, includeAnswers: includeAnswers), flush: true);
  return file;
}
