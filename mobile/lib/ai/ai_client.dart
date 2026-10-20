import 'dart:convert';

import 'package:http/http.dart' as http;

import 'ai_config.dart';

class AiChatReply {
  const AiChatReply({required this.ok, this.text = '', this.error = ''});
  final bool ok;
  final String text;
  final String error;
}

String aiChatEndpoint(String baseUrl) {
  var base = baseUrl.trim().replaceAll(RegExp(r'/+$'), '');
  if (base.endsWith('/chat/completions')) return base;
  return '$base/chat/completions';
}

Future<AiChatReply> askAi({
  required AiConfig config,
  required String systemPrompt,
  required String userPrompt,
  int maxTokens = 2200,
  Duration timeout = const Duration(seconds: 120),
}) async {
  final uri = Uri.tryParse(aiChatEndpoint(config.baseUrl));
  if (uri == null || !uri.hasScheme) {
    return const AiChatReply(ok: false, error: 'API 地址格式不正确');
  }
  if (config.model.trim().isEmpty) {
    return const AiChatReply(ok: false, error: '请先选择或填写模型');
  }
  try {
    final response = await http
        .post(
          uri,
          headers: {
            'Content-Type': 'application/json',
            if (config.apiKey.trim().isNotEmpty)
              'Authorization': 'Bearer ${config.apiKey.trim()}',
          },
          body: jsonEncode({
            'model': config.model.trim(),
            'messages': [
              {'role': 'system', 'content': systemPrompt},
              {'role': 'user', 'content': userPrompt},
            ],
            'temperature': 0.2,
            'max_tokens': maxTokens,
          }),
        )
        .timeout(timeout);
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = response.body.length > 500
          ? response.body.substring(0, 500)
          : response.body;
      return AiChatReply(
        ok: false,
        error: 'HTTP ${response.statusCode}：$body',
      );
    }
    final decoded = jsonDecode(utf8.decode(response.bodyBytes));
    var content = '';
    if (decoded is Map<String, dynamic>) {
      final choices = decoded['choices'];
      if (choices is List && choices.isNotEmpty) {
        final message = choices.first is Map ? (choices.first as Map)['message'] : null;
        if (message is Map) content = message['content']?.toString() ?? '';
      }
    }
    if (content.trim().isEmpty) {
      return const AiChatReply(ok: false, error: 'AI 返回内容为空');
    }
    return AiChatReply(ok: true, text: content);
  } catch (error) {
    return AiChatReply(ok: false, error: error.toString());
  }
}
