# 数研组卷 Mobile v0.6.0

首个可安装的 Android 版本（Flutter），并补全题库中 31 道几何/统计题的配图。

## 主要更新

- **手机版源码**：`mobile/`，Flutter 3.47.7 / Dart 3.13.5
- **题库配图**：从 `数学精选题.docx` 提取 30 张原图，匹配 31 道缺图题；1 道圆锥变体按题干重绘示意图
- **AI 组卷**：默认标准 12 选择 + 4 填空 + 6 解答，题库不足时按实际题量生成
- **题库自动组卷**：不调用 AI、不需要 Key
- **手动组卷**：逐题勾选，支持搜索/题型/已导出/缺图筛选
- **Word 试卷导出**：A4 标准版式，可附答案解析；支持图片嵌入
- **已导出标记 / 最近做过**：导出后自动打标记并进入“最近做过”篮子
- **AI 供应商配置**：OpenAI Chat Completions 兼容接口，Key 存 Android Keystore
- **题库 JSON 导入**：按 ID 合并，重复跳过
- **启动修复**：首帧不再等待插件初始化；Android 回退 Skia
- **题目配图**：支持相册补图/换图，题库可按“缺图”筛选

## 下载

- `MathPaperStudio-Mobile-v0.6.0-arm64.apk`：绝大多数手机
- `MathPaperStudio-Mobile-v0.6.0-armeabi-v7a.apk`：老 32 位设备
- `MathPaperStudio-Mobile-v0.6.0-universal.apk`：不确定架构/兼容包

直接覆盖安装旧版即可，题库、配图、收藏和最近做过记录会保留。

## 已知限制

- 当前 APK 使用 debug 签名，适合直接安装/内测；上架应用商店需要正式签名
- 只导出 DOCX，暂不导出 PDF
- AI 识题/标注尚未接入，只完成供应商配置与 AI 组卷
- 没有答题计时和成绩记录；“最近做过”按导出时间记录
- 截止本次发布，尚未在 Android 真机上完成全部点按流程测试

## 测试

- `flutter analyze`：无问题
- 10 项 Flutter 测试通过
- DOCX 结构与嵌图测试通过，并用 LibreOffice 渲染验证
- 31 道配图题与 `figure_map.json` 的映射校验通过

## 数据

- 235 道题：`data/questions.json`、`mobile/assets/questions.json`
- 配图映射：`data/figure_map.json`、`mobile/assets/figure_map.json`
- 配图：`data/figures/`、`mobile/assets/figures/`
- 说明：[docs/question-figures.md](docs/question-figures.md)
