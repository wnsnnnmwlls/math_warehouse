import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AiModelOption {
  const AiModelOption(this.name, this.baseUrl);
  final String name;
  final String baseUrl;
}

const List<AiModelOption> aiModelOptions = [
  AiModelOption('deepseek-chat', 'https://api.deepseek.com'),
  AiModelOption('deepseek-flash', 'https://api.deepseek.com'),
  AiModelOption('deepseek-reasoner', 'https://api.deepseek.com'),
  AiModelOption('gpt-4o-mini', 'https://api.openai.com/v1'),
  AiModelOption('gpt-4o', 'https://api.openai.com/v1'),
  AiModelOption('qwen-vl-max', 'https://dashscope.aliyuncs.com/compatible-mode/v1'),
  AiModelOption('qwen-vl-plus', 'https://dashscope.aliyuncs.com/compatible-mode/v1'),
  AiModelOption('glm-4v-plus', 'https://open.bigmodel.cn/api/paas/v4'),
  AiModelOption('glm-4-flash', 'https://open.bigmodel.cn/api/paas/v4'),
];

class AiConfig {
  const AiConfig({
    this.baseUrl = 'https://api.deepseek.com',
    this.model = 'deepseek-chat',
    this.apiKey = '',
  });

  final String baseUrl;
  final String model;
  final String apiKey;

  AiConfig copyWith({String? baseUrl, String? model, String? apiKey}) {
    return AiConfig(
      baseUrl: baseUrl ?? this.baseUrl,
      model: model ?? this.model,
      apiKey: apiKey ?? this.apiKey,
    );
  }
}

class AiConfigStore {
  static const _secure = FlutterSecureStorage();
  static const _baseKey = 'ai_base_url';
  static const _modelKey = 'ai_model';
  static const _apiKeyKey = 'ai_api_key';

  Future<AiConfig> load() async {
    String base = 'https://api.deepseek.com';
    String model = 'deepseek-chat';
    String apiKey = '';
    try {
      final prefs = await SharedPreferences.getInstance();
      base = prefs.getString(_baseKey) ?? base;
      model = prefs.getString(_modelKey) ?? model;
    } catch (_) {}
    try {
      apiKey = await _secure.read(key: _apiKeyKey) ?? '';
    } catch (_) {}
    return AiConfig(baseUrl: base, model: model, apiKey: apiKey);
  }

  Future<void> save(AiConfig config) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_baseKey, config.baseUrl);
    await prefs.setString(_modelKey, config.model);
    try {
      if (config.apiKey.isEmpty) {
        await _secure.delete(key: _apiKeyKey);
      } else {
        await _secure.write(key: _apiKeyKey, value: config.apiKey);
      }
    } catch (_) {
      // Keep the non-secret part saved even if secure storage is unavailable.
    }
  }
}
