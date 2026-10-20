# 数研组卷 v1.2.1 · 桌面补全 + 手机

本次在 v1.2.0 基础上继续补全题库和桌面端编辑器。

## 桌面版 v1.2.1

- 修复/补全 4 道“选项是图片”的选择题：
  - `q_6906f063601c`：题干二次函数图 + A/B/C/D 四个选项图
  - `q_eec071b51c7d`：A/B/C/D 四个选项图
  - `q_febac82a8d17`、`q_bbad4b8249d8`：补入选项图
- 题库搜索支持 **题目 ID**
- 列表新增红色 **“待补全”** 标记
- 新增题目完整性审计：
  - 机器可读：`data/question-audit.json`
  - 说明：[docs/question-audit.md](docs/question-audit.md)
- 仍保留 v1.2 的题目配图编辑、列表标记和 Word 嵌图

文件：

- `MathPaperStudio-v1.2.1-Windows-x64.zip`：解压后运行，包含题库数据（推荐）
- `MathPaperStudio-v1.2.1.exe`：单文件程序

> 程序未签名，Windows SmartScreen 可能提示“未知发布者”，选择“仍要运行”。

## 手机版 v0.6.0

本次 Release 继续附带手机 APK（v0.6.0）：

- `MathPaperStudio-Mobile-v0.6.0-arm64-v1.2.1.apk`
- `MathPaperStudio-Mobile-v0.6.0-armeabi-v7a-v1.2.1.apk`
- `MathPaperStudio-Mobile-v0.6.0-universal-v1.2.1.apk`

## 仍需人工确认的题目

审计共 7 条：

- 高优先级：`q_dc54f6c11778`、`q_dd986daa2278`、`q_dacdc1486a72`（选项缺失/答案不是 A–D）
- 中优先级：`q_febac82a8d17`、`q_bbad4b8249d8`（已补选项图，答案仍是文字）、`q_7320d153bba1`（只有 3 个选项）
- 低优先级：`q_71b7e4c25cb9`（多小问合并，D 空）

## 验证

- 桌面端 `selftest.exe`：核心测试全部通过
- 桌面端带配图 DOCX：生成并由 LibreOffice 渲染通过
- 手机端 `flutter analyze`：无问题
- 手机端 10 项测试通过
- 桌面版/手机版数据已同步
