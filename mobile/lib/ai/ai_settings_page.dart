import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

import 'ai_config.dart';

class AiSettingsPage extends StatefulWidget {
  const AiSettingsPage({super.key});

  @override
  State<AiSettingsPage> createState() => _AiSettingsPageState();
}

class _AiSettingsPageState extends State<AiSettingsPage> {
  final _store = AiConfigStore();
  final _base = TextEditingController();
  final _model = TextEditingController();
  final _key = TextEditingController();
  String _selectedModel = 'deepseek-chat';
  bool _obscure = true;
  bool _loading = true;
  bool _testing = false;
  String? _status;
  bool _statusError = false;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    final config = await _store.load();
    if (!mounted) return;
    setState(() {
      _base.text = config.baseUrl;
      _model.text = config.model;
      _key.text = config.apiKey;
      _selectedModel = aiModelOptions.any((e) => e.name == config.model)
          ? config.model
          : 'deepseek-chat';
      _loading = false;
    });
  }

  @override
  void dispose() {
    _base.dispose();
    _model.dispose();
    _key.dispose();
    super.dispose();
  }

  AiConfig get _config => AiConfig(
        baseUrl: _base.text.trim().isEmpty
            ? 'https://api.deepseek.com'
            : _base.text.trim(),
        model: _model.text.trim().isEmpty ? _selectedModel : _model.text.trim(),
        apiKey: _key.text.trim(),
      );

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Scaffold(
      appBar: AppBar(title: const Text('AI 供应商配置')),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : ListView(
              padding: const EdgeInsets.fromLTRB(16, 8, 16, 30),
              children: [
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text('选择模型', style: TextStyle(fontSize: 14, fontWeight: FontWeight.w800)),
                        const SizedBox(height: 10),
                        DropdownButtonFormField<String>(
                          initialValue: _selectedModel,
                          isExpanded: true,
                          decoration: const InputDecoration(labelText: '模型'),
                          items: aiModelOptions
                              .map((option) => DropdownMenuItem(
                                    value: option.name,
                                    child: Text(option.name),
                                  ))
                              .toList(),
                          onChanged: (value) {
                            if (value == null) return;
                            setState(() {
                              _selectedModel = value;
                              _model.text = value;
                              final option = aiModelOptions.firstWhere((e) => e.name == value);
                              _base.text = option.baseUrl;
                            });
                          },
                        ),
                        const SizedBox(height: 12),
                        TextField(
                          controller: _base,
                          keyboardType: TextInputType.url,
                          decoration: const InputDecoration(
                            labelText: 'API 地址',
                            hintText: 'https://api.deepseek.com',
                          ),
                        ),
                        const SizedBox(height: 12),
                        TextField(
                          controller: _model,
                          decoration: const InputDecoration(
                            labelText: '模型名称',
                            hintText: 'deepseek-chat',
                          ),
                        ),
                        const SizedBox(height: 12),
                        TextField(
                          controller: _key,
                          obscureText: _obscure,
                          decoration: InputDecoration(
                            labelText: 'API 密钥',
                            hintText: 'sk-...',
                            suffixIcon: IconButton(
                              onPressed: () => setState(() => _obscure = !_obscure),
                              icon: Icon(_obscure ? Icons.visibility_outlined : Icons.visibility_off_outlined),
                            ),
                          ),
                        ),
                        const SizedBox(height: 8),
                        Text(
                          '密钥只保存在本机安全存储（Android Keystore）中，不会进入题库 JSON。',
                          style: TextStyle(fontSize: 11, color: scheme.onSurfaceVariant),
                        ),
                      ],
                    ),
                  ),
                ),
                if (_status != null) ...[
                  const SizedBox(height: 12),
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
                      ),
                    ),
                  ),
                ],
                const SizedBox(height: 16),
                Row(
                  children: [
                    Expanded(
                      child: FilledButton.icon(
                        onPressed: _testing ? null : _test,
                        icon: _testing
                            ? const SizedBox(
                                width: 15,
                                height: 15,
                                child: CircularProgressIndicator(strokeWidth: 2),
                              )
                            : const Icon(Icons.wifi_tethering),
                        label: Text(_testing ? '连接中…' : '测试连接'),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: OutlinedButton.icon(
                        onPressed: _save,
                        icon: const Icon(Icons.save_outlined),
                        label: const Text('保存配置'),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(14),
                    child: Text(
                      '配置完成后，AI 识题、AI 标注和 AI 考试会读取这里的供应商设置。'
                      '当前 v0.2 先完成配置与连接测试，识题功能在下一阶段接入。',
                      style: TextStyle(fontSize: 12, color: scheme.onSurfaceVariant, height: 1.5),
                    ),
                  ),
                ),
              ],
            ),
    );
  }

  Future<void> _save() async {
    await _store.save(_config);
    if (!mounted) return;
    setState(() {
      _status = '配置已保存。';
      _statusError = false;
    });
  }

  Future<void> _test() async {
    final config = _config;
    final base = config.baseUrl.replaceAll(RegExp(r'/+$'), '');
    final uri = Uri.tryParse('$base/chat/completions');
    if (uri == null || !uri.hasScheme) {
      setState(() {
        _status = 'API 地址格式不正确。';
        _statusError = true;
      });
      return;
    }
    setState(() {
      _testing = true;
      _status = null;
    });
    try {
      final response = await http
          .post(
            uri,
            headers: {
              'Content-Type': 'application/json',
              if (config.apiKey.isNotEmpty) 'Authorization': 'Bearer ${config.apiKey}',
            },
            body: jsonEncode({
              'model': config.model,
              'messages': [
                {'role': 'user', 'content': '请回复 OK'}
              ],
              'max_tokens': 16,
            }),
          )
          .timeout(const Duration(seconds: 25));
      if (!mounted) return;
      setState(() {
        _statusError = response.statusCode < 200 || response.statusCode >= 300;
        _status = _statusError
            ? '连接失败：HTTP ${response.statusCode}\n${response.body.length > 300 ? response.body.substring(0, 300) : response.body}'
            : '连接成功，模型返回正常。';
      });
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _statusError = true;
        _status = '连接失败：$error';
      });
    } finally {
      if (mounted) setState(() => _testing = false);
    }
  }
}
