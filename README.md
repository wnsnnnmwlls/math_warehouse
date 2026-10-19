# 数研组卷 · MathPaperStudio

一个使用 **C++17 + Win32 + GDI+** 开发的本地桌面数学组卷程序，支持清晰渲染和日间/夜间双主题。它把题库、AI 标签库和试卷导出串成一条完整流程：

**拍照/截图上传题目 → AI 识别题目并提取公式/知识点 → 按难度与标签筛选 → 勾选组卷 → 导出 A4 Word 试卷**

![题库界面](docs/screenshot-bank.png)

## 快速开始

1. 双击 `MathPaperStudio.exe`，或双击 `启动数研组卷.bat`。
2. 首次启动会内置 24 道示例题，覆盖 A/B/C 三个难度，包含选择题、填空题和解答题。
3. 可以直接点题库右上角的“**图片识题**”，一次选择一张或多张题目照片/截图。AI 会提取图片中的**所有题目**，并为每道题自动写出答案、解析、难度和公式/知识点标签，确认后批量加入题库。
4. 侧边栏底部的“**夜间模式**”开关可随时切换深色外观，选择会自动保存，下次启动仍然生效。
5. 在“试卷合成”中设置标题和输出选项，导出 `.docx`，用 Word / WPS 打开即可打印。

运行环境：Windows 10 / 11 x64。程序无第三方 DLL 依赖，直接运行；首次启动会在程序目录生成 `data` 文件夹。

## 功能

### 1. 数学题库

- 三个难度：**A 基础 / B 提升 / C 挑战**。
- 三种题型：选择题、填空题、解答题。
- 支持新增、编辑、删除题目；选择题支持 A–D 四个选项。
- 题目、答案、解析、公式标签和知识点标签全部保存在本地。
- 选择题选项按两栏排版导出；解答题可预留书写空间。

### 2. AI 接口与标签容器

- 兼容 **OpenAI Chat Completions** 协议，可接入：
  - DeepSeek
  - OpenAI
  - Ollama、vLLM、LM Studio 等本地/自托管服务
  - 其他 OpenAI 兼容服务
- 配置项：API 地址、模型名称、API 密钥；提供 DeepSeek / OpenAI 快速填充。
- API 密钥使用 **Windows DPAPI** 加密，只在当前 Windows 用户下可解密，不会以明文写入磁盘。
- **图片识题（支持多图、多题）**：
  - 在题库右上角点击“图片识题”，一次可以选择 1–16 张 PNG / JPG / JPEG / BMP / GIF 图片；
  - 程序会按需缩放图片、纠正 EXIF 方向并压缩后，以 OpenAI 视觉消息格式发送给模型；
  - 一张图片中包含多道题时，AI 会按顺序全部提取，而不是只提取第一道；
  - AI 会为**每一道题独立求解**，自动生成答案和解析，即使原图中没有答案；
  - 同一道题的多个小问（如 (1)(2)）会合并为一道题，答案解析中分别回答；
  - 识别结果会先显示确认框（题目数量、缺失答案/解析提醒），确认后批量加入题库顶部；
  - 图片识题必须使用**支持图片输入（Vision）且推理能力较好的模型**，例如 `gpt-4o-mini`、`qwen-vl-*`、`glm-4v` 等；纯文本模型会返回接口错误；
  - 如果模型返回 LaTeX，程序会自动转换为可读的 Unicode 数学文本（如把 `\frac{\sqrt{2}}{2}` 转成 `√2/2`，把 `\frac{\cos 2\alpha}{\sin\alpha+\cos\alpha}` 转成 `(cos 2α)/(sin α+cos α)`）。
- 图片只用于本次 AI 请求，不会保存到本地题库；题库里保存的是 AI 提取后的题目、答案解析和标签。

![AI 自动生成答案与解析](docs/screenshot-ai-answers.png)

- AI 标签标注会读取题干与选项，输出严格的 JSON：
  ```json
  {"formulas":["余弦定理"],"concepts":["解三角形","三角函数"]}
  ```
- 标签自动写入两个独立分类：
  - **公式标签**（公式、定理、运算名称）
  - **知识点标签**（知识概念、方法、题型）
- 已有标签会作为提示词上下文发送给模型，便于复用同一套标签体系。
- AI 标注是逐题请求的，界面会显示进度；失败时保留原标签并给出错误原因。

#### 常用配置示例

| 服务 | API 地址 | 模型名称 |
| --- | --- | --- |
| DeepSeek | `https://api.deepseek.com` | `deepseek-chat` |
| OpenAI | `https://api.openai.com/v1` | `gpt-4o-mini` |
| Ollama | `http://127.0.0.1:11434/v1` | 例如 `qwen2.5:7b` |

> 只有 API 地址和模型名称必填。使用本地服务时，API 密钥可以留空。
> 点击“测试连接”会发送一条最小请求，可能产生极少量的接口费用。
> 图片识题使用的是同一个模型配置，因此需要把“模型名称”设置为支持图片输入的视觉模型；
> 如果只想用纯文本模型做标签，则不要使用“图片识题”功能。

### 3. 筛选与自主选择题

- 关键词搜索题干、答案、解析与标签。
- 难度多选、题型多选。
- 标签多选；支持：
  - **任一匹配**：选中“三角函数”或“集合”中任一标签的题；
  - **全部匹配**：题目必须同时拥有选中的所有标签。
- “只看待标注”可筛出还没有标签的题目。
- 点击标签容器中的任意标签，会回到题库并自动应用该标签筛选。
- 不限制选择数量；选中的题会进入底部组卷篮，并实时统计各题型与预计总分。

### 4. 试卷合成与导出

- 在“试卷合成”设置试卷标题、考试时间、注意事项。
- 可勾选：
  - 生成参考答案；
  - 附详细解析；
  - 解答题预留书写空间。
- Word 文档版式：
  - A4 纵向，页边距 2.0 / 2.2 cm；
  - 标题黑体、正文宋体、页脚自动页码；
  - 按“选择题 → 填空题 → 解答题”分节；
  - 题号连续，组内按 A → C 难度排序；
  - 选择题选项两栏排列，避免原生表格边框；
  - 参考答案与解析自动另起一页。
- 默认分值：选择题 5 分、填空题 5 分、解答题 12 分；满分按已选题目自动计算。

示例输出：[docs/示例试卷.docx](docs/示例试卷.docx)

![试卷预览](docs/示例试卷_第1页.png)

### 5. 界面清晰度与夜间模式

- 界面使用 **ClearType 文字渲染 + 原生像素级双缓冲拷贝**，不再经过位图缩放或二次采样，文字和 1 像素边框更锐利。
- 支持 **日间 / 夜间双主题**：侧边栏底部开关即时切换，自定义绘制区域、原生输入框、滚动条和窗口标题栏会同步换色。
- 夜间模式偏好保存在 `data/settings.json` 的 `dark_mode` 字段，重启后保持上次选择。
- 试卷版式预览中的“纸张”始终保持白色，便于在夜间模式下准确预览打印效果。

![夜间模式](docs/screenshot-bank-dark.png)

![夜间模式编辑器](docs/screenshot-editor-dark.png)

## 使用流程建议

1. 在“AI 接口”填写并保存 API 配置，点击“测试连接”确认可用。
2. 在“题库管理”点击“图片识题”选择一张或多张题目图片；AI 会提取全部题目并生成答案与解析，
   确认后批量导入，新题会出现在题库顶部。
   也可以手动新增题目，或直接使用内置示例题。
3. 已有题目可以在列表中勾选，点击“AI 智能标注”补全/更新公式和知识点标签。
4. 打开“标签容器”检查归档结果；以后可按标签反向筛题。
5. 勾选要进入试卷的题目，点击底部“前往组卷”。
6. 设置标题、时间和注意事项，导出 `.docx`。

## 本地数据

所有数据位于程序同目录下的 `data`：

| 文件 | 内容 |
| --- | --- |
| `data/questions.json` | 题库、难度、题型、选项、答案、解析、标签 |
| `data/tags.json` | 公式标签容器与知识点标签容器 |
| `data/settings.json` | API 地址、模型、夜间模式；密钥为 Windows DPAPI 密文 |

- 备份或迁移时，直接复制整个 `data` 目录。
- 如果题库文件损坏，程序会自动备份原文件并载入示例题库。
- 删除 `data` 文件夹后重启，会重新生成示例题库和默认设置。

## 目录结构

```
MathPaperStudio/
├─ MathPaperStudio.exe         编译好的 Windows 程序
├─ 启动数研组卷.bat
├─ data/                       题库、标签库、AI 配置
├─ docs/                       截图、示例试卷、页面预览
├─ src/
│  ├─ main.cpp                 程序入口、COM/GDI+/DPI 初始化
│  ├─ app.h / app.cpp          主窗口、页面与业务调度
│  ├─ ui.h / ui.cpp            自绘 UI 工具层与主题
│  ├─ editor.h / editor.cpp    题目编辑弹窗
│  ├─ ai.h / ai.cpp            AI 标签、图片识题与 JSON 解析
│  ├─ image_data.h / .cpp      图片解码、EXIF、压缩与 Base64
│  ├─ math_text.h / .cpp       LaTeX 到可读 Unicode 数学文本
│  ├─ model.h / model.cpp      题目、标签、筛选等核心数据模型
│  ├─ seed.h / seed.cpp        内置示例题库数据
│  ├─ store.h / store.cpp      JSON 持久化、DPAPI、损坏恢复
│  ├─ json.h / json.cpp        轻量 JSON 解析器/序列化器
│  ├─ docx.h / docx.cpp        OOXML 试卷生成与 ZIP 封装
│  ├─ util.h / util.cpp        UTF/路径/时间/文件/Base64 工具
│  └─ selftest.cpp             核心功能自测
├─ res/                        图标、资源与 DPI 清单
├─ CMakeLists.txt
├─ build.ps1
└─ build/
   ├─ selftest.exe             核心自测程序
   └─ ...                      CMake 中间文件
```

## 代码结构与模块职责

这个项目是**单进程的 Win32 桌面程序**，没有 Qt、Electron、浏览器或第三方 UI 框架。界面使用“立即模式绘制”：每帧根据当前状态重新画整个界面，并把可点击区域记录到 `hits_` 中；只有需要输入文字时才创建原生 `EDIT` 控件。

### 各模块的作用

| 模块 | 主要职责 | 关键内容 |
| --- | --- | --- |
| `src/main.cpp` | 程序入口 | `wWinMain`；设置 DPI awareness；初始化 COM 和 GDI+；创建 `App` 并进入消息循环。 |
| `src/app.h` / `app.cpp` | 主窗口与应用调度 | 四个页面（题库、标签、AI、试卷）；所有 `WM_*` 消息；鼠标命中/hover/滚动；筛选、勾选组卷；AI 标注和图片识题的线程调度；批量导入；主题切换；导出入口；Toast 提示。 |
| `src/ui.h` / `ui.cpp` | 自绘 UI 工具层 | `Rect`、`Theme`、`Canvas`；字体缓存、圆角矩形、线段、裁剪、文本测量/省略号、复选框；ClearType 文字渲染；DWM 原生标题栏明暗适配。 |
| `src/editor.h` / `editor.cpp` | 题目编辑器 | 独立模态窗口；题干/选项/答案/解析/标签的原生输入框；选择题/填空题/解答题切换；A/B/C 难度；保存校验；未保存提示；删除题目；暗色主题。 |
| `src/ai.h` / `ai.cpp` | AI 能力 | OpenAI Chat Completions 兼容请求；文本标签提示词；视觉请求（`image_url` data URL）；多题 `questions[]` 解析；答案/解析生成约束；标签去重；错误与超时处理。 |
| `src/image_data.h` / `image_data.cpp` | 图片预处理 | 使用 GDI+ 读取 PNG/JPEG/BMP/GIF；按 EXIF 方向纠正；限制最长边并缩放；白底合成；JPEG 编码；Base64 data URL。 |
| `src/math_text.h` / `math_text.cpp` | 数学文本清洗 | 去除 `$…$`、`\(…\)`；递归解析 `\frac`、`\sqrt`、希腊字母、常用符号、上下标；把 LaTeX 转换成人类可读的 Unicode 数学文本。 |
| `src/model.h` / `model.cpp` | 核心数据模型 | `Question`、`TagLibrary`、`Settings`、`QuestionFilter`；题型/难度枚举；标签合并、使用次数统计；题目筛选逻辑。 |
| `src/seed.h` / `seed.cpp` | 内置示例数据 | 24 道示例题、27 个公式标签、52 个知识点标签；首次启动或数据损坏时用于初始化。 |
| `src/store.h` / `store.cpp` | 本地数据层 | 加载/保存 `questions.json`、`tags.json`、`settings.json`；字段校验与默认值；原子写入；API Key 的 Windows DPAPI 加密；文件损坏备份与恢复。 |
| `src/json.h` / `json.cpp` | JSON 基础设施 | 自行实现的 JSON AST；解析、转义、Unicode `\uXXXX`、数组/对象访问、紧凑/缩进输出。 |
| `src/docx.h` / `docx.cpp` | Word 试卷生成 | 直接写 OOXML；A4 页面/页边距/字体/页脚；按选择题→填空题→解答题分节；A→C 排序；选项制表位两栏；答案页分页；写入自定义 Store-only ZIP。 |
| `src/util.h` / `util.cpp` | 通用工具 | UTF-8/UTF-16 转换、Trim、标签/行拆分、文件原子写入、可执行目录/桌面/图片目录、时间与 ID、Base64、整数夹取。 |
| `src/selftest.cpp` | 自测程序 | 验证 JSON、题库/标签、筛选、DOCX、图片编码、Store 读写、DPAPI、暗色偏好持久化等核心路径，不启动图形界面。 |
| `CMakeLists.txt` / `build.ps1` | 构建系统 | C++17、MinGW-w64、静态链接；同时构建 `MathPaperStudio.exe` 和 `build\selftest.exe`；资源文件包含图标和 DPI manifest。 |
| `res/app.rc`、`app.manifest`、`app.ico` | Windows 资源 | 程序图标、版本/清单资源、系统 DPI 感知、Common Controls 6.0、长路径支持。 |
| `data/` | 运行数据 | 题库、两个标签容器、API 与主题配置；可直接备份或迁移。 |
| `docs/` | 交付文档 | 亮/暗主题截图、示例试卷和页面预览。 |

### 一次操作在代码中的流转

1. **启动**
   `main.cpp` 初始化 COM/GDI+ → `App::Create` → `Store::Initialize` 读取 `data/`，数据为空则调用 `seed.cpp` 生成示例题库 → 初始化字体、控件和主题 → 进入 `Run()` 的消息循环。

2. **界面绘制**
   `App::OnPaint` 创建不透明 32 位缓冲区 → `ui::Canvas` 使用 ClearType 绘制当前页面 → 将缓冲区按 1:1 像素拷贝到窗口。鼠标移动时重建 `hits_` 的 hover 状态，点击时由 `Activate()` 分发动作。

3. **筛选与组卷**
   用户操作修改 `QuestionFilter` 和 `selectedIds_` → `App::RebuildFiltered()` 重新计算 `visibleIds_` → 题库列表重绘；底部组卷篮实时统计题型和预计分数。

4. **AI 文本标注**
   选中题目 → `StartAiTagging()` 启动后台线程 → 逐题调用 `AnalyzeQuestionWithAi()` → 通过 `WM_APP` 消息回到主线程 → `Store::MergeQuestionTags` 合并标签 → 保存题库和标签库。

5. **图片识题**
   文件对话框（支持多选）→ 每张图片经 `EncodeImageForAi()` 压缩 → `AnalyzeQuestionImageWithAi()` 要求模型返回 `questions[]`，每项包含题干、选项、难度、标签，以及模型独立求解的 `answer`/`analysis` → 主线程弹出确认框 → 确认后批量插入题库顶部、合并标签并保存。

6. **题目编辑**
   `App::OpenQuestionEditor()` → `ShowQuestionEditor()` 创建独立模态窗口和原生 `EDIT` 控件 → 保存时把 `QuestionEditorResult` 返回主窗口 → `App::SaveQuestionFromEditor()` 规范化文本、更新或新增题目、保存数据。

7. **导出试卷**
   `selectedIds_` 生成有序题目列表 → 组装 `PaperOptions` → `GenerateExamDocx()` 写 OOXML 并打包为 `.docx` → 输出到桌面或用户选择的位置。

8. **主题切换**
   点击侧边栏“夜间模式” → `App::SetDarkMode()` 更新 `Settings.darkMode` → 应用 `col` / `edcol` 两套调色板 → 同步原生输入框画刷和 DWM 标题栏 → `Store::SaveSettings()` 持久化 → 下次启动恢复。

### 维护时从哪里改

- **改界面布局/新增按钮**：主要在 `app.cpp` 的 `Paint*`、`Compute*Rects`、`Activate` 和 `hits_` 逻辑中；新增文本框时同步修改 `LayoutEditControls`。
- **改 AI 提示词/返回结构**：改 `ai.cpp` 的 `BuildAiPrompt` / `BuildImagePrompt` 和对应解析函数；如果增加题目字段，还要同步 `model.*`、`store.*`、`editor.*`、`docx.*`。
- **改主题颜色**：主界面在 `app.cpp` 的 `namespace col`，编辑器在 `editor.cpp` 的 `namespace edcol`；两处都通过 `Apply(bool dark)` 切换。
- **改试卷版式**：集中在 `docx.cpp`；运行 `build\selftest.exe` 会生成 `docs/示例试卷.docx`，再用 Word/WPS 打开检查分页。
- **改数据结构/持久化**：从 `model.h` 开始，随后检查 `store.cpp`、`json.cpp` 的序列化兼容性。

## 从源码构建

需要：

- Windows 10/11 x64；
- CMake 3.20+；
- MinGW-w64 g++（验证版本为 GCC 16.2.0，POSIX 线程模型）。

在项目根目录运行：

```powershell
.\build.ps1
```

或手动：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
```

程序以静态方式链接 MinGW 运行时，只依赖 Windows 系统 DLL（GDI+、WinHTTP、DPAPI、Common Dialogs）。

## 自测

源码构建完成后运行：

```powershell
.\build\selftest.exe
```

自测覆盖：

- JSON 解析与转义；
- 示例题库、难度、题型和标签；
- 标签/难度筛选；
- DOCX 试卷生成；
- 题库保存与重新加载；
- 夜间模式偏好保存与重新加载；
- DPAPI 密钥加密（明文不落盘、可正确解密）；

图片上传、多图多题识别、AI 自动生成答案/解析、批量导入、LaTeX 公式转可读文本，以及视觉消息请求也已用本地模拟视觉接口做过完整验证。

## 说明与边界

- “任意模型”在本程序中指 **OpenAI Chat Completions 兼容接口**。不兼容该协议的接口需要额外的网关适配。
- 图片识题使用 OpenAI 的 `image_url` 视觉消息格式；模型本身必须支持图片输入。
- 题目文本使用 Unicode 数学符号；常见 LaTeX 会被自动转换成可读文本，但导出的是普通文本，不会生成 Word 公式对象。特别复杂的分式、矩阵或多行 cases 可能显示为普通单行文本，可在编辑器中手工调整。
- AI 生成的答案和解析可能出错，导入后请在题目编辑器中核对；程序不会自动验证数学正确性。
- AI 输出标签质量取决于所选模型；程序会校验 JSON 结构并做去重，但不会自动判断 OCR 转录是否完全正确。
- 一次图片识题最多选择 16 张图片；如果题目和解析太多导致模型输出达到长度上限，程序会提示分批上传。
- 图片中的图形题识别效果取决于视觉模型；建议上传清晰、正对镜头、单页题目的照片或截图。
- 导出的试卷用 Word/WPS 打开后即可打印；程序本身不直接调用打印机。
