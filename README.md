# GraphicsLab

一个基于 C++17 和 OpenGL 4.3 的实时图形渲染引擎与演示平台，展示了实例化渲染、多线程任务系统和可插拔场景架构。

## 功能特性

- **实例化 2D 渲染** — 通过 SSBO 实现高效的四边形和圆形批量绘制，单次 Draw Call 提交大量几何体
- **多线程帧流水线** — Job System 将场景逻辑运算与 GPU 渲染解耦，实现并行执行
- **可插拔场景系统** — 抽象 Scene 基类，支持运行时切换不同演示场景
- **ImGui 集成** — 实时 FPS/帧时间监控、场景选择面板、离屏帧缓冲显示
- **OpenGL 后端抽象** — RAII 封装 VAO/VBO/EBO/SSBO/FBO/Shader，自动资源管理

## 项目结构

```
GraphicsLab/
├── src/
│   ├── App.hpp/cpp                     # 应用入口
│   ├── DemoLayer.hpp/cpp               # 主演示层，管理场景与帧流水线
│   ├── Backend/OpenGL/                 # OpenGL 后端封装
│   │   ├── Buffer.hpp/cpp              # VBO / EBO / SSBO
│   │   ├── Shader.hpp/cpp              # Shader 编译与 Uniform 访问
│   │   ├── VertexArray.hpp/cpp         # VAO 状态管理
│   │   └── Framebuffer.hpp/cpp         # FBO 离屏渲染
│   ├── JobSystem/
│   │   └── JobSystem.hpp/cpp           # 线程池 + 原子计数器同步
│   ├── Renderer/
│   │   └── Renderer2D.hpp/cpp          # 2D 实例化渲染器
│   └── Scene/
│       ├── Scene.hpp                   # 场景抽象基类
│       ├── BasicShapesScene.hpp/cpp    # 三角形演示
│       ├── GridScene.hpp/cpp           # 色彩网格演示
│       └── CirclePatternScene.hpp/cpp  # 动态圆形图案演示
├── shaders/                            # GLSL 430 Core 着色器
│   ├── basic.vert / basic.frag         # 基础三角形
│   ├── quad.vert / quad.frag           # 实例化四边形
│   └── circle.vert / circle.frag      # 实例化圆形
├── docs/
│   └── JobSystem.md                    # Job System 详细文档
├── thrid_party/Flux/                   # 自研应用框架（GLFW + GLAD + GLM + ImGui）
└── CMakeLists.txt
```

## 技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++17 |
| 构建 | CMake 3.15+ |
| 图形 API | OpenGL 4.3 Core |
| 数学库 | GLM |
| 窗口管理 | GLFW |
| GUI | ImGui (Docking) |
| 着色器 | GLSL 430 Core |
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
./bin/CGDemo      # 运行
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

