# 数研组卷 v1.2.0 · 桌面 + 手机

本次同时发布桌面版 v1.2 和手机版 v0.6.0，并整合补全后的题库配图。

## 桌面版 v1.2

- 题目配图支持：
  - 编辑器右侧可**选择图片 / 更换图片 / 清除图片**
  - 支持 PNG / JPG / JPEG / BMP / GIF，自动缩放并压缩
  - 题库列表显示“配图 / 缺图”标记
  - 导出的 Word 试卷自动嵌入配图
  - 缺图题在试卷中明确显示“配图缺失”，不会静默生成残卷
- 题库数据已直接写入 **31 道题的 `image_base64`**
- `data/questions.json` 打开即有图；无需额外迁移
- 桌面版版本号更新为 **v1.2**
- 核心自测通过，配图 DOCX 已用 LibreOffice 实际渲染验证

文件：

- `MathPaperStudio-v1.2-Windows-x64.zip`：解压后运行 `MathPaperStudio.exe`，包含 `data/questions.json` 和 `data/tags.json`
- `MathPaperStudio-v1.2.exe`：单文件程序；首次运行会在同目录生成/使用 `data`

> 程序未做代码签名，Windows SmartScreen 可能提示“未知发布者”，请选择“仍要运行”。

## 手机版 v0.6.0

手机版源码和 31 道题配图已在 `mobile/`，本次 Release 同时附上 APK：

- `MathPaperStudio-Mobile-v0.6.0-arm64-v1.2.0.apk`：绝大多数手机
- `MathPaperStudio-Mobile-v0.6.0-armeabi-v7a-v1.2.0.apk`：老 32 位设备
- `MathPaperStudio-Mobile-v0.6.0-universal-v1.2.0.apk`：兼容包

手机版功能：

- 235 题题库、搜索与筛选、收藏
- JSON 导入
- AI 供应商配置
- AI 组卷 / 题库自动组卷 / 手动逐题组卷
- 标准 A4 Word 试卷导出，可附答案解析
- 导出题自动标记“已导出”，进入“最近做过”
- 31 道几何/统计题配图，支持相册补图/换图

## 题库配图

- 从 `数学精选题.docx` 提取 30 张原图
- 31 道缺图题全部匹配
- 其中 3 组同图变体复用原图；1 道圆锥变体按题干重绘示意图
- 详见 [docs/question-figures.md](docs/question-figures.md)

## 验证

- 桌面端 `selftest.exe`：核心测试全部通过
- 桌面端 DOCX 配图：生成并由 LibreOffice 渲染通过
- 手机端 `flutter analyze`：无问题
- 手机端 10 项测试通过
- 手机 APK 构建成功

## 说明

- 桌面版和手机版的数据目录、程序形态不同，分别安装/运行
- 桌面版当前仍是 C++/Win32，手机版是 Flutter；本次 Release 放在一起方便下载
- 手机 APK 使用 debug 签名，适合直接安装/内测；上架应用商店需要正式签名
- 桌面版未签名，适合直接运行/分发内测
