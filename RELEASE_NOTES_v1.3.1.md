# 数研组卷 v1.3.1 · 启动修复 + 函数图像

v1.3.0 的桌面版存在启动栈溢出：函数输入规范化时回写了输入框，`WM_SETTEXT` 触发 `EN_CHANGE`，再次调用 `ParseFunction()`，无限递归导致程序闪退/打不开。

v1.3.1 已移除回写并重新编译，桌面版可以正常启动。

## 桌面版 v1.3.1

- 修复启动栈溢出，程序可正常打开
- 函数工具支持完整写法：
  - `y=x`
  - `y=x^2-1`
  - `f(x)=sin(x)`
  - `f(x)=sqrt(x+1)`
- 自动取等号右边，不再报 `unknown identifier "y"`
- 新增 **AI 画图**：输入中文描述，让 AI 转成函数表达式并绘制
- 保留 **AI 分析性质**
- 上次 v1.3.0 的题库配图、待补全标记、题目 ID 搜索均保留

文件：

- `MathPaperStudio-v1.3.1-Windows-x64.zip`：含题库数据，推荐
- `MathPaperStudio-v1.3.1.exe`：单文件程序

## 手机版 v0.7.0

手机函数图像功能不变，继续附带：

- `MathPaperStudio-Mobile-v0.7.0-arm64-v1.3.1.apk`
- `MathPaperStudio-Mobile-v0.7.0-armeabi-v7a-v1.3.1.apk`
- `MathPaperStudio-Mobile-v0.7.0-universal-v1.3.1.apk`

入口：AI 组卷右上角函数图标，或设置 → 函数图像 / AI 画图。

## 验证

- 桌面端已实际启动验证，窗口可正常打开
- 桌面端 `selftest.exe` 全部通过
- 手机端 `flutter analyze` 无问题
- 手机端 12 项测试通过
- 函数输入 `y=x` / `f(x)=x` 解析测试通过

## 注意

- AI 画图需要先在 AI 接口中配置模型和 Key
- 只支持单变量显式函数，三角函数使用弧度
- 桌面版未签名，Windows SmartScreen 可能提示“未知发布者”
- 手机 APK 使用 debug 签名，适合内测/直接安装
