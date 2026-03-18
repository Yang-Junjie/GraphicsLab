# GraphicsLab

一个基于 C++17、OpenGL 4.6 和 ImGui 的实时图形实验仓库，用来集中实现和验证渲染管线、场景系统、相机系统、模型加载、计算着色器压力测试，以及曲线/曲面细分等图形学主题。

当前可执行程序名为 `CGLab`，启动后通过 ImGui 面板在多个实验场景之间切换。

## 项目亮点

- 多场景实验平台：统一的 `Scene` 抽象，支持在运行时切换不同渲染主题
- OpenGL 封装：对 `Shader`、`Buffer`、`VertexArray`、`Framebuffer` 做了 RAII 包装
- 2D 批渲染路径：`Renderer2D` 使用持久映射 SSBO、三缓冲和 GPU Timer Query
- 多线程帧流水线：`JobSystem` 将 `OnUpdate` 与主线程渲染/ImGui 解耦
- ImGui 工作台：Docking、Viewport、场景参数调节、统计面板一体化
- 3D 功能覆盖：相机控制、基础光照、阴影、HDR、Bloom、glTF 模型加载
- 图形学实验：Bezier 曲线细分、Bezier 曲面细分、Compute Shader 压测

## 技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++17 |
| 构建系统 | CMake 3.15+ |
| 图形 API | OpenGL 4.6 Core |
| 窗口与输入 | GLFW |
| GUI | Dear ImGui（Docking + Viewports） |
| 数学库 | GLM |
| 模型加载 | fastgltf |
| 图像加载 | stb_image |
| 并发 | `std::thread` / `std::atomic` / `std::condition_variable` |

## 已实现的场景

| 场景 | 内容 |
|------|------|
| `Basic Shapes` | 最基础的三角形示例，手工编译 shader、创建 VAO/VBO，并通过 `glMultiDrawArraysIndirect` 提交绘制 |
| `Texture Quad` | 纹理四边形显示，演示图片加载、纹理创建、缩放和纵横比适配 |
| `Camera Test` | 统一验证 2D 正交、3D 透视、3D 正交三种相机模式与输入控制 |
| `Base Lighting` | 多光源基础光照，包含方向光、点光源、聚光灯、材质高光和贴图 |
| `AdvanceLighting` | 进阶光照实验，包含阴影映射、Blinn-Phong/Phong 切换、HDR、Bloom、Gamma 校正 |
| `Model Viewer` | 基于 glTF 的模型加载与渲染，当前内置 `WaterBottle` 模型 |
| `Stress Test` | CPU 并行更新与 GPU Compute Shader 更新两条路径的粒子压力测试 |
| `Bezier Curve` | 基于 TCS/TES 的三次 Bezier 曲线细分，可调控制点和细分等级 |
| `Bezier Surface` | 基于曲面细分着色器的 4x4 Bezier 曲面，可调控制笼和显示模式 |

## 目录结构

```text
.
├─ src/                核心源码
│  ├─ Backend/OpenGL/  OpenGL 封装
│  ├─ Camera/          2D/3D 相机系统
│  ├─ JobSystem/       线程池与同步原语
│  ├─ Model/           glTF 模型与材质
│  ├─ Renderer/        2D 批渲染器
│  └─ Scene/Scenes/    各实验场景
├─ shaders/            GLSL shader
├─ res/                纹理、模型等运行时资源
├─ docs/               设计说明与实现笔记
└─ thrid_party/        第三方依赖与子模块
```

## 构建

### 依赖要求

- 支持 OpenGL 4.6 的显卡和驱动
- CMake 3.15 或更高版本
- 支持 C++17 的编译器

### 拉取仓库

如果是首次克隆，记得带上子模块：

```bash
git clone --recurse-submodules <your-repo-url>
cd GraphicsLab
```

如果仓库已经存在但子模块还没初始化：

```bash
git submodule update --init --recursive
```

### 生成与编译

```bash
cmake -S . -B build
cmake --build build --config Release
```

程序目标名为 `CGLab`。构建后会自动把 `shaders/` 和 `res/` 复制到运行目录。

常见输出位置：

- 单配置生成器：`bin/CGLab`
- 多配置生成器：`bin/Release/CGLab.exe` 或 `bin/Debug/CGLab.exe`

## 运行

在仓库根目录执行：

```bash
./bin/CGLab
```

如果使用的是多配置生成器，请从对应配置目录启动可执行文件。

## 交互方式

大多数 3D 场景共用以下控制方式：

- `W/A/S/D` 或方向键：移动
- `Space` / `Left Shift`：上升 / 下降
- 鼠标右键拖拽：旋转视角
- 鼠标滚轮：调整 FOV 或缩放

补充说明：

- `Camera Test` 的 2D 模式支持 `Q/E` 旋转
- `AdvanceLighting` 支持 `B` 键切换 Blinn-Phong / Phong
- 所有场景参数都可以在 ImGui 面板中实时调整

## 架构概览

### 帧执行模型

项目采用“主线程渲染 + 工作线程更新”的简单流水线：

1. 主线程等待上一帧的场景更新完成
2. 主线程执行 OpenGL 渲染并绘制 ImGui
3. 将当前场景的 `OnUpdate` 提交给 `JobSystem`
4. 工作线程并行计算下一帧状态

这种方式保持了 OpenGL/ImGui 只在主线程调用，同时把纯 CPU 逻辑更新移出渲染关键路径。

### 渲染层分工

- `Renderer2D`：处理四边形/圆形实例渲染与统计
- `Scene`：组织各实验主题的生命周期、更新、渲染与 UI
- `Model`：负责 glTF 模型、子网格和材质绑定
- `Backend/OpenGL`：封装底层 GL 对象与 shader 编译

## 文档

仓库内还包含一些单独的设计说明：

- `docs/JobSystem.md`
- `docs/Camera_System.md`
- `docs/Camera_Implementation_Summary.md`
- `docs/StressTestOptimization.md`

另外 `docs/FluidSim2D/` 和 `docs/Experimental/` 下保留了一些尚未正式接入主程序的实验代码与 shader 草稿。

## 备注

- 仓库目录中第三方文件夹名目前为 `thrid_party/`，这是现有路径命名，不是 README 的笔误
- 运行时依赖资源相对路径，建议从构建输出目录直接启动程序，不要手动拆散 `shaders/` 和 `res/`
