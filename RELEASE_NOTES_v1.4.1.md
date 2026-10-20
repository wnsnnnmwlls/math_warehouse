# 数研组卷 v1.4.1 · 桌面 v1.4 + 手机 v0.8

本次把桌面端的 **几何示意图 3D 斜二测** 同步到手机版。

## 桌面版 v1.4.0

- 立体几何示意图改为 x/y/z 斜二测投影
- AI 返回隐藏棱和表面
- 半透明面、隐藏棱虚线、前后顶点明暗
- 修复 v1.3.0 启动栈溢出
- 函数工具支持 `y=x` / `f(x)=x` 和中文描述 AI 画图

## 手机版 v0.8.0

新增 **几何示意图**：

- 入口：AI 组卷右上角几何图标，或设置 → 几何示意图 / AI 3D
- AI 根据中文几何题描述返回：
  - 顶点 x / y / z
  - 隐藏棱 `hidden`
  - 表面 `faces`
- 本地按斜二测投影绘制：
  - 半透明表面
  - 虚线隐藏棱
  - 前后顶点明暗
  - 点标签
- 内置“三棱锥示例”，不配置 AI 也能查看 3D 效果
- 手机版函数图像 / AI 画图和桌面版对齐

## 文件

- `MathPaperStudio-v1.4.0-Windows-x64.zip`
- `MathPaperStudio-v1.4.0.exe`
- `MathPaperStudio-Mobile-v0.8.0-arm64-v1.4.1.apk`
- `MathPaperStudio-Mobile-v0.8.0-armeabi-v7a-v1.4.1.apk`
- `MathPaperStudio-Mobile-v0.8.0-universal-v1.4.1.apk`

## 验证

- 桌面端实际启动、3D mock 渲染验证
- 桌面端 selftest 全部通过
- 手机端 `flutter analyze` 无问题
- 手机端 13 项测试通过，包含几何示例页手机宽度渲染
- 手机 APK v0.8.0 构建成功

## 说明

- 几何图是示意图，不保证严格透视比例
- AI 返回的顶点/棱/面关系可能出错，使用前请核对
- 手机端 AI 功能需要先配置模型和 Key
- 桌面版未签名，手机 APK 使用 debug 签名
