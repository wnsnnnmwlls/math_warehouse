# MathPaperStudio 移动端迁移技术评估

> 评估对象：`C:\Users\lio\Desktop\MathPaperStudio`（Windows 桌面版 v1.0）  
> 评估日期：2026-10-19  
> 代码基线：`0015296 init project` 及当前工作区源码；工作区已存在未提交的 `grade`/`favorite` 字段改动，本评估只读，不修改任何代码。  
> 目标平台：Android / iOS；后续可选 iPadOS、Android 平板。  
> 结论性质：工程建议，不构成法律意见；合规部分需由目标市场的法务/隐私负责人复核。  
> 基线说明：评估期间工作区存在并发开发，新增了 AI 考试、函数工具、数学键盘、`math_expr`、`dedup`、`text_file` 等模块；其中部分页面/构建集成仍在施工（截至本快照，`PaintExam`/`PaintTools` 只有声明和调用，未在 `app.cpp` 中找到定义，且 `math_expr.cpp`/`text_file.cpp` 尚未加入 `CMakeLists.txt`）。本文把“已稳定逻辑”和“在研功能”分开看；移动端工作量按完整目标范围估算，不按半成品代码量估算。

## 0. 结论摘要

**推荐路线：Flutter + Dart 领域层/数据层 + 本地 SQLite（Drift）+ 系统 Keychain/Keystore；v1 不做 C++ FFI，不做自建服务端依赖，不做纯 PWA。**

核心判断如下：

1. **桌面 UI 没有可复用价值。** 现有界面是 Win32 立即模式自绘 + 少量原生 `EDIT` 控件 + 自绘数学键盘，和移动端声明式 UI、触摸手势、安全区、软键盘模型完全不同。移动端应重新设计信息架构，而不是把 1160px 宽的桌面多栏界面缩小。
2. **可复用的是“规则”和“格式”，不是“代码形态”。** 题目模型、筛选逻辑、AI 提示词与 JSON 解析规则、LaTeX→Unicode 清洗、DOCX 的 OOXML 版式、种子题库、数学表达式解析、去重规则、文本导入规则都可以移植；其中大部分应该用 Dart 重写，而不是通过 FFI 调用 C++。
3. **现有“核心层”仍不是干净的可移植核心。** `CMakeLists.txt` 的 `CORE_SOURCES` 仍包含 `store.cpp`（DPAPI）、`ai.cpp`（WinHTTP）、`image_data.cpp`（GDI+）；自测程序也链接 `winhttp/crypt32/gdiplus`。新增的 `math_expr`/`dedup`/`text_file` 反而是更干净的可移植候选，但截至快照还没有完整接入构建。要在移动端复用得先做真正的平台抽象，成本不低于用 Dart 重写。
4. **移动端 v1 的真实难点不是 CRUD，而是四件事：** 数学输入/渲染、相机→压缩→视觉模型→结构化解析的可靠流水线、DOCX/PDF 版式保真、离线数据与密钥安全。把这四件事做好，产品就成立；做不好，功能列表再长也没用。
5. **六屏全量首发偏重。** 若移动端目标“题库、标签、AI 识题、AI 考试、函数工具、几何画板”全部进入 v1，按 2.5 人有效投入估算约 **40–56 人周，日历 5–7 个月**（含 20% 缓冲）。建议商业首发先做“题库 + 标签 + AI 识题/标注 + 试卷合成导出”四屏，约 **30–38 人周，3.5–5 个月**；AI 考试、函数工具、几何画板放 v1.1/v1.2。若六屏是硬性商业承诺，应按 6–7 个月立项，不要按 3 个月排期。
6. **不建议纯 PWA/WebView 作为正式 v1。** 它的开发速度确实快，但在 iOS 的存储清理、文件系统、后台任务、应用商店审核、密钥安全和任意 AI 接口的 CORS 上都有硬限制。PWA 适合做验证原型或内部工具；要上架，最终仍会被迫补 Capacitor/原生插件，反而形成两套技术债。
7. **不建议全原生双端重写。** 只有在已有独立 Android/iOS 团队、且产品明确需要深度系统集成时才值得。当前产品是“内容工具”，不是相机/地图/即时通讯类应用，双端 UI 重复开发的收益很低。
8. **v1 的 AI 采用 BYOK（用户自带 Key）直连，开发者不承担推理成本，也不接触学生数据。** 代价是密钥在用户设备上、跨设备不同步、对模型服务商网络有依赖；这是当前阶段最合适的取舍。若未来做订阅/配额，再引入服务端代理，但不要把“代理”误当成“同步”或“账号体系”。

### 0.1 一页决策表

| 决策点 | 建议 | 主要原因 |
| --- | --- | --- |
| UI 框架 | Flutter | 单代码库、自绘 Canvas 适合函数图像/几何画板、A4 预览一致、Dart 侧可直接做 DOCX/PDF |
| 领域与数据层 | Dart 重写 | C++ 核心被 Win32 依赖切割；Dart 与 UI 同构，测试/迭代更快 |
| C++ 核心 | v1 不接 FFI | 可移植部分约 3–5k 行（含新增 math_expr/dedup/text_file）；FFI 构建、内存、回调、CI 成本仍高于收益 |
| 服务端 | v1 不建 | 本地优先；BYOK；无账号/无同步；降低合规与运维成本 |
| 本地存储 | Drift + SQLite | 结构化查询、迁移、事务、标签多对多、后续同步元数据 |
| API Key | iOS Keychain / Android Keystore | 不进入 SQLite/日志/备份；设备绑定 |
| 数学内容 | v1 继续 Unicode 纯文本 | 与桌面/AI/DOCX 完全兼容；LaTeX 双写放 phase 2 |
| 试卷导出 | 移植 DOCX + 新增 PDF | DOCX 是教师工作流刚需；PDF 保证手机预览/打印一致 |
| 相机/OCR | 系统相机 + 压缩 + 视觉模型 | 端上 OCR 对数学公式/图形不可靠，视觉模型是主路径 |
| 首发范围 | 四屏核心 + 可选基础函数图像 | 控制风险；AI 组卷/几何画板放后续 |
| 发布形态 | 原生 Flutter App | 应用商店、权限、后台、文件分享都更可控 |

---

## 1. 桌面端现状与可移植性

### 1.1 运行形态

- 单进程、单窗口，当前源码为六个页面：题库、标签、AI 接口、AI 考试、函数工具、试卷合成（`src/app.h:31` 的 `Page` 枚举；`PaintExam`/`PaintTools` 截至快照仍在施工）。
- 入口 `wWinMain` 初始化 COM、GDI+、DPI awareness，创建 `App` 并进入 Win32 消息循环（`src/main.cpp`）。
- UI 是**立即模式绘制**：每次 `WM_PAINT` 创建 32 位 DIB 缓冲区，`ui::Canvas` 用 GDI+ 画当前页，再把可点击区域记录到 `hits_`；只有输入文字时才创建原生 `EDIT` 控件。`src/app.cpp:1401` 的 `OnPaint` 是核心绘制入口。
- 题目编辑弹窗为独立 Win32 窗口 + 原生 `EDIT`；新增“数学键盘”自绘弹窗（`src/keyboard.cpp`，123/ABC/函数三个 tab），通过 `WS_EX_NOACTIVATE` 把按键插入目标输入框。
- 业务状态集中在 `App`：当前页、筛选条件、可见题目 ID、已选题目 ID、滚动状态、AI 线程状态、导出状态都在 `App` 里；`Store` 只负责加载/保存。这是典型的“UI 控制器 + 本地文件”架构，不是分层领域架构。
- 数据放在 exe 同目录的 `data/`：`questions.json`、`tags.json`、`settings.json`。写入采用临时文件 + `MoveFileExW` 原子替换（`src/util.cpp:344`）。
- 导出是直接拼 OOXML 再手写 Store-only ZIP（`src/docx.cpp:613` 起），不依赖 Word 或第三方库；目前仍没有 PDF 导出。
- AI 通过 WinHTTP 调用 OpenAI Chat Completions 兼容接口（`src/ai.cpp:101`），API Key 用 Windows DPAPI 加密后 Base64 写入 `settings.json`（`src/store.cpp:42`）。
- 图片识题用 GDI+ 解码、EXIF 方向纠正、最长边 1600px、JPEG 质量 84、Base64 data URL（`src/image_data.cpp:91` 起）；一次最多 16 张，逐张顺序请求（`src/app.cpp:3299` 起）。新增 `text_file.cpp` 支持从 `.txt`/`.md`/`.docx` 提取文本再交给 AI，但不支持 `.doc`/`.pdf`。
- 新增 `math_expr.cpp`：单变量表达式解析/求值（隐式乘法、三角/对数/根号、定义域返回 NaN）和零点扫描；`dedup.cpp`：题干规范化去重/隐式乘号插入。这两块是较干净的可移植逻辑。
- 新增“AI 考试”的设计意图：描述薄弱点 → AI 诊断 → 从题库生成针对性练习卷 → 计时作答；它和“试卷合成”不是同一个功能。新增“函数工具”的设计意图：表达式绘图、零点、AI 分析定义域/值域/单调性。两页的 UI 实现仍在施工。

### 1.2 模块规模与可移植性

以下行数为本次快照的 `src/*.cpp` 粗略统计，合计约 **1.18 万行**（不含头文件），且仍在增长；不要把这些数字当成最终规模：

| 文件 | 约行数 | 可移植性 | 移动端处理建议 |
| --- | ---: | --- | --- |
| `app.cpp` | 3,400+ | 不可移植 | 重新实现页面、导航、手势、状态管理 |
| `text_file.cpp` | 1,170 | 大部分可移植 | `.txt`/`.md`/`.docx` 提取；移动端可用 Dart 库重写 |
| `math_expr.cpp` | 950 | 纯 C++ 可移植 | 移植为 Dart；表达式语法/求值/零点测试向量要保留 |
| `editor.cpp` | 950 | 不可移植 | 改为全屏编辑页/表单，不使用原生 EDIT 弹窗 |
| `ai.cpp` | 810 | 部分可移植 | 提示词/解析规则/错误文案可复用；HTTP、线程、超时重写 |
| `docx.cpp` | 670 | 大部分可移植 | OOXML 版式与 ZIP 逻辑移植到 Dart |
| `keyboard.cpp` | 530 | 不可移植（UI） | 键位/插入语义/分组可复用；Win32 弹窗重写 |
| `json.cpp` | 515 | 可移植 | 直接用 Dart `dart:convert` + `json_serializable`/`freezed` |
| `math_text.cpp` | 470 | 纯 C++ 可移植 | 移植为 Dart，保留测试向量 |
| `util.cpp` | 470 | 部分可移植 | UTF-8 处理可复用；文件、路径、时间、原子写入全部重写 |
| `model.cpp` | 400 | 大部分可移植 | 题目、标签、筛选、难度/题型/年级/收藏逻辑移植 |
| `store.cpp` | 360 | 不可移植 | DPAPI、exe 相对目录、整文件 JSON 都替换为 Drift |
| `ui.cpp` | 350 | 不可移植 | GDI+ 绘制/字体/裁剪全部改为 Flutter Widget/Canvas |
| `seed.cpp` | 210 | 数据可移植 | 导出为 JSON asset，不保留 C++ 结构 |
| `dedup.cpp` | 180 | 纯 C++ 可移植 | 移植规范化/相似度规则 |
| `image_data.cpp` | 160 | 不可移植 | GDI+ 编解码改为 `image`/`flutter_image_compress`/平台编解码 |
| `selftest.cpp` | 220 | 不可移植 | 用 Dart 单元/集成测试重写 |
| `main.cpp` | 45 | 不可移植 | Flutter `main()` + 平台初始化 |

### 1.3 Windows 依赖清单与移动替代

| Windows 依赖 | 具体用途 | 移动端替代 | 迁移影响 |
| --- | --- | --- | --- |
| Win32 `user32`/`gdi32`/`comctl32` | 窗口、消息循环、自绘、原生 EDIT、滚动条 | Flutter Widget 树、手势、`TextField`、`ScrollController` | 界面层 100% 重写 |
| `dwmapi`/`uxtheme` | 深色标题栏、原生控件主题 | Flutter 主题 + 系统状态栏/导航栏适配 | 不再需要 |
| GDI+ | ClearType 文字、圆角、裁剪、图片解码/缩放/JPEG 编码、EXIF | Impeller/Skia、`dart:ui` Canvas、`image` 包、平台 ImageIO | 绘制代码全部重写；图片管线要重新做 HEIC/EXIF/方向 |
| WinHTTP | AI HTTP POST、超时、代理 | `dio`/`package:http` + `dart:io HttpClient` | 重写；原生端没有浏览器 CORS，但要注意 TLS、代理、证书 |
| DPAPI / `crypt32` | API Key 加密 | iOS Keychain、Android Keystore（`flutter_secure_storage`） | 密文不可跨平台迁移，用户必须重新输入 Key |
| `comdlg32`/`shell32` | 打开图片、选择导出目录、打开数据目录、桌面/图片目录 | 系统照片选择器、文件选择器、系统分享面板、`path_provider` | 不能假设“桌面路径”；改用 app sandbox + SAF/Documents |
| OLE `IStream`/`CoInitialize` | GDI+ 编码到内存流 | 内存 `Uint8List`/临时文件 | 重写 |
| Win32 文件 API、`std::filesystem` | 读写 JSON、原子替换、路径 | SQLite + `path_provider` + 事务；临时文件 + rename | 不再用“exe 同目录”模型 |
| `SYSTEMTIME`/`GetLocalTime` | ZIP DOS 时间戳、备份文件名 | `DateTime.now()` + 时区处理 | 简单替换 |
| MinGW 静态运行时、单 exe | 分发 | Flutter AOT + 平台包 | 构建/签名/商店流程全新 |

### 1.4 哪些逻辑真正值得复用

按“复用价值/改造成本”排序：

1. **AI 提示词与输出契约（高价值）**  
   `BuildAiPrompt`、`BuildImagePrompt`、标签 JSON 结构、`questions[]` 多题解析、`finish_reason=length` 处理、错误文案，都是产品资产。应原样迁移到 Dart 的 `prompts/` 与 `ai_parser`，并建立固定测试样例。
2. **DOCX 版式（高价值）**  
   A4 尺寸 11906×16838 twips、页边距 1247/1134/1134/1134、宋体/黑体、选择题两栏制表位、按题型分节、答案页另起、页脚 PAGE/NUMPAGES，这些是经过验证的版式知识。移植到 Dart 比重新设计更稳。
3. **数学文本清洗（中高价值）**  
   `math_text.cpp` 把常见 LaTeX 转为可读 Unicode，覆盖 `\frac`、`\sqrt`、希腊字母、上下标、常用符号；它明确不是完整 LaTeX 引擎，复杂矩阵/cases 会退化为单行。移动端应移植它作为“AI 输出归一化”和“旧数据兼容”的兜底。
4. **数学表达式解析（高价值，新增）**  
   `math_expr.cpp` 是纯 C++ 的单变量表达式 lexer/parser/求值器，支持隐式乘法、三角/反三角/对数/根号/绝对值/取整、`pi`/`e`、定义域返回 NaN，并有零点扫描。它没有 Win32 依赖，是函数图像功能的理想基础；移动端建议用 Dart 重写同一套语法和测试向量，而不是 FFI。
5. **去重与文本导入规则（中价值，新增）**  
   `dedup.cpp` 的规范化题干/隐式乘号处理、`text_file.cpp` 的 `.txt`/`.md`/`.docx` 提取规则，都可以移植为 Dart。注意 `text_file.cpp` 自己解 ZIP/OOXML，移动端可直接用 `archive` + `xml` 重写，测试样例比代码更重要。
6. **题目模型/筛选/标签统计（中价值）**  
   `Question`、`TagLibrary`、`QuestionFilter` 的字段和匹配规则可以直接映射到 Dart/SQLite。注意当前工作区新增了 `grade`/`favorite` 字段，移动端导入时要按可选字段处理。
7. **种子题库（低改造成本）**  
   24 道示例题、27 个公式标签、52 个知识点标签，导出成 JSON asset 即可；不建议在 Dart 里重写 C++ 初始化代码。
8. **数学键盘的键位/交互语义（中价值，新增）**  
   `keyboard.cpp` 是 Win32 自绘弹窗，代码不可复用；但“123/ABC/函数”分组、在光标处插入、退格/左右/回车/Tab、深色主题等交互可以复用。移动端应重做键盘，而不是把 832×272 的桌面弹窗直接搬过去。
9. **JSON 解析器（低价值）**  
   桌面的 `json.cpp` 是自己实现的 AST，移动端直接用成熟 Dart 库，没必要移植。
10. **UI/编辑器/主题（几乎无复用）**  
   可以复用配色、圆角、间距、信息层级等设计语言，但代码不可复用。桌面端固定 1160px 最小宽度（`src/app.cpp:602`），移动端必须重新做断点、单列/双栏、底部导航、软键盘避让。

### 1.5 现有架构对移动端不友好的地方

- **整文件 JSON 持久化**：题目、标签、设置各一个文件，任何保存都重写整个文件。24 道题没问题，几千道题会明显卡顿，也无法做增量同步、事务和并发。
- **标签冗余存储**：题目内嵌 `formulas`/`concepts`，另有 `tags.json`；标签重命名/合并/删除没有外键约束，容易出现“题目里有、标签库里没有”的漂移。移动端应改为 `tags` + `question_tags` 多对多。
- **没有同步元数据**：没有 `updated_at`/`rev`/`deleted_at`/`device_id`。如果以后要做多设备同步，后补这些字段会很痛苦；即使 v1 不同步，也应该在 schema 里预留。
- **AI 任务不可恢复**：桌面用 `std::thread` + `WM_APP`，关闭窗口/崩溃后任务丢失；没有取消、重试、断点续跑、单图失败隔离。移动端必须把这部分做成可持久化的任务队列。
- **UI 与业务强耦合**：`App` 同时知道筛选、选中、AI 线程、导出路径、Toast；移动端要拆成 `presentation/application/domain/data`，否则测试和后续功能会越来越难。
- **数据目录不可移植**：移动端没有“exe 同目录”，也不能保证可写；必须使用 iOS Application Support / Android filesDir，并把用户可见导出放到 Documents/SAF。
- **新功能仍在施工，不能按“已完成”估移动端**：AI 考试、函数工具的页面已进入导航/源码，但截至快照 `PaintExam`/`PaintTools` 还没有实现，`math_expr.cpp`/`text_file.cpp` 也未加入 `CMakeLists.txt`；这说明桌面端仍在快速迭代。移动端不能假设这些交互已经稳定，必须重新做产品设计。
- **AI 考试不是“AI 从零出卷”**：从现有文案看，它是“描述薄弱点 → AI 诊断 → 从题库生成针对性练习卷 → 计时作答”；如果移动端要“AI 生成新题”，仍是新功能，需要新的 prompt、校验、审核和成本模型。
- **没有 PDF、几何画板、云同步**：函数工具已有表达式解析/零点/AI 分析的设计，但没有完整几何对象模型；试卷仍只导出 DOCX；数据仍只在本机 JSON。移动端这三块都要新建。
- **桌面预览不是分页引擎**：当前预览是自绘的缩略页，真正分页由 Word 完成。移动端如果承诺“所见即所得”，必须用 PDF 渲染做最终预览，不能只靠 Flutter Widget 近似。

---

## 2. 三条现实路线

### 2.1 路线 A：全原生重写（Kotlin/Compose + Swift/SwiftUI）

**做法**：Android 用 Kotlin + Jetpack Compose，iOS 用 Swift + SwiftUI；领域模型、数据、AI、DOCX/PDF 各写一套，或用 Kotlin Multiplatform 共享部分非 UI 逻辑。

**优点**

- 最佳的系统集成：相机、相册、文件、分享、打印、Keychain/Keystore、后台任务、辅助功能都是一等公民。
- 性能和包体可控；复杂手势、Canvas、键盘避让可以做到最细。
- 应用商店信任度最高；审核时不容易被认定为“网页套壳”。
- 可以针对 iPad/Android 平板做真正的多窗口、拖拽、Apple Pencil。

**缺点**

- **两套 UI、两套状态管理、两套测试、两套发布流程**。以当前产品的页面密度，双端 UI 工作量接近 2 倍。
- 数学公式渲染、DOCX 生成、PDF 排版在两端很难保持一致；教师在不同设备看到的试卷可能不同。
- iOS 需要 Mac + Xcode + 开发者账号；Android 国内渠道还要单独适配。
- 如果不用 KMP，领域逻辑也重复；用 KMP 则引入 Gradle/ KMP 工具链和 iOS 互操作复杂度，团队门槛更高。
- 维护成本约为 Flutter 的 1.7–2.2 倍；小团队很难同时保持两端高质量。

**工作量估算**

| 部分 | 人周 |
| --- | ---: |
| Android UI/交互/适配 | 18–24 |
| iOS UI/交互/适配 | 20–27 |
| 共享领域/数据/AI/导出（若 KMP） | 8–12 |
| 相机/OCR/文件/分享/打印 | 4–6 |
| 测试、性能、无障碍、商店 | 6–8 |
| **合计** | **48–65** |

按 2 名原生开发 + 1 名共享逻辑/后端，约 **6–9 个月**；若只有 2 人串行做双端，约 **9–12 个月**。

**适用条件**：已有独立双端团队；产品需要大量系统级能力（如手写笔、实时相机、离线 OCR、后台采集）；公司明确不做跨平台。**当前项目不满足这些条件。**

### 2.2 路线 B：Flutter 或 React Native + 原生/FFI 核心

#### 2.2.1 为什么优先 Flutter

- **单代码库、单渲染管线**：题库列表、试卷预览、函数图像、几何画板都可以在同一套 Widget/Canvas 体系里做，不需要 RN 的 JS→原生桥接来画高频图形。
- **Canvas 能力强**：`CustomPainter` 直接画函数曲线、坐标轴、几何对象、A4 预览；`RepaintBoundary.toImage()` 可直接导出 PNG 插入题目。
- **Dart 侧能完成大部分“桌面核心”**：HTTP、JSON、SQLite、ZIP/OOXML、PDF、图片处理都有成熟包，不需要 C++。
- **后续可扩展到 iPad/Android 平板，甚至 Windows/macOS**：如果未来想用同一套 UI 替代桌面端，Flutter 是更现实的路径。
- **测试友好**：领域逻辑、AI 解析、DOCX 生成都能用纯 Dart 单测覆盖，不依赖模拟器。

React Native 也完全可行，尤其团队是 TypeScript 背景时：

- 优点：JS 生态、Expo 开发体验、热更新/OTA（注意商店规则）、Web 技能复用。
- 缺点：数学渲染通常要 WebView + KaTeX/MathJax，或原生 MathView；高频 Canvas 需要 `react-native-skia`；DOCX/PDF/文件/打印要拼多个库；原生模块升级的兼容性风险不低于 Flutter。
- 如果团队 TypeScript 强、Flutter 弱，RN 的“总成本”可能更低；否则当前产品更适合 Flutter。

**建议**：以团队现有能力为第一优先级；在能力相当的情况下选 Flutter。不要为了“跨平台”选一个团队不熟的框架。

#### 2.2.2 C++ 核心通过 FFI 复用：可行，但不建议 v1 做

**技术上可行**：

- 把可移植代码抽成静态库，暴露 C ABI（不要暴露 STL 类型），用 `dart:ffi` + `ffigen` 绑定。
- Android 用 NDK 编译 `arm64-v8a`/`x86_64`（模拟器）；iOS 编译真机 `arm64` 和模拟器 slice，做成 XCFramework。
- 入参/出参用 UTF-8 `char*` + 长度，调用方负责释放；错误用返回码 + `mps_last_error()`。
- 耗时调用放到 `Isolate.run`，避免阻塞 UI isolate；不要让 C++ 线程回调 Dart，除非确实需要。

**但当前代码不适合整体直接抽**：

- `CMakeLists.txt:13` 的 `CORE_SOURCES` 包含 `store.cpp`（`windows.h` + `dpapi.h`）、`ai.cpp`（`windows.h` + `winhttp.h`）、`image_data.cpp`（GDI+）。这不是“可移植核心”，而是“除 UI 外所有东西”。
- `docx.cpp` 虽然 OOXML 逻辑可移植，但通过 `util::WriteFileAtomic` 写文件，并调用 `GetLocalTime()` 生成 ZIP 时间戳；`util.cpp` 又大量使用 Win32 文件 API、`SHGetFolderPathW`、`GetModuleFileNameW`。
- `model.h` 的部分函数返回 `const wchar_t*`（面向 Win32 UI 的显示名），不适合作为跨语言领域 API。
- `selftest` 也链接 `winhttp/crypt32/gdiplus`，说明当前“核心”没有平台边界。
- 新增的 `math_expr.cpp`、`dedup.cpp`、`text_file.cpp` 是更干净的候选，但截至快照 `math_expr.cpp`/`text_file.cpp` 还没有加入 `CMakeLists.txt`，说明集成尚未完成；不要假设它们已经是稳定 API。

**如果要抽，至少需要：**

1. 新建 `core_portable` 目标，只含 `json/model/seed/math_text/docx`；
2. 用 `std::chrono`/`DateTime` 替代 `GetLocalTime`，用抽象 `FileSink` 替代 Win32 文件写入；
3. 把提示词从 `ai.cpp` 拆到资源文件，HTTP 完全留在 Dart；
4. 定义稳定 C ABI，补 UTF-8/内存所有权/错误码测试；
5. 建 Android/iOS 交叉编译与 CI。

这大约 **3–6 人周**（不含业务功能），之后每个版本都要维护多 ABI 构建。对当前约 1.5–2.5k 行可移植逻辑来说，**收益不足以抵消复杂度**。

**只有在以下情况才值得重新考虑 FFI**：

- 桌面端和移动端要长期共享同一个“排版/计算引擎”，且该引擎会增长到 1–2 万行以上；
- 有复杂 CAS、符号计算、几何约束求解等 Dart 不擅长的重计算；
- 团队有稳定 C++/NDK/iOS 构建能力，并愿意把它作为长期基础设施。

#### 2.2.3 推荐库清单（Flutter）

| 能力 | 推荐 | 说明/风险 |
| --- | --- | --- |
| 状态/路由 | `riverpod` 或 `bloc` + `go_router` | 选团队熟悉的；不要把所有状态塞进一个全局对象 |
| 本地数据库 | `drift` + `sqlite3_flutter_libs` | 关系型、迁移、事务、响应式查询；适合题目/标签/试卷 |
| 安全存储 | `flutter_secure_storage` | iOS Keychain、Android Keystore；注意 Android 备份与密钥丢失 |
| 数学渲染 | `flutter_math_fork`（KaTeX 的 Dart 移植） | 纯 Dart、可选中；但上游更新慢（截至 2026-10 约 0.7.4，17 个月未发版），必须 pin 版本并做 iOS/Android 真机回归 |
| 复杂公式兜底 | WebView + KaTeX/MathJax | 仅用于单题详情/预览，不用于列表；WebView 有启动与内存成本 |
| 图表/函数 | `CustomPainter` + `vector_math`；统计图可用 `fl_chart` | 函数图像不要硬套统计图库；自绘更可控 |
| PDF | `pdf` + `printing` | A4、页眉页脚、打印/分享；需内嵌中文字体 |
| DOCX | `archive` + 手写 OOXML（移植桌面逻辑） | 不依赖 Word；可做 golden test |
| 文件/分享 | `file_picker`、`share_plus`、`path_provider` | iOS 用系统分享/文档选择器；Android 用 SAF |
| 相机/相册 | `camera`、`image_picker` | iOS HEIC 要转 JPEG；注意权限和照片选择器 |
| 图片处理 | `image`、`flutter_image_compress` | EXIF 方向、缩放、JPEG 质量；大图在 isolate 处理 |
| OCR（可选） | `google_mlkit_text_recognition` | 仅做文字预检/辅助，不能替代视觉模型解数学题 |
| 网络 | `dio` | 取消、超时、SSE、拦截器；不要用 WebView 发 AI 请求 |
| 崩溃/日志 | `sentry_flutter`（可选） | 必须 scrub prompt、图片、Key、学生信息 |
| 后台任务 | 不作为核心依赖 | iOS 后台执行不可靠；AI 任务应前台可恢复 |

**注意**：

- `flutter_math_fork` 维护活跃度是实际风险。建议把它包在 `MathView` 抽象后面，保留“纯文本 fallback”；如果 iOS 渲染或复杂公式问题严重，再切换到 WebView/KaTeX 或原生 MathView，而不是让全项目依赖它。
- 不要用 `syncfusion_flutter_charts` 这类商业许可库做函数图像，除非确认许可证；自绘更简单。
- 不要把 API Key 放进 `SharedPreferences`、`flutter_dotenv`、`--dart-define` 或 Git。

### 2.3 路线 C：本地 Web/PWA/WebView 套壳

**做法**：React/Vue/Svelte + TypeScript，KaTeX/MathJax 渲染公式，Canvas/SVG 画函数和几何，IndexedDB 存数据，Service Worker 离线；用 PWA 直接发布，或用 Capacitor/Cordova 套壳上架。

**为什么看起来最快**

- Web 技术对表单、列表、响应式布局、公式渲染（KaTeX）非常成熟。
- 同一套 UI 可以同时跑桌面浏览器、Android、iOS，开发迭代快。
- 不需要学 Dart/Swift/Kotlin；前端招聘容易。
- 如果未来做 Web 版，代码可复用。

**硬限制（按严重程度排序）**

| 限制 | 具体表现 | 对本品的影响 |
| --- | --- | --- |
| **任意 AI 接口的 CORS** | 浏览器直接 `fetch` 多数 OpenAI 兼容服务会被 CORS 拦截；不是所有服务商允许浏览器直连 | 必须自建代理，代理又要保管 Key、做配额和隐私合规；“无后端”优势消失 |
| **密钥安全** | 浏览器只能把 Key 放 IndexedDB/localStorage/WebCrypto；XSS、恶意扩展、共享电脑都可能泄露 | 教师设备上的 Key 风险高于 Keychain/Keystore；不适合作为正式方案 |
| **离线存储** | IndexedDB 可用，但 iOS 对可写存储有清理策略；用户清理浏览器数据会丢题库 | 需要频繁导出备份；不能承诺“永久离线保存” |
| **文件系统** | iOS Safari 不支持完整的 File System Access；下载/保存/分享受限；Android Chrome 稍好 | DOCX/PDF 导出后“保存到文件/微信/打印”体验割裂 |
| **相机** | `getUserMedia` 可用，但权限、对焦、闪光、方向、HEIC 处理不如原生；iOS 后台切换容易中断 | 拍照识题是核心入口，体验不能将就 |
| **后台计时/任务** | 浏览器会节流 `setTimeout`，切后台可能冻结；长 AI 请求可能被中断 | 批量识题/长文生成不可靠；不能做“后台上传” |
| **应用商店** | 纯 WebView 套壳容易被 Apple 4.2（minimum functionality）拒绝；Google Play 对纯网页体验也有限制 | 上架需要原生能力/差异化；否则只能做 PWA/企业分发 |
| **性能/内存** | 大图 Base64、长列表、Canvas 高频重绘在移动浏览器上更吃内存 | 16 张原图/长试卷容易 OOM；需要额外压缩和分页 |
| **系统集成** | 打印、分享、文件选择、Keychain、后台下载、通知都不如原生 | 教师工作流会频繁遇到“多一步”的摩擦 |

**PWA 仍然有价值的场景**

- 内部试点/教研组小范围验证，不追求应用商店；
- 以桌面浏览器为主、移动端只做查看/轻编辑；
- 已经有稳定的 AI 代理和账号体系，Web 只是前端；
- 用 Capacitor 套壳并接受“仍然要写原生插件”的现实。

**如果坚持 WebView 套壳**：至少用 Capacitor，补齐相机、文件、分享、安全存储、PDF 预览等原生插件；不要用纯 `WebView` 包一个 URL。即使这样，数学/图形性能、离线可靠性、商店审核仍不如 Flutter 原生。

**工作量估算**

| 形态 | 人周 | 说明 |
| --- | ---: | --- |
| 纯 PWA（含代理） | 14–20 | 前端 6–8、数据/离线 2–3、AI 代理 2–3、导出 2、测试 2 |
| Capacitor 套壳上架 | 20–28 | 加原生插件、权限、商店、PDF/文件、兼容性 |
| 纯 WebView 包 URL | 6–10 | 不建议；审核和体验风险最高 |

### 2.4 路线对比

| 维度 | 全原生 | Flutter | PWA/WebView |
| --- | --- | --- | --- |
| 首版速度 | 慢 | 中 | 快 |
| 双端一致性 | 差（两套 UI） | 好 | 好（但受浏览器差异影响） |
| 数学/图形性能 | 最好（需分别实现） | 好 | 中 |
| 相机/文件/打印 | 最好 | 好（插件） | 差 |
| 离线可靠性 | 好 | 好 | 中/差（iOS 存储） |
| 密钥安全 | 好 | 好 | 差（浏览器存储） |
| 应用商店风险 | 低 | 低 | 高（套壳） |
| 长期维护 | 最高 | 中 | 中高（浏览器/插件兼容） |
| 适合本产品 | 不推荐 | **推荐** | 仅原型/内部 |

---

## 3. 推荐的移动端 v1 架构

### 3.1 设计原则

1. **本地优先（local-first）**：题库、标签、试卷、设置先写本地 SQLite；没有网络也能完整使用除 AI 外的功能。
2. **领域与平台分离**：`domain` 不依赖 Flutter Widget、不依赖 `dart:io`；AI/文件/相机/Keychain 通过接口注入。
3. **桌面兼容而非像素兼容**：导入/导出保持 JSON 兼容；DOCX 版式保持一致；UI 按手机重新设计。
4. **AI 是可选增强**：没有 Key、没有网络、模型失败时，手动录题、筛选、组卷、导出仍然可用。
5. **所有长任务可取消、可恢复、可计费可见**：图片识题、AI 标注、AI 组卷都进入持久化任务队列。
6. **安全默认值**：Key 只进 Keychain/Keystore；日志不落原文；图片默认不长期保存；HTTP 只允许本机/局域网显式开启。
7. **先做窄而深，不做宽而浅**：先把“拍一张题 → 识别 → 校对 → 入库 → 导出”跑通，再扩展几何/组卷。

### 3.2 分层与目录建议

```text
lib/
  main.dart
  app/                     # 启动、路由、主题、依赖注入
  core/
    db/                    # Drift 数据库、DAO、迁移
    domain/                # Question/Tag/Paper/AiJob 实体与规则
    ai/                    # Provider 适配、prompt、parser、任务队列
    export/                # DOCX、PDF、JSON 导入导出
    security/              # Keychain/Keystore、日志脱敏、权限
    platform/              # 相机、文件、分享、网络、设备信息
  features/
    bank/                  # 题库列表、筛选、编辑
    tags/                  # 标签容器
    ai_import/             # 图片识题、校对、批量导入
    ai_exam/               # AI 组卷
    graph/                 # 函数图像
    geometry/              # 几何画板
    paper/                 # 试卷合成、预览、导出
    settings/              # AI 配置、外观、备份
```

不建议一上来拆成多个 pub 包；当前规模用单 app + feature 目录足够。等函数图像/几何引擎稳定后，再抽 `mps_math`、`mps_geometry` 独立包。

### 3.3 复用决策表

| 桌面资产 | 移动端决策 | 理由 |
| --- | --- | --- |
| `Question`/`TagLibrary`/`QuestionFilter` | **移植为 Dart 实体 + SQLite 表** | 规则简单、可测试；FFI 不划算 |
| `questions.json`/`tags.json` | **保持导入/导出兼容** | 教师已有数据；桌面可继续用 |
| `seed.cpp` 的 24 题/标签 | **导出 JSON asset** | 零逻辑，直接复用数据 |
| `ai.cpp` 提示词/解析/`AskAiChat` | **移植到 Dart** | 产品核心资产；需要更强的取消/重试/流式 |
| `ai.cpp` HTTP/线程 | **重写** | WinHTTP/WM_APP 不可移植；Dart 有更好的取消和流 |
| `math_text.cpp` | **移植为 Dart 并保留测试向量** | 纯字符串处理；用于 AI 输出归一化和旧数据 |
| `math_expr.cpp` | **移植为 Dart** | 纯 C++ 表达式解析/求值/零点；函数图像的基础 |
| `dedup.cpp` | **移植为 Dart** | 题干规范化/隐式乘号；入库去重 |
| `text_file.cpp` | **用 Dart 重写** | `.txt`/`.md`/`.docx` 文本提取；保留扩展名/错误规则测试 |
| `keyboard.cpp` | **重做 UI，复用键位语义** | Win32 弹窗不可移植；123/ABC/函数分组可复用 |
| `docx.cpp` OOXML | **移植** | 版式已验证；Dart ZIP/XML 足够 |
| `docx.cpp` 文件写入/时间 | **重写** | 平台文件、时区、原子写入不同 |
| `store.cpp` DPAPI/JSON | **完全重写为 Drift + Keychain/Keystore** | 安全模型和存储模型都不同 |
| `image_data.cpp` GDI+ | **重写** | 需要 HEIC、方向、压缩、内存控制 |
| `app.cpp`/`editor.cpp`/`ui.cpp` | **重新设计** | 桌面布局/立即模式/原生 EDIT 不可复用 |
| C++ FFI | **v1 不做** | 需先抽出干净核心；收益低、CI 成本高 |
| 自建服务端 | **v1 不做** | BYOK 直连；无账号/同步；降低合规成本 |

### 3.4 数据模型（SQLite）

建议表结构（字段可按实现调整，关键是关系与同步元数据）：

```sql
-- 题目
questions(
  id TEXT PRIMARY KEY,              -- 保留桌面 q_xxx；新题可用 q_<ulid>
  difficulty TEXT NOT NULL,         -- A/B/C
  type TEXT NOT NULL,               -- choice/fill/solution
  grade TEXT,                       -- g1/g2/g3，可空（兼容工作区新增字段）
  favorite INTEGER NOT NULL DEFAULT 0,
  stem TEXT NOT NULL,
  stem_format TEXT NOT NULL DEFAULT 'unicode',  -- unicode | latex（phase 2）
  answer TEXT NOT NULL DEFAULT '',
  analysis TEXT NOT NULL DEFAULT '',
  tagged INTEGER NOT NULL DEFAULT 0,
  source TEXT,                      -- manual/ai_image/ai_exam/import
  review_status TEXT NOT NULL DEFAULT 'approved', -- draft/approved，AI 组卷用
  created_at TEXT NOT NULL,
  updated_at TEXT NOT NULL,
  deleted_at TEXT,
  rev INTEGER NOT NULL DEFAULT 1
);

question_options(question_id TEXT, idx INTEGER, text TEXT,
                 PRIMARY KEY(question_id, idx));

-- 标签（公式/知识点共用，kind 区分）
tags(
  id TEXT PRIMARY KEY,
  kind TEXT NOT NULL,               -- formula | concept
  name TEXT NOT NULL,
  normalized_name TEXT NOT NULL,
  created_at TEXT NOT NULL,
  updated_at TEXT NOT NULL,
  deleted_at TEXT,
  rev INTEGER NOT NULL DEFAULT 1
);

question_tags(
  question_id TEXT NOT NULL,
  tag_id TEXT NOT NULL,
  source TEXT NOT NULL DEFAULT 'manual', -- manual/ai
  PRIMARY KEY(question_id, tag_id)
);

-- 试卷
papers(
  id TEXT PRIMARY KEY, title TEXT NOT NULL, duration_minutes INTEGER NOT NULL,
  notes TEXT, include_answers INTEGER NOT NULL, include_analysis INTEGER NOT NULL,
  include_answer_space INTEGER NOT NULL, created_at TEXT NOT NULL,
  updated_at TEXT NOT NULL, deleted_at TEXT, rev INTEGER NOT NULL DEFAULT 1
);

paper_items(
  paper_id TEXT, question_id TEXT, section TEXT, position INTEGER, points INTEGER,
  PRIMARY KEY(paper_id, position)
);

-- AI 任务（可恢复/可计费）
ai_jobs(
  id TEXT PRIMARY KEY, kind TEXT NOT NULL, status TEXT NOT NULL,
  provider TEXT, model TEXT, request_json TEXT, response_json TEXT,
  input_tokens INTEGER, output_tokens INTEGER, cost_estimate REAL,
  error TEXT, created_at TEXT NOT NULL, updated_at TEXT NOT NULL
);

-- 附件（默认不保存原始照片；用户显式保留时才写入）
attachments(
  id TEXT PRIMARY KEY, question_id TEXT, kind TEXT,               -- image/graph/geometry
  relative_path TEXT NOT NULL, mime TEXT, width INTEGER, height INTEGER,
  sha256 TEXT, created_at TEXT NOT NULL, deleted_at TEXT
);

app_settings(key TEXT PRIMARY KEY, value TEXT NOT NULL);           -- 非敏感配置
```

要点：

- **不要把 API Key 放进 SQLite**；只放 `model`、`api_base`、`temperature` 等非敏感项。
- **标签规范化**：`normalized_name` 用 `trim + lower + 全角转半角 + 连续空白折叠`；唯一索引 `(kind, normalized_name)`。桌面已有 `TagsEqual` 的逻辑可以照搬。
- **搜索**：v1 题目量 < 5k 时可用 `LIKE` + 索引；数据量大时用 SQLite FTS5。中文没有空格，默认 `unicode61` 分词效果有限，应优先验证 FTS5 `trigram`，或对题干做预分词；不要直接假设 FTS5 默认分词可用。
- **列表分页**：`LIMIT/OFFSET` 在几万题时也可接受；配合 `updated_at`/`id` 游标更稳。不要一次性把全部题目读进内存。
- **删除**：软删除 `deleted_at`，为同步预留；UI 可提供“彻底删除”并清理附件。
- **迁移**：Drift `schemaVersion` 从 1 开始，每次迁移写测试；桌面 JSON 导入作为“数据迁移”，不是 schema 迁移。

### 3.5 桌面数据导入/导出

- **导入 `questions.json`**：
  - 兼容根对象 `{"version":1,"questions":[...]}`，也兼容直接数组；
  - 字段缺失用默认值；`difficulty` 无效时用 B；`type` 无效时按有无 options 推断；
  - `grade`/`favorite` 可选；旧桌面没有这些字段时不报错；
  - ID 冲突时生成新 ID，并记录来源；重复题干可提示合并，但不要自动删。
- **导入 `tags.json`**：
  - 兼容 `{formulas:[], concepts:[]}`；与题目内嵌标签合并；
  - 统一去重、计数、重建 `question_tags`。
- **导入 `settings.json`**：
  - 只导入 `api_base`、`model`、`temperature`、`dark_mode`；
  - **跳过 `api_key_protected`（DPAPI 密文）和明文字段 `api_key`**。桌面端仍保留明文兼容分支（`src/store.cpp:286`），移动端绝不能把它带进新库；提示用户重新输入 Key。
- **导出**：
  - 生成与桌面兼容的 `questions.json`/`tags.json`，可被桌面版继续读取；
  - 新增字段桌面会忽略（`FromJson` 只读已知字段），但不要依赖这一点做破坏性变更；
  - 导出前做事务快照，避免导出到一半数据变化；
  - 文件名包含日期，例如 `数研组卷_备份_2026-10-19.json`。

### 3.6 离线优先与同步

**v1：不做云同步。** 只做：

- 本地 SQLite 为唯一真源；
- 手动“导出备份/导入备份”；
- 可选通过系统分享把备份发到微信/网盘/邮件；
- 可选 iCloud Drive/Google Drive 的“文件级备份”，但不要把它宣传成实时同步。

这是最稳的：没有账号、没有服务器、没有学生数据上传，合规成本最低。

**如果 phase 3 要做多设备同步**，现在就要预留：

- 每条记录有 `updated_at`、`rev`、`deleted_at`、`device_id`；
- 删除用 tombstone，不物理删除；
- 客户端有 outbox（待推送变更）和 pull cursor（最后同步序号）；
- 冲突策略：题目级 last-write-wins + 冲突副本（保留双方），不要静默覆盖教师编辑；
- 附件按 `sha256` 内容寻址，按需上传；
- 服务端可用 Supabase（Postgres + Auth + Storage）、Cloudflare D1/R2 或自建 API；选择取决于目标市场的数据驻留要求；
- 不要同步 API Key、AI 原始请求/响应、临时图片；
- 如果要端到端加密，密钥管理和多设备恢复会显著增加复杂度，不建议 v1 做。

**同步顺序建议**：先做“备份文件导入/导出”，再做“只读同步/单向推送”，最后做双向。不要第一版就做 CRDT。

### 3.7 API Key 与安全存储

**v1 推荐 BYOK 直连**：

- 用户在设置页输入 `api_base`、`model`、`api_key`；
- Key 存 iOS Keychain（建议 `kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly`，兼顾后台任务与不随备份迁移）或 Android Keystore（AES-GCM，设备绑定）；
- Android 关闭自动备份或排除 secure storage，避免恢复后密钥解不开；
- 不写日志、不进崩溃报告、不进分析事件、不进导出文件；
- UI 默认掩码，提供“显示/隐藏”；可选生物识别解锁；
- 删除 Key 时同时清理内存中的字符串（Dart 无法保证立即擦除，但至少不持有全局引用）；
- 切换 provider/model 时保留 Key，但明确提示“Key 将发送给该地址”；
- 默认只允许 HTTPS；`http://localhost`/`127.0.0.1`/局域网地址需显式确认，并提示流量可被同一网络观察；
- 自定义 base URL 时显示实际 host，防止用户被钓鱼地址诱导；
- 不实现“把 Key 同步到 iCloud Keychain/Google 备份”，除非产品明确接受跨设备风险。

**如果未来做服务端代理**：

- App 只持有短期用户 token；provider Key 在服务端 KMS/Secret Manager；
- 服务端做鉴权、配额、速率限制、审计、模型路由；
- 永远不要把 provider Key 下发给客户端；
- 代理会接触题目/图片，隐私政策、数据处理协议、数据驻留都要重新评估。

### 3.8 UI 信息架构

手机屏幕不适合桌面三栏。建议：

- **底部导航 4–5 项**：题库、组卷、AI、工具、我的（标签可放“题库”顶部或“工具”里）。
- **平板/折叠屏**：`NavigationRail` + 列表/详情双栏；不要直接放大手机布局。
- **全局悬浮按钮**：题库页的“拍照识题”；组卷页的“生成试卷”；工具页的“新建图形”。
- **编辑页**：全屏，顶部“取消/保存”，内容分组（题干、选项、答案、解析、标签、难度/年级/题型）；离开时拦截未保存修改。
- **触控目标**：≥44×44pt；不要依赖 hover；长按用于更多操作，但要有可见的“…”入口。
- **软键盘**：使用 `SafeArea` + `resizeToAvoidBottomInset`；数学键盘是额外键盘面板，不要和系统键盘打架；编辑长题干时提供“展开全屏”。

### 3.9 六个核心页面

#### 3.9.1 题库（Bank）

**目标**：快速找到题、批量选、进入组卷；这是使用频率最高的页面。

**布局**

- 顶部：标题 + 搜索框 + 筛选按钮（显示激活条件数）+ 多选按钮。
- 筛选：底部弹层（难度、题型、年级、标签、只看待标注/收藏、任一/全部匹配）；不要像桌面那样常驻一个 236px 侧栏。
- 列表：单列卡片；每张卡显示题号、难度、题型、题干 2–3 行、标签前 3 个、AI/手工来源、同步状态（如有）。
- 底部：选中后出现浮动操作条“已选 N 题 · 预计 X 分 · 前往组卷”。
- 入口：FAB“拍照识题”；顶部“新增”。

**关键交互**

- 点击卡片进入详情/编辑；左侧复选框或长按进入多选；避免“点卡片=选中”导致误操作。
- 搜索 200ms 防抖；支持题干、答案、解析、标签；清空按钮。
- 筛选条件用 chip 展示，可单独删除；提供“重置”。
- 列表使用 `ListView.builder`，每页 50 条；滚动保持位置；不要一次性渲染全部。
- 下拉刷新只在有同步时使用；v1 可不用，避免误导。

**技术点**

- 查询用 SQL 聚合标签，避免 N+1；列表只取展示字段，详情再取全文。
- 数学文本按纯文本渲染；长题干 `maxLines` + 展开；不要在每个列表项里放 WebView。
- 图片附件默认不加载缩略图；如需要，使用缓存 + 限制解码尺寸。

#### 3.9.2 标签（Tags）

**目标**：管理公式/知识点两套标签，反向筛题。

**布局**

- 顶部分段控件“公式标签 / 知识点标签”；搜索框；统计卡（总数、已使用、未使用）。
- 列表：标签名、使用题数、最近使用；点击进入筛题；右侧“…”提供重命名、合并、删除。
- 空状态：引导去题库勾选题目做 AI 标注。

**关键交互**

- 点击标签 → 回题库并应用该标签筛选（保持桌面行为）。
- 重命名/合并：先展示影响题数，确认后事务更新；合并要处理重复。
- 删除：仅允许删除“未使用”标签，或有明确二次确认；不要静默从题目中移除。
- 排序：使用次数降序/名称升序；支持搜索。

**技术点**

- `tags` 唯一索引 + `question_tags` 外键；使用计数用 `COUNT(DISTINCT question_id)` 实时查或缓存。
- 标签名最长 16 个汉字（沿用 AI 提示词约束）；超长截断/拒绝。
- 大小写/全半角/空白规范化；显示保留用户原始写法。

#### 3.9.3 AI 识题（AI Import）

**目标**：拍照/选图或导入文本 → 识别多题 → 校对 → 批量入库。这是移动端最重要的差异化入口。

**支持的输入**

- 图片：JPG/PNG/HEIC（HEIC 需转 JPEG）、BMP/GIF（可按平台支持裁剪）。
- 文本：`.txt`/`.md`/`.docx`（桌面 `text_file.cpp` 已支持）；`.doc`/`.pdf` 明确提示“请先另存为 docx/txt”，不要假装能解析。
- 移动端可直接用系统文件选择器 + `archive`/`xml` 读取 docx；纯文本走文本模型，不必用视觉模型，能显著省 token。

**流程**

1. 入口：题库 FAB 或空状态“拍照识题”，二级选择“拍照/相册/导入文档”。
2. 采集：相机拍照 / 相册多选；建议单次 1–8 张（桌面 16 张，手机内存和等待时间不适合）。
3. 预处理：EXIF 方向纠正、最长边 1600px、白底、JPEG 82–85；iOS HEIC 必须转 JPEG；在 isolate 中处理，避免卡 UI。
4. 预览：每张图缩略图、删除、旋转、重拍；显示“将上传到 <host>，可能产生费用”。
5. 队列：逐张请求，显示“第 2/5 张 · 识别中”；支持取消当前/全部；失败图片可单独重试。
6. 解析：严格 JSON；校验 `stem` 非空、`type`/`difficulty` 合法、选项数量、答案/解析缺失标记；保留原始响应到 `ai_jobs`（可配置 7 天保留）。
7. 校对：卡片列表，逐题可编辑；显示“AI 生成，请核对”横幅；缺答案/解析高亮；重复题提示。
8. 入库：事务写入题目、选项、标签；生成新 ID；可选“全部导入/仅导入选中”；导入后跳到题库并定位到新题。

**UI 细节**

- 相机页提供拍摄提示：正对、光线均匀、单页、避免手指遮挡、一次拍一页。
- 识别中允许切后台，但任务可能被系统挂起；回到前台恢复轮询/继续，不要承诺后台完成。
- 校对页支持“上一题/下一题”、批量设置难度/标签、删除某题。
- 导入前显示摘要：“识别 6 题，2 题缺解析，1 题可能重复”。
- 图片默认不保存；如用户选择“保留原图以便复核”，明确提示会占用空间并可能包含学生信息；提供一键清理。

**技术点**

- 请求体与桌面一致：`messages` + `image_url` data URL；`detail` 可按模型支持选择 `high`/`auto`。
- 输出上限 4k–6k tokens；`finish_reason=length` 时提示分批，不要静默截断。
- 多图不要塞进一个请求（成本和输出都不可控）；一张图一次请求，串行或最多 2 并发。
- 失败分类：网络、401/403、429、5xx、JSON 解析、内容为空、模型不支持图片；不同错误给不同操作。
- 重试只对网络/429/5xx，最多 2 次，指数退避 + jitter；重试前提示可能再次计费。
- 去重：题干规范化后 SHA-256 + 三元组相似度；只提示，不自动删。

#### 3.9.4 AI 考试 / AI 组卷（AI Exam）

**现状**：桌面新增了“AI 考试”入口，文案是“描述薄弱点 → AI 诊断 → 从题库生成针对性练习卷 → 计时作答”；但截至快照 `PaintExam` 还没有实现，不能当作稳定功能。它和“试卷合成”不同：前者是诊断+针对练习，后者是把已选题目排版导出。

**移动端建议分两层**

- **v1：诊断 + 题库组卷 + 计时作答**。复用桌面的思路：教师描述薄弱点/知识点，AI 输出诊断和选题建议，App 从本地题库按标签/难度选题，生成练习卷；支持计时、交卷、查看答案解析。所有题目来自题库，AI 不生成新题，正确性风险最低。
- **v1.1：AI 生成新题**。如果确实需要“从零出卷”，再引入下面的蓝图/生成/审核流程；这是新功能，需要单独的成本模型和合规评估。

**AI 生成新题的产品定义（phase 2）**

- 输入：标题、年级、考试时长、题型数量/分值、难度分布、知识点范围、是否允许 AI 生成新题、是否只从题库选。
- 处理：
  1. 先生成“蓝图”（题型/难度/知识点分布），让教师确认；
  2. 从题库检索候选（按标签/难度/年级）；
  3. 不足部分再让 AI 生成新题；分 3–5 题一批，避免长输出截断；
  4. 每题校验 JSON、答案/解析非空、标签合法；标记 `source=ai_exam`、`review_status=draft`；
  5. 去重、难度/知识点覆盖检查；
  6. 教师逐题替换/删除/排序，确认后才进入试卷。
- 输出：保存为 `papers` 草稿，再走 DOCX/PDF 导出。

**UI**

- 表单用底部弹层/分步向导，不要一屏塞满。
- 生成前显示预估 token/费用（基于当前模型价格配置），让用户确认。
- 生成中显示进度（第几批/共几批），可取消；取消保留已完成题目为草稿。
- 结果页显示“AI 生成，未审核”标记；提供“全部标为已审核”但需二次确认。
- 失败时保留蓝图和已生成题目，不要整单丢弃。

**风险与约束**

- 数学正确性无法在端上自动验证；不要宣传“AI 保证正确”。可以加“AI 自检”提示，但不能替代教师。
- 版权/题目来源：如果从题库选，保留来源；如果生成新题，提示教师审核是否与现有教材/考试题重复。
- 成本：一套 15 题的输出可能 3k–8k tokens；比单题标注贵一个数量级，必须让用户有感知。
- v1 如果资源不足，先做“规则组卷”（按标签/难度/题型自动选题），这已经能解决大部分需求；AI 组卷放 v1.1。

#### 3.9.5 函数图像（Function Graph）

**目标**：快速画 `y=f(x)`，用于讲解/出题/插图，不是完整 CAS。

**v1 范围**

- 显式函数，最多 3–4 条曲线；支持 `+ - * / ^`、括号、`sin/cos/tan`、`asin/acos/atan`、`ln/log`、`sqrt`、`abs`、`exp`、`pi`、`e`、常数。
- 平移/缩放、坐标轴、网格、刻度、图例、颜色、线宽。
- 点击/拖动显示追踪点 `(x, f(x))`；可选函数值表。
- 导出 PNG（2x）并插入题目附件；保存 `graph_spec` JSON 到题目。
- 不做的：隐函数、参数方程、极坐标、导数/积分符号计算、不等式区域（可放 phase 2）。

**实现**

- 桌面已有 `math_expr.cpp` 的 lexer/Pratt parser/求值/零点实现（单变量、隐式乘法、定义域 NaN）；移动端建议用 Dart 重写同一套语法并复用测试向量。不要用 `eval`；限制表达式长度（如 500 字符）和递归深度，防止恶意输入卡死。
- 采样：按屏幕像素列采样（宽度的 1–2 倍），用 `Path` 绘制；遇到 `|Δy|` 过大或不在定义域时断开路径，避免把渐近线连成竖线。
- 坐标变换：世界坐标 ↔ 屏幕坐标；`InteractiveViewer` 或自定义 `GestureDetector` 处理 pan/zoom；双指缩放围绕手势中心。
- 刻度：`nice numbers`（1/2/5×10^n），根据缩放动态调整；标签用 `TextPainter`。
- 性能：3 条曲线 + 1000 采样点在中端机上应稳定 60fps；重采样只在视口/表达式变化时做，不在每帧做。
- 导出：`RepaintBoundary.toImage(pixelRatio: 2)`，白色/透明背景可选；保存到临时文件后插入附件。

**为什么不用图表库**：`fl_chart` 等是统计图抽象，不适合任意函数、定义域断裂、渐近线、追踪点；自绘更简单可控。

#### 3.9.6 几何画板（Geometry Board）

**必须先定义范围**，否则会变成“重写 GeoGebra”：

**v1 基础作图（建议 4–6 人周）**

- 工具：选择/移动、点、线段、直线、圆、多边形、文字标签、角度标记、删除。
- 网格/吸附、颜色/线宽、撤销/重做、清空、导出 PNG。
- 对象命中测试（点优先，线/圆有容差）；拖动自由点；整体平移/缩放。
- 场景 JSON 持久化到题目附件；在题目详情中可重新编辑。
- 不含：约束推导、交点自动计算、轨迹、测量、圆锥曲线、动态 drag 依赖。

**phase 2 动态几何（12–20 人周，容易被低估）**

- 对象依赖图：中点、交点、垂线、平行线、角平分线、圆上点、切线；拖动时拓扑重算。
- 测量：长度、角度、面积、比例；轨迹/ locus；圆锥曲线。
- 约束求解/数值稳定性、退化情况、撤销栈、序列化版本迁移。
- 这部分是数学引擎，不是 UI 工作量；不要用“画板”两个字低估它。

**替代方案**：嵌入 GeoGebra 需要商业许可/品牌合规，WebView 还会带来性能和离线包体问题；除非已购买许可并接受 WebView，否则不建议。自研基础作图 + phase 2 动态几何更可控。

### 3.10 数学输入与公式

这是移动端体验的分水岭。建议分两步，不要在 v1 同时做“完整 LaTeX 编辑器 + OMML 导出”。

**v1：Unicode 纯文本 + 自定义数学键盘（与桌面完全兼容）**

- 桌面已有可参考的数学键盘：`keyboard.cpp` 的 123/ABC/函数三组、光标插入、退格/左右/回车/Tab、深色主题；移动端复用交互语义，但用 Flutter 重做 UI，并建议增加“常用/结构”两组（桌面目前是 3 组）。
- 数据格式：`stem`/`answer`/`analysis` 继续是 Unicode 纯文本；新增 `stem_format='unicode'` 仅为未来迁移标记。
- 键盘：
  - 顶部 tab：常用 / 希腊 / 函数 / 关系 / 结构；
  - 常用：数字、`+ − × ÷ = ≠ ≤ ≥ ±`、`( ) [ ] { }`、`/`、`^`、`_`、`√`、`π`、`°`、`∞`；
  - 希腊：`α β γ δ θ λ μ σ φ ω Δ Σ Π` 等；
  - 函数：`sin cos tan log ln lim ∑ ∫`；
  - 结构：分式模板 `□/□`、根号 `√□`、幂 `□²`/`□^□`、下标 `□ₙ`、方程组/分段（插入多行文本）；
  - 按键插入到光标处；有选区时用模板包裹选区；支持撤销/重做、长按变体、触觉反馈；
  - 系统键盘仍可用于中文题干；数学键盘是“附加面板”，可收起；
  - 物理键盘（iPad/Android 平板）支持 Tab/箭头/快捷键。
- 渲染：列表和编辑页用 `Text` + 数学字体 fallback；不追求分式上下叠排。复杂公式仍以可读单行呈现，和桌面/DOCX 一致。
- 字体：UI 用系统中文字体；数学符号建议内置 `Noto Sans Math`（OFL）或使用系统 fallback，避免 `√`、`∑`、`∈` 显示为方框；PDF 嵌入字体。
- AI 输出：继续要求 Unicode，服务端返回的 LaTeX 用移植的 `math_text` 归一化；对 `$...$`、`\frac`、`\sqrt` 做兜底；复杂矩阵/cases 提示用户手工整理。
- 好处：与桌面数据、DOCX、AI 提示词完全一致；不需要 OMML；风险最低。

**phase 2：可选 LaTeX 双写（+6–10 人周）**

- 增加 `stem_tex`/`options_tex`/`answer_tex`/`analysis_tex`（或 `content_json` 富文本），保留 Unicode 作为 fallback/搜索/导出。
- 编辑器用 `flutter_math_fork` 做实时预览；键盘生成 LaTeX 片段（`\frac{}{}`、`\sqrt{}`、`^{}`、`_{}`）。
- DOCX 两种方案：
  1. 仍导出 Unicode（最稳，但公式不美观）；
  2. 生成 OMML `m:oMath`（工作量大，需 Word 兼容测试）；
  3. 公式渲染成 PNG 插入 DOCX（保真但不可编辑、文件变大）。
- PDF 可用 `flutter_math_fork` 渲染成图片，或使用 MathJax/KaTeX 生成 SVG 再嵌入；不要假设 `pdf` 包能直接排 LaTeX。
- 迁移：旧题 `format='unicode'` 原样保留；新题可选 LaTeX；不要批量自动转换旧题（容易破坏语义）。
- 如果产品定位是“教师快速组卷/打印”，Unicode 足够；如果定位是“高质量数学排版”，LaTeX 是 phase 2 的核心投资，不要低估。

### 3.11 AI 客户端设计

**Provider 抽象**

```dart
abstract class AiProvider {
  Future<AiResponse> chat(AiRequest request, {CancelToken? cancel});
  Stream<AiChunk> streamChat(AiRequest request, {CancelToken? cancel});
  Future<AiTestResult> testConnection();
}
```

- 桌面已抽出 `AskAiChat` 通用文本请求，供 AI 考试、函数分析、几何草图的 prompt 使用；移动端也应保持“一个底层 chat + 多个业务 prompt/parser”，不要为每个功能复制 HTTP 代码。
- 统一 `api_base` 规范化：去掉尾部 `/`；若已包含 `/chat/completions` 不重复拼接；支持 `/v1`。
- 鉴权默认 `Authorization: Bearer`；少数兼容服务需要自定义 header 时，phase 2 加“高级设置”，不要污染主流程。
- 能力标记：`vision`、`jsonMode`、`maxOutputTokens`、`supportsStreaming`；用户可手动覆盖，因为兼容服务的声明不可靠。
- 请求：`temperature` 0.2（标签/识题）、`max_tokens` 按任务；`stream=false` 用于结构化输出，组卷可用 SSE 展示进度。
- 超时：连接 10s、读取 90–180s；图片 150s；测试 30s。超时后取消请求，不要留下悬挂 Future。
- 重试：仅网络/408/429/5xx；最多 2 次；尊重 `Retry-After`；401/403/400 不重试；重试前 UI 提示可能再次计费。
- 错误映射：把 provider 的英文错误转成中文可操作提示（检查 Key、模型是否支持图片、额度、网络、base URL）。
- 日志：只记录状态码、耗时、token 数、模型名；不记录 prompt、图片、Key、完整响应。调试模式需用户显式开启并本地保存，默认关闭。

**图片管线**

- iOS：`image_picker`/`camera` 可能返回 HEIC；用 `flutter_image_compress` 或平台编解码转 JPEG；处理 EXIF orientation。
- 压缩：最长边 1600px（可降到 1280px 省 token）、JPEG 82–85、白底；单张目标 < 1.5MB；总请求 < 8MB。
- 内存：逐张处理，不把 8 张原图同时解码到内存；使用临时文件 + `Uint8List`，完成后删除。
- Base64 只存在于请求体；不要写日志/数据库；请求结束后释放。
- 可选的“仅文字模式”：先端上 OCR，把文字发给文本模型，省 vision token；但数学公式/图形会丢失，必须明确提示，默认不用。

**AI 标注**

- 选中题目 → 逐题请求（或 2–3 题一批，视输出限制）；显示进度、成功/失败数。
- 只合并标签，不覆盖手工标签；记录 `source='ai'` 便于追踪。
- 已有标签库作为上下文；控制长度，避免把整个题库发过去。
- 失败保留原标签；错误可复制/重试。
- 批量操作可取消；取消后已完成的保留。

**AI 组卷**

- 先蓝图、后题目；分批生成；每批校验；失败批次可重试。
- 生成题目默认 `draft`，不直接进入“可导出”集合；教师确认后 `approved`。
- 保存 prompt 版本、模型、token、费用估算，便于复盘。
- 不要自动把 AI 生成题目标记为“已审核”。

### 3.12 DOCX / PDF 导出与分享

**DOCX：移植而不是重写版式**

桌面 `docx.cpp` 的关键参数：

- A4：11906×16838 twips；页边距上 1247、下/左/右 1134；页脚 567。
- 标题黑体/Arial，正文宋体/Times New Roman；字号 half-points。
- 按“选择 → 填空 → 解答”分节；组内 A→C 排序；题号连续。
- 选择题选项两栏：制表位 `4700` twips；不生成表格边框。
- 答案页 `pageBreakBefore`；页脚 `PAGE`/`NUMPAGES` 域；`settings.xml` 开启 `updateFields`。
- ZIP 是 Store-only（无压缩），CRC32 + DOS 时间；手机端可用 `archive` 的 store/deflate。

**移植要点**

- 用 Dart 字符串模板或 `xml` builder 生成同样的 part：`[Content_Types].xml`、`_rels/.rels`、`docProps/core.xml`、`docProps/app.xml`、`word/document.xml`、`word/styles.xml`、`word/settings.xml`、`word/footer1.xml`、`word/_rels/document.xml.rels`。
- DOS 时间用 `DateTime.now()` 转 1980 基准；注意本地时区。
- 写入 app 临时目录，再通过系统分享/文件选择器交给用户；不要假设“桌面”。
- **字体不嵌入 DOCX**：手机 WPS 可能替换宋体/黑体；最终版式验收以桌面 Word/WPS 为准。PDF 才嵌入字体保证一致。
- 公式：v1 是 Unicode 纯文本；如果 phase 2 引入 LaTeX，DOCX 需要 OMML 或公式图片，不能只把 LaTeX 源串写进 `<w:t>`。

**测试**

- 用固定种子题生成 10 套卷：全选择题、全填空、全解答、含答案/不含答案、含解析/不含解析、长题干、6 个选项、空答案。
- 解压检查 part 名称、XML 合法、关系 ID 完整；用 Word/WPS 打开不报“修复”。
- 与桌面版生成的 DOCX 做文本/分页对比；允许 ±1 页差异，但章节/题号/答案页必须一致。
- 在 iOS/Android 的 WPS/Word 各打开一次，记录字体替换情况。

**PDF：移动端新增**

- 用 `pdf` + `printing`；A4、同样的页边距/分节/页脚页码；答案页另起。
- 嵌入 Noto Serif SC/Noto Sans SC（OFL）或思源字体；不要依赖系统字体，否则不同设备不一致。
- 中文断行、标点避头尾、长公式/长题干分页要专门测试；`pw.MultiPage` 可自动分页，但不可分页的块要设 `Inseparable` 或拆分。
- 生成后直接用 `printing` 的预览/打印/分享；这是手机端最可靠的“所见即所得”。
- 文件命名：`<标题>_<日期>.pdf`/`.docx`；清理非法字符；临时文件在分享后延迟清理。

**分享/保存**

- 优先系统分享面板（微信/邮件/AirDrop/文件）；
- Android 保存到用户选择的位置用 SAF（`file_picker` 的 save 或平台 Intent）；
- iOS 用 Document Picker/分享面板保存到“文件”；
- 不要申请 `WRITE_EXTERNAL_STORAGE`；使用系统照片选择器和 SAF；
- 导出前提示文件可能包含题目/答案，分享给谁由用户决定。

### 3.13 任务队列与后台

- 所有 AI 任务写入 `ai_jobs`：`pending/running/succeeded/failed/cancelled`；应用重启后可恢复“未完成”任务（至少显示可重试）。
- 前台执行，显示进度；切后台时：
  - iOS 不保证继续；回到前台检查状态，超时则标记失败/可重试；
  - Android 可用前台服务 + 通知做长任务，但会增加权限/商店审核复杂度，v1 不建议；
  - 不要用 `Timer` 做“后台定时上传”，移动 OS 会挂起。
- 批量识题限制 8 张/批；每张完成即保存结果，不要等全部完成才写库。
- 取消：使用 `CancelToken` + 请求级 `Abortable`；取消后不要把部分 JSON 当成功。
- 网络恢复：只自动重试“网络失败且未产生费用”的任务；其他让用户点重试。

---

## 4. 分阶段路线图与工作量

### 4.1 估算假设

- 1 人周 = 5 人天；估算包含设计、开发、单测、联调，不含应用商店排队、法务审批、市场推广。
- 团队假设：2.5 名有效投入（1 名资深 Flutter、1 名 Flutter/全栈、0.5 名设计/QA），或者 2 名资深 + 兼职设计/QA。
- 开发者使用 AI 辅助编码，但仍按正常 review/测试估算；不假设“10 倍效率”。
- 六屏全量 v1 指：题库、标签、AI 识题、AI 组卷、函数图像、几何画板（基础作图）+ 试卷导出。

### 4.2 阶段划分

| 阶段 | 范围 | 人周 | 日历（2.5 FTE） | 退出标准 |
| --- | --- | ---: | --- | --- |
| **Phase 0：技术验证** | Flutter 壳、Drift、桌面 JSON 导入、DOCX 移植 spike、相机→压缩→视觉模型 spike、数学键盘/渲染选型、Keychain/Keystore 验证 | 4–6 | 2–3 周 | DOCX 在 Word/WPS 无修复打开；10 张图片识题成功；Key 重启可读且不在备份/日志 |
| **Phase 1：MVP（建议首发）** | 题库 CRUD/搜索/筛选、标签、题目编辑、AI 识题、AI 标注、试卷合成、DOCX/PDF、分享、设置/密钥、导入导出、离线 | 30–38 | 3.5–5 个月 | 教师可完成“拍题→校对→入库→选题→导出→打印”闭环；崩溃率/性能达标 |
| **Phase 2：增强** | AI 组卷、函数图像、基础几何画板、LaTeX 预览（可选）、平板布局、同步预研 | 14–20 | 2–3 个月 | AI 组卷/图形题可用；教师试点反馈；成本可控 |
| **Phase 3：平台化** | 动态几何、云同步/账号、协作/分享库、订阅/配额、国内渠道/国际化 | 18–28 | 3–4.5 个月 | 多设备/多用户；商业化与合规就绪 |

**如果六屏必须全部进入 v1**：把 Phase 1 + Phase 2 的部分合并，约 **40–56 人周，2.5 FTE 日历 5–7 个月**（含 20% 缓冲）。这是偏乐观但可执行的范围；如果团队只有 1–2 人，按 7–9 个月排。

### 4.3 功能级估算

| 工作包 | 人周 | 备注 |
| --- | ---: | --- |
| 基础架构、设计系统、路由、CI、崩溃/日志 | 4–5 | 含深色主题、空状态、加载/错误组件 |
| Drift schema、DAO、迁移、桌面 JSON 导入/导出 | 3–4 | 含标签重建、ID 冲突、字段兼容 |
| 题库列表/搜索/筛选/多选/批量操作 | 4–5 | 含 1k 题性能、空状态、下拉刷新 |
| 题目编辑（含选项/标签/校验/未保存拦截） | 3–4 | 全屏表单 + 数学键盘 |
| 标签容器（重命名/合并/删除/计数） | 2–3 | 与题库联动 |
| AI 客户端、Provider 适配、错误/重试/取消 | 2–3 | 不含具体业务 prompt |
| 图片识题（相机/相册/压缩/队列/校对/导入） | 5–7 | 最大不确定项之一 |
| AI 标注 | 2–3 | 可复用识题的标签 UI |
| 试卷合成、DOCX、PDF、分享/打印 | 5–7 | DOCX 移植 + PDF 新做；测试量大 |
| AI 组卷 | 3–4 | 不含复杂数学验证 |
| 函数图像 | 3–4 | 表达式解析 + 绘制 + 导出 |
| 几何画板（基础作图） | 4–6 | 不含动态约束 |
| 安全/权限/设置/备份 | 2–3 | Keychain/Keystore、隐私页、权限文案 |
| 测试、性能、无障碍、商店合规 | 3–5 | 真机矩阵、iOS/Android 商店材料 |
| **合计（六屏）** | **40–56** | 与上表一致 |

### 4.4 建议排期（六屏全量）

1. **第 1–3 周**：Phase 0 spike；确定数学格式、DOCX 方案、相机方案；冻结 v1 schema。
2. **第 4–8 周**：数据层、题库/标签/编辑、导入导出；完成设计系统。
3. **第 9–13 周**：AI 识题/标注、任务队列、校对 UI。
4. **第 14–18 周**：试卷合成、DOCX/PDF、分享/打印；内部 dogfood。
5. **第 19–22 周**：函数图像、基础几何、AI 组卷（可与上并行，取决于人力）。
6. **第 23–26 周**：性能/无障碍/安全/商店；教师试点；修 bug。
7. **第 27 周起**：缓冲/发布；不要排满。

### 4.5 风险登记表

| 风险 | 概率 | 影响 | 缓解 |
| --- | --- | --- | --- |
| 数学排版不达预期（Unicode 太丑/LaTeX 太重） | 高 | 高 | Phase 0 做键盘+预览 spike；v1 明确 Unicode 边界；LaTeX 双写放 phase 2 |
| DOCX 在手机 WPS/Word 版式漂移 | 高 | 高 | 用桌面 DOCX 做 golden test；字体不嵌入要提前告知；最终以桌面打印为准；PDF 兜底 |
| AI 输出 JSON 不稳定/截断 | 高 | 高 | 严格 schema、分批、`finish_reason` 检查、重试/降级、保留草稿 |
| 视觉模型识题/解题错误 | 高 | 高 | 强制人工校对；缺答案/解析高亮；不要自动入库；显示“AI 可能出错” |
| 几何画板范围失控 | 高 | 高 | v1 只做基础作图；动态几何单独立项；不要承诺 GeoGebra 等价 |
| 密钥丢失/备份恢复失败 | 中 | 高 | 禁用 Android 自动备份 secure storage；明确“换机需重填”；测试恢复场景 |
| iOS 审核/隐私材料被打回 | 中 | 中 | 隐私清单、权限文案、数据安全表单；不要纯 WebView；提前准备审核说明 |
| 国内安卓渠道/备案/合规 | 中 | 高 | 提前确认目标市场；APP 备案/ICP/隐私政策；渠道包测试 |
| 大图 OOM/HEIC 兼容 | 中 | 中 | 逐张处理、限 1600px、转 JPEG、isolate、真机矩阵 |
| 中文搜索性能/分词 | 中 | 中 | 先 `LIKE`，量大再 FTS5 trigram；基准测试 |
| Flutter 数学库维护停滞 | 中 | 中 | 封装 `MathView`；pin 版本；准备 WebView/KaTeX fallback |
| 进度乐观/范围蔓延 | 高 | 高 | 固定首发四屏；六屏需重新立项；每两周 demo/砍 scope |
| AI 成本超预算 | 中 | 中 | BYOK；预估/确认；限制图片数；缓存/去重；不自动重试 |
| 学生数据/图片泄露 | 低–中 | 极高 | 默认不存图；脱敏；不上传日志；隐私政策；最小权限 |

---

## 5. 安全、隐私与成本

### 5.1 威胁模型（移动端）

| 威胁 | 场景 | 对策 |
| --- | --- | --- |
| 设备丢失/被他人使用 | 教师手机里有题库、学生照片、API Key | 系统锁屏 + 可选应用锁；Key 设备绑定；数据库 app-private；敏感备份排除 |
| 恶意应用/root | 读取 app sandbox/剪贴板 | 不把 Key 放剪贴板历史；不导出 Key；敏感场景可选生物识别；不要承诺越狱设备绝对安全 |
| 网络拦截 | 公共 Wi-Fi、恶意代理、HTTP base URL | 默认 HTTPS；仅显式允许 localhost/局域网 HTTP；校验证书；不关闭 TLS 校验 |
| AI 服务商留存 | 图片/题目被用于训练或长期保存 | 选择不训练/可零留存的 API；隐私政策披露；敏感图片先脱敏；允许用户关闭 AI |
| 日志/崩溃报告 | prompt、图片、Key 进入 Sentry/日志 | 全局脱敏；关闭请求体；采样；本地调试日志默认关闭 |
| 导出/分享 | DOCX/PDF/备份被转发 | 分享前提示；临时文件清理；不写公共目录；可选加密备份（phase 2） |
| Prompt injection/内容注入 | 模型输出包含恶意文本/HTML | 把输出当纯文本；校验/截断；不执行；WebView 若渲染 Markdown 必须 sanitize |
| 供应链 | 第三方 Flutter 包/插件漏洞 | 锁定版本、审许可证、最小依赖、定期升级；关键包做安全评估 |

### 5.2 学生数据与合规

- 题库可能包含学校内部考试题、学生姓名/学号/成绩；拍照识题的照片可能包含姓名、班级、人脸。按“可能包含未成年人个人信息”处理，至少按敏感数据设计。
- **默认不保存原始照片**：AI 请求完成后删除临时文件；用户显式选择“保留原图”时才写入附件，并提供一键清理。
- **上传前可见**：用户能看到将发送的图片和文本；不要后台自动上传；不要默认把整个题库/标签库发给模型。
- **最小化**：只发当前题目/图片；标签上下文只发名称，不发题目全文；不要发学生信息。
- **日志最小化**：不记录题干、答案、图片、Key；只记录任务 ID、模型、耗时、token、状态码。
- **权限最小化**：iOS 相机/相册/本地网络用途文案；Android 用系统照片选择器（Photo Picker），避免 `READ_EXTERNAL_STORAGE`/`MANAGE_EXTERNAL_STORAGE`。
- **数据删除**：提供“清空题库/删除附件/删除 Key”；卸载后系统会清理 app 数据，但用户备份/分享出去的文件不受控。
- **备份**：iOS 的 iCloud 备份可能包含数据库；如果数据敏感，使用 `NSURLIsExcludedFromBackupKey` 排除大缓存/附件，或提供“加密备份”。Android 关闭 `allowBackup` 或排除 secure storage；注意这会导致换机不恢复，需在 UI 说明。
- **目标市场合规**：
  - 中国：《个人信息保护法》《数据安全法》《网络安全法》《未成年人保护法》；不满 14 周岁未成年人个人信息属敏感个人信息，需监护人同意；生成式 AI 服务另有规定；App 上架通常涉及 APP 备案/ICP（若有服务端）、隐私政策、权限说明、SDK 清单。若只做本地工具 + BYOK，合规面小很多，但仍要处理照片中的学生信息。
  - 欧盟：GDPR（合法性基础、数据最小化、删除权、跨境传输）；美国：视客户类型可能涉及 FERPA/COPPA/州隐私法。
  - 应用商店：Apple 隐私清单/隐私标签、Google Play Data safety、照片/相机权限说明；AI 生成内容/用户内容审核要求。
  - 以上不是法律意见；发布前让法务按目标市场复核。

### 5.3 API Key 的具体规则

1. **只存 Keychain/Keystore**，不进 SQLite、SharedPreferences、日志、崩溃报告、分析事件、Git、CI 变量。
2. **默认不跨设备同步**；换机/重装需重新输入。若用户开启 iCloud Keychain，明确风险。
3. **Android 备份策略**：`android:allowBackup="false"` 或排除 secure prefs；否则恢复后 Keystore 密钥不匹配，出现“Key 在但解不开”。
4. **Key 掩码**：只显示后 4 位；复制/粘贴时不清空剪贴板历史（系统限制），提示用户。
5. **错误信息**：不要把带 Key 的请求头/URL 打进错误；401 只提示“Key 无效或无权限”。
6. **撤销/轮换**：提供“删除 Key”；如果怀疑泄露，引导去 provider 控制台撤销。
7. **代理模式**：如果以后收费，provider Key 只放服务端；客户端 token 短期、可撤销、按用户配额；不要用同一个共享 Key 打包进 App。

### 5.4 AI 请求与成本模型

**成本公式**

```text
单次成本 ≈ 输入 tokens × 输入单价 + 输出 tokens × 输出单价
图片还会按视觉 token 计费（不同 provider 差异很大，通常与分辨率/细节档位相关）
```

**典型 token 量级（用于预算，不是报价）**

| 场景 | 输入 tokens | 输出 tokens | 说明 |
| --- | ---: | ---: | --- |
| 图片识题/张 | 1.5k–3k + prompt 0.8k–1.5k | 1.5k–4k | 1–6 题；高 detail 更贵 |
| AI 标注/题 | 0.8k–1.5k | 100–300 | 只输出标签 JSON |
| AI 组卷/套 | 2k–5k | 3k–8k | 10–20 题；分批更稳 |
| 连接测试 | <100 | <50 | 可忽略 |

**示例（假设输入 $0.50/M tokens、输出 $2.00/M tokens，仅用于演示）**

- 图片识题：3k 输入 + 2k 输出 = $0.0015 + $0.004 = **$0.0055/张**；100 张 ≈ **$0.55**。
- AI 标注：1.5k + 200 = $0.00075 + $0.0004 = **$0.00115/题**；1,000 题 ≈ **$1.15**。
- AI 组卷：4k + 6k = $0.002 + $0.012 = **$0.014/套**；100 套 ≈ **$1.40**。
- 一个教师月用量（200 张图 + 500 题标注 + 20 套卷）≈ **$1.96/月**；1,000 名教师 ≈ **$1,960/月**。
- 如果使用更贵的高质量视觉模型，按 5–10 倍估算，1,000 名教师可能 **$10k–20k/月**。实际价格以 provider 账单为准。

**成本控制**

- **BYOK**：用户直接向 provider 付费，开发者不承担推理成本；适合早期。
- **模型分级**：标签用便宜文本模型；识题用视觉模型；组卷用强模型但限制次数。
- **图片限制**：单次 8 张、最长边 1600px、JPEG 82–85；重复图片按 SHA-256 去重，不重复上传。
- **缓存**：同一题目标签结果缓存；未修改不重复标注；prompt 稳定以利用 provider 的 prompt caching（若支持）。
- **预估与确认**：AI 组卷/批量识题前显示预估 token/费用；超过阈值二次确认。
- **重试收费**：重试前提示“可能再次计费”；4xx 不重试；429 尊重 `Retry-After`。
- **预算上限**：设置每月/每次上限；达到后只允许手动继续；代理模式按用户配额。
- **监控**：记录 token/任务/模型/费用估算；按天聚合；异常（单次输出暴增、重复重试）告警。
- **不要**：把共享 Key 打进 App；把整个题库作为上下文；对每张图自动重试 5 次；在后台无限重试。

**服务端成本（如果未来做代理）**

- 主要是鉴权、限流、请求转发、日志/监控、出口流量；通常小于模型费用，但合规、运维、客服成本更高。
- 代理会接触题目/图片，必须做数据最小化、留存策略、加密、访问审计；如果目标是中国，服务器地域/备案/跨境传输要提前设计。

---

## 6. 具体建议

### 6.1 最终推荐

**用 Flutter 做移动端，Dart 重写领域/数据/AI/导出；v1 只做本地优先 + BYOK，不做 C++ FFI、不做自建服务端、不做纯 PWA。**

理由：

1. **产品形态匹配**：题库、公式、图形、A4 试卷都需要高质量自绘和稳定布局；Flutter 的 Canvas/Widget 统一模型比 WebView/RN 桥接更直接。
2. **成本可控**：单代码库、单测试体系；不需要双端团队；不需要维护 C++ 多 ABI 构建。
3. **离线可靠**：SQLite + 本地文件，AI 只是增强；符合教师“随手拍、随时改、随时导出”的场景。
4. **安全边界清晰**：Key 在 Keychain/Keystore；题目/图片默认不上服务端；BYOK 避免开发者成为数据控制者。
5. **可演进**：后续可加 AI 组卷、函数图像、几何画板、同步、iPad 布局，不需要推翻架构；如果未来做 Web 版，Dart/Flutter Web 或独立 Web 前端都可选。

### 6.2 明确不要做

- 不要把桌面 1160px 三栏 UI 直接缩小到手机；不要保留 hover/右键/固定侧栏交互。
- 不要为了“复用 C++”把 Win32/GDI+ 代码硬塞进 FFI；先把平台依赖抽干净，否则会得到一个更难维护的混合体。
- 不要第一版做完整 LaTeX 编辑器/OMML 导出；先把 Unicode 输入、AI 归一化、DOCX 版式跑通。
- 不要第一版做动态几何/GeoGebra 等价；基础作图 + 导出已经能覆盖大量场景。
- 不要把“AI 组卷”当成现有“试卷合成”的改名；它是新功能，需要新 prompt、校验、审核和成本模型。
- 不要用纯 PWA 上架；不要在浏览器里存 API Key；不要用共享 Key 打包。
- 不要在没有同步元数据的情况下承诺“多设备同步”；先做备份/导入导出。

### 6.3 建议的首发范围（v1.0）

**必做（四屏核心）**

- 题库：列表、搜索、筛选、多选、详情/编辑、标签。
- AI 识题：相机/相册、多图、压缩、逐张队列、校对、批量入库。
- AI 标注：选中题目、逐题/小批、进度、失败保留。
- 试卷合成：标题/时间/注意事项、答案/解析/书写空间、DOCX + PDF、分享/打印。
- 基础：离线 SQLite、桌面 JSON 导入/导出、Keychain/Keystore、设置、深色模式、隐私页。

**建议 v1.1（第二波）**

- AI 组卷（蓝图 + 题库优先 + 分批生成 + 审核）。
- 函数图像（显式函数、追踪、导出 PNG）。
- 基础几何画板（点/线/圆/多边形/标注/导出）。
- 平板双栏布局。
- 可选 LaTeX 预览（不改变 DOCX 主流程）。

**phase 2/3**

- 动态几何、云同步/账号、协作、订阅/配额、国际化、国内安卓渠道。

### 6.4 前 30 天验证清单

1. **DOCX spike**：用 Flutter/Dart 生成一份与桌面版结构相同的试卷，Word/WPS 打开不修复；对比页数/题号/答案页。
2. **AI 识题 spike**：相机拍 10 张真实高中题（含公式/图形/多题），跑通“压缩→请求→JSON→校对→入库”；记录成功率、耗时、token/费用。
3. **数学键盘 spike**：实现常用/希腊/函数/结构四组键，插入/包裹选区/撤销；找 3 位教师试用 30 分钟，确认比系统键盘快。
4. **存储 spike**：1,000 题导入 SQLite，列表滚动/搜索/筛选性能；测试中文搜索。
5. **安全 spike**：iOS/Android 各测 Key 保存/重启/卸载重装/备份恢复；确认 Key 不在日志/导出文件。
6. **函数图像 spike**：3 条曲线、pan/zoom、渐近线断开、导出 PNG；中端 Android 机 60fps。
7. **商店预检**：准备隐私政策、权限文案、隐私清单/Data safety；确认 AI 内容/学生数据的披露方式。

### 6.5 Go / No-Go 指标

- DOCX 在 Word/WPS 打开无修复，题号/分节/答案页正确；与桌面版差异可解释。
- 1,000 题冷启动 < 2s，列表滚动稳定 60fps，搜索 < 200ms。
- 10 张真实图片识题，目标模型成功率 ≥ 90%，单张成本/耗时在可接受范围；失败可重试/可跳过。
- Key 在 iOS/Android 重启后可读，卸载/备份恢复行为符合预期，日志/导出无明文。
- 教师试用能独立完成“拍题→校对→入库→组卷→导出”，不需要看说明书。
- 崩溃率/ANR 在试点期达标；AI 相关错误有明确提示，不阻塞手动流程。

如果上述指标在 Phase 0/1 达不到，优先砍 AI 组卷/几何画板，保核心闭环；不要用“加人”掩盖架构问题。

---

## 7. 参考资料

- Flutter 官方文档：<https://docs.flutter.dev/>
- Drift（SQLite）：<https://pub.dev/packages/drift>
- flutter_secure_storage：<https://pub.dev/packages/flutter_secure_storage>
- flutter_math_fork：<https://pub.dev/packages/flutter_math_fork>
- Dart PDF：<https://pub.dev/packages/pdf>
- Printing：<https://pub.dev/packages/printing>
- Apple App Review Guidelines（4.2 Minimum Functionality）：<https://developer.apple.com/app-store/review/guidelines/#minimum-functionality>
- Apple Keychain Services：<https://developer.apple.com/documentation/security/keychain_services>
- Android Keystore：<https://developer.android.com/privacy-and-security/keystore>
- Android Photo Picker：<https://developer.android.com/training/data-storage/shared/photopicker>
- OWASP MASVS：<https://mas.owasp.org/MASVS/>
- SQLite FTS5：<https://www.sqlite.org/fts5.html>
- Google ML Kit Text Recognition v2：<https://developers.google.com/ml-kit/vision/text-recognition/v2>

---

*本文只读评估，未修改任何源代码或数据文件。*
