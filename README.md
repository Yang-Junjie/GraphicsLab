# GraphicsLab

一个基于 C++17 和 OpenGL 4.3 的实时图形渲染引擎与演示平台。

## 功能特性

- **实例化 2D 渲染** — 通过 SSBO 实现高效的四边形和圆形批量绘制，单次 Draw Call 提交大量几何体
- **多线程帧流水线** — Job System 将场景逻辑运算与 GPU 渲染解耦，实现并行执行
- **可插拔场景系统** — 抽象 Scene 基类，支持运行时切换不同演示场景
- **ImGui 集成** — 实时 FPS/帧时间监控、场景选择面板、离屏帧缓冲显示
- **OpenGL 后端抽象** — RAII 封装 VAO/VBO/EBO/SSBO/FBO/Shader，自动资源管理


## 技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++17 |
| 构建 | CMake 3.15+ |
| 图形 API | OpenGL 4.3 Core |
| 数学库 | GLM |
| 窗口管理 | GLFW |
| GUI | ImGui (Docking) |
| 着色器 | GLSL 460 Core |
| 多线程 | std::thread / std::atomic / std::condition_variable |

## 构建与运行

**前置要求：** CMake 3.15+、支持 C++17 的编译器（MSVC / GCC / Clang）

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

编译产物位于 `bin/` 目录，着色器文件会自动复制到 `bin/shaders/`。

```bash
./bin/CGLab      # 运行
```

## 演示场景

| 场景 | 说明 |
|------|------|
| **BasicShapesScene** | RGB 三角形，手动管理 Shader 与 VAO |
| **GridScene** | 12×8 色彩方块网格，测试四边形实例化渲染 |
| **CirclePatternScene** | 3 层旋转圆环 + 脉冲中心圆，展示圆形渲染与动画 |

## 架构概览

### 帧流水线

```
主线程                          工作线程
  │                               │
  ├─ 等待上一帧计算完成 ◄─────────┤
  ├─ 渲染当前帧到 FBO             │
  ├─ 提交 OnUpdate 任务 ─────────►├─ 计算下一帧场景数据
  ├─ ImGui 显示 FBO 纹理          │
  └─ 交换缓冲区                   └─ 完成后通知主线程
```

### 渲染管线

1. 场景将实例数据（颜色、位置、大小、旋转）收集到 CPU 端数组
2. `Renderer2D` 在 `EndScene` 时通过 SSBO 上传至 GPU
3. 顶点着色器从 SSBO 读取每实例数据，计算变换矩阵
4. 片段着色器处理着色（圆形使用 `smoothstep` 实现抗锯齿边缘）
5. 所有几何体渲染到离屏 FBO，最终以纹理形式在 ImGui 窗口中显示

