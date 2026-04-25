# `sigcon` 软件架构设计与落地规范

> 版本整合说明：本文在原始架构规范基础上，整合了以下五条落地补充：
> 1. L3 的定位澄清（最小软件单元，非上帝对象）
> 2. L2 装配的懒加载策略（控制 L3 装配规模）
> 3. 全工程统一模块格式规范（src/include/test/docs/example）
> 4. 双轨线程模型（共享任务池 + 专属驻守工人注册表）
> 5. **配置模块重设计**：取消 L2::Config，改为 L0 设置入口 + L3 直属配置窗口

---

## 1. 核心架构原则

本规范定义了 `sigcon` 软件部分的架构标准，旨在构建一个高内聚、低耦合、具备强防御性编程能力的系统。

### 1.1 主控制链与依赖方向

- **单向主控制链**：`UI -> 调度 -> 业务`
- **双盲隔离**：`UI` 层不知道业务的存在，业务层不知道 `UI` 的存在。两者绝对禁止直接交互。
- **唯一枢纽**：调度层是唯一合法的编排层，负责组织 `UI` 与业务的连接。
- **反向通知契约**：主依赖方向必须保持单向，但允许基于契约的反向通知。凡是 `UI -> 调度`、`业务 -> 调度`、`L2 -> L3` 这类"向上通知"场景，必须通过装配阶段注入的回调函数、标准通知接口或 Qt 信号槽完成，禁止下层直接持有上层具体对象。
- **依赖拓扑禁忌**：架构中绝对禁止出现**三角依赖**和**菱形依赖**。系统依赖必须尽量保持无环，禁止横向穿透破坏主控制链。

### 1.2 状态与资源管理

- 一份核心状态不能被多条路径并行主写。
- 一个核心实例不能被多个装配入口重复创建和并行管理（向下严格管理）。

### 1.3 全模块配置契约：`setConfig` 与 `upConfig` 统一入口

- `setConfig` 是所有 UI 模块和业务模块都必须默认具备的标准接口，其地位等同于面向框架的构造函数、析构函数这类默认存在的生命周期入口。
- `setConfig` 必须作为架构关键词长期保留，禁止不同模块各自命名为 `applyTheme`、`refreshFont`、`loadLanguage`、`resetPolicy`、`reloadRule` 等分散接口来替代统一入口。
- UI 模块的 `setConfig(...)` 只负责接收配置快照并刷新当前显示状态，例如字体、字样、语言、颜色、主题、控件布局参数；禁止在该接口中夹带业务调用、线程创建、磁盘读写或跨层依赖。
- 业务模块的 `setConfig(...)` 只负责接收业务配置快照并刷新当前业务行为参数，例如阈值、策略、设备参数、通信超时、算法开关；禁止在该接口中直接操作 UI、直接接管调度职责或越级修改其他模块。
- 调度层必须统一暴露 `upConfig(...)` 接口，专门供上一层调用。上一层只能通过 `upConfig(...)` 把配置意图下发给下一层调度器，再由该调度器继续拆分并调用直属模块的 `setConfig(...)`，禁止绕过调度链直接跨层灌配置。
- `upConfig(...)` 的职责是"接收上一层配置意图、拆分作用范围、转发给直属子对象"，它不是配置持久化入口，不是业务计算入口，也不是全局扫描入口。
- 全局配置变更（如换语言、换字体、改主题）属于低频操作，本质上是全局性参数而非模块级参数，必须全局生效，不存在局部豁免。

### 1.4 异常归属与健康归属

- **下一层的异常由上一层负责处理，下一层的健康状态由上一层负责管理。**
- 下层模块只负责三件事：执行、暴露自身状态、在必要时上报异常或健康变化；下层模块禁止反向承担上层的全局治理职责。
- 任意一层都只能管理自己的直属下一层，禁止跨层托管。例如 L2 只管理直属 L1 与直属业务实例，L3 只管理直属 L2 与应用级资源。
- 健康管理必须采用"父层持有、事件驱动、按需刷新"的模式，禁止把全局健康治理做成一个由 L3 常驻线程不断扫描全工程对象的方案。
- 如果 L3 需要知道更深层的状态，必须由 `L2 -> L3` 逐层汇总上报；L3 拿到的是上层化、收敛后的结果，而不是自己下钻扫描所有 L1/L0/业务对象。

---

## 2. 四层调度理论体系（L0 ~ L3）

调度层不是单一的"垃圾桶"，而是采用树状分工的四层调度体系。高层负责装配与生命周期，底层负责具体执行，**低层绝不能反向持有或越级承担高层职责**。

### L0（最小逻辑单元）

负责最基础、单次、结果唯一的调度行为。其核心特征是**拥有独立 UI 窗口与完整的交互闭环**，典型场景包括确认弹窗、进度弹窗、单次授权弹窗、**以及设置入口触发动作**等。L0 与 `support/utils` 的本质区别在于：utils 是无状态纯函数，无生命周期概念；L0 有窗口状态（打开/关闭/交互结果），有完整的显示—交互—结果回传生命周期。L0 不允许创建多线程，不负责更高层的窗口装配。

### L1（最小功能单元）

负责单一窗口、页面的调度闭环。负责将页面交互转成业务调用，再将结果转成页面状态。L1 必须向上暴露 `upConfig(...)` 供 L2 调用，并负责管理自己直属的 UI 模块、L0 动作单元和页面级局部异常与健康状态。所有 L1 装配的 UI 模块都必须暴露统一的 `setConfig(...)` 接口，供 L1 标准化注入。

### L2（最小业务单元）

模块级/业务域级调度器。负责多步骤流程编排与 L1 的切换，以及业务断点时的重试或流程回滚。L2 必须向上暴露 `upConfig(...)` 供 L3 调用，并负责管理直属 L1、直属业务模块的异常与健康状态。

**L2 对 L3 的感知归零**：L2 不包含任何 Config 模块，不主动通知 L3，不持有任何指向 L3 的通道。这是本规范与早期版本的核心差异，原因见第 2.1 节。

### L3（最小软件单元）

应用级总装配根。**L3 的职责边界是"软件级管理"，不包含任何业务逻辑**，其职责天然不可再向上拆分：再加一层只会引入等价的中转和额外耦合，没有实质收益。

L3 的固定职责为：
- 应用全生命周期管理（启动、关闭、崩溃兜底）
- 全系统**唯一**的线程与进程创建权、销毁权持有者，管辖共享任务池与专属驻守工人注册表两套线程基础设施
- 向下装配直属 L2，并在 L2 间完成信号槽连接
- **直接持有 `ConfigWindow`，作为应用级配置界面的唯一归属者**
- 接收 `ConfigWindow` 的配置变更通知后，将配置通过 `upConfig(...)` 逐层下发
- 作为全局致命异常的最后兜底网，不下钻扫描 L1/L0/业务对象

**L3 装配规模控制——懒加载策略**：L3 只在用户真正触发某个业务域时，才装配对应的 L2 实例并建立其信号槽连接；未被触发的业务域不参与装配，其代码路径在运行期不存在。`ConfigWindow` 同样遵循懒加载策略，首次唤起时才实例化。

---

### 2.1 配置模块重设计：为什么取消 L2::Config

> **写给后来者**：如果你看到这里，想把配置相关逻辑重新移回 L2，请先读完这一节。

早期版本在 L2 内设置了 `Config` 模块，职责是"接收配置请求、校验、持久化、通知 L3"。这个设计存在一个根本性的结构矛盾：

**L2 的核心约束是"不知道 L3 的存在"，但 L2::Config 的最后一步必须通知 L3。** 无论通过回调、信号槽还是接口指针实现，其本质都是 L2 在感知 L3——这不是写法问题，而是职责归属问题。把"决定拉起配置界面"这个应用级决策放在 L2，只会让 L2 永远无法与 L3 解耦，和没有设计约束时没有任何区别。

新方案的核心洞察：**在用户视角，"配置"从来就是一个独立的操作入口，不附属于任何业务域。** 用户点击设置按钮，这是一个独立的、最小的交互动作——它天然就是一个 L0 单元。L0 只做一件事：告诉上层"用户想进配置"。配置界面由谁管理、怎么拉起，L0 完全不知道，也不需要知道。

### 2.2 L0 设置入口：`SettingsEntryAction`

这是配置重设计引入的唯一新构件，职责边界极窄：

```cpp
// scheduler/l0_action/settings_entry/include/SettingsEntryAction.h

class SettingsEntryAction : public QObject {
    Q_OBJECT
public:
    explicit SettingsEntryAction(QObject* parent = nullptr);

    // L3 装配阶段调用，将此 L0 嵌入主界面某个触发点（按钮、菜单项等）
    QWidget* triggerWidget();

signals:
    // 唯一对外信号：用户请求打开配置界面
    // L3 在装配阶段连接此信号，L0 本身不知道谁在监听
    void userRequestedSettings();

private slots:
    void onTriggerClicked();
};
```

`SettingsEntryAction` 的铁律：
- 只负责捕获用户的"打开配置"意图并发出信号
- 不持有任何配置数据，不知道配置界面是什么、由谁创建
- 不知道 L3、L2 或任何调度层的存在
- 符合 L0 定义：拥有一个最小交互闭环（点击→信号），有生命周期，无业务逻辑

### 2.3 L3 对配置的直接管理

配置界面（`ConfigWindow`）是应用级资源，由 L3 直接持有。这是正确的归属——配置覆盖全软件，没有理由让任何一个业务域（L2）来托管它。

```cpp
// scheduler/l3_app/main_entry.cpp（伪代码示意）

class L3_App : public QObject {
    Q_OBJECT

    ConfigWindow* configWindow_ = nullptr;       // 懒加载，首次唤起时才实例化
    SettingsEntryAction* settingsEntry_ = nullptr;

public:
    void bootstrap() {
        settingsEntry_ = new SettingsEntryAction(this);

        // L3 装配阶段：连接 L0 的唯一信号
        // L0 不知道这里发生了什么，L3 自主决策
        connect(settingsEntry_, &SettingsEntryAction::userRequestedSettings,
                this, &L3_App::onUserRequestedSettings);

        // ...其余 L2 的懒加载装配
    }

private slots:
    void onUserRequestedSettings() {
        if (!configWindow_) {
            configWindow_ = new ConfigWindow(this);
            connect(configWindow_, &ConfigWindow::configChanged,
                    this, &L3_App::onConfigChanged);
        }
        configWindow_->show();
        configWindow_->raise();
    }

    void onConfigChanged(const AppConfig& newConfig) {
        // 配置下发专线：L3 → 各 L2::upConfig() → L1::upConfig() → setConfig()
        l2_network_->upConfig(newConfig.network);
        l2_device_->upConfig(newConfig.device);
        // ...
    }
};
```

配置持久化由 `ConfigWindow` 通过 `support/infra/storage` 防腐层完成，读写入口依然收敛，不扩散到各业务模块。

### 2.4 配置流转专线

```
用户点击设置按钮
    → SettingsEntryAction::userRequestedSettings()     [L0 信号]
    → L3::onUserRequestedSettings()                    [L3 装配连接]
    → ConfigWindow::show()                             [L3 持有，懒加载]
    → 用户修改配置并确认
    → ConfigWindow::configChanged()                    [L3 持有窗口的信号]
    → L3::onConfigChanged()
    → L2::upConfig() → L1::upConfig() → Business/UI::setConfig()
```

与早期规范相比，**L3 之后的下发链路完全不变**。变化只发生在"谁触发 L3 开始下发"——从原来的 L2::Config 主动通知 L3，改为 L3 自己持有配置窗口、自己感知变更、自己决定下发。

### 2.5 上一层托管下一层的责任链

为了避免低层越权和高层失控，异常与健康管理必须遵守如下固定责任链：

1. **L1 管** L0、UI 页面内局部对象、页面级业务适配对象。
2. **L2 管** 直属 L1、直属业务模块、模块级流程状态。
3. **L3 管** 直属 L2、线程池、驻守工人、进程资源、应用级生命周期，以及 `ConfigWindow`。

补充强制约束：
- 下层可以上报异常和健康变化，但不拥有全局解释权。
- 上层必须保存直属子对象的健康视图与异常处置策略，但不能替代更上层做越级决策。
- 禁止出现"L3 统一扫描全局所有对象健康"的实现。

---

## 3. 标准物理文件架构

为了在物理层面锁死依赖关系，整个工程的目录必须严格按照以下结构划分。同级目录之间绝对物理隔离。

```text
Project_Root/
├── assets/                    # 【静态资源域】图片、图标、样式表、多语言翻译包
├── Config/                    # 【配置文件域】默认配置、用户配置、配置模板、配置版本迁移文件
├── data/                      # 【运行数据域】缓存、导出结果、日志快照、运行期持久化数据
├── docs/                      # 【文档域】架构说明、API 协议、错误码对照表
├── lib/                       # 【第三方库域】(绝对隔离，禁止业务/UI 直接引用)
│   ├── spdlog/
│   ├── nlohmann_json/
│   └── ...
│
├── src/                       # 【核心源码区】
│   │
│   ├── ui/                    # 模块一：【纯 UI 层】(只能依赖 support，盲区：不知道业务和调度)
│   │   ├── components/        # 基础公共组件（无业务逻辑的自定义按钮等）
│   │   ├── panels/            # 局部业务面板（只抛出交互信号）
│   │   └── windows/           # 独立顶层窗口（主窗口、设置弹窗等）
│   │
│   ├── business/              # 模块二：【纯业务层】(只能依赖 support，盲区：不知道 UI，禁止弹窗)
│   │   ├── device_ctrl/       # 业务域：设备控制
│   │   │   ├── exceptions/    # 局部异常定义：DeviceException.h
│   │   │   ├── models/        # 数据模型：纯 C++ struct/class
│   │   │   └── DeviceManager.cpp
│   │   └── algorithm/         # 业务域：核心算法
│   │
│   ├── scheduler/             # 模块三：【调度层】(唯一允许同时 import UI 和 Business 的地方)
│   │   ├── l0_action/         # L0 动作编排：含独立 UI 窗口的最小交互单元
│   │   │   └── settings_entry/    # ← 设置入口 L0（SettingsEntryAction）
│   │   ├── l1_page/           # L1 页面调度：组装 UI Window + L0 动作单元
│   │   ├── l2_domain/         # L2 业务调度：组装 L1 + Business（处理重试/中断）
│   │   │                      # ※ 无 config/ 子目录，L2 不感知 L3，不持有配置职责
│   │   └── l3_app/            # L3 软件装配：全局控制台（最小软件单元）
│   │       ├── thread_pool/       # 【共享任务池】AppThreadPool；对外暴露 submit()
│   │       ├── worker_registry/   # 【专属驻守工人注册表】AppWorkerRegistry；对外暴露 WorkerHandle
│   │       ├── config_window/     # ← 【应用级配置窗口】ConfigWindow；由 L3 直接持有与管理
│   │       ├── app_context/       # 全局运行时上下文
│   │       └── main_entry.cpp     # 启动与兜底；懒加载装配 L2；全局 try-catch 收口
│   │
│   └── support/               # 模块四：【支撑域】(基石：被上层单向依赖，禁止反向 import 上层)
│       ├── core/
│       │   ├── exceptions/    # 全局异常基类：AppBaseException.h
│       │   ├── constants.h    # 全局常量、协议键名
│       │   └── shared_state.h # 极少量的、严格受控的全局状态
│       ├── infra/             # 基础设施区（第三方库的"防腐层"）
│       │   ├── logger/        # 封装日志库
│       │   ├── storage/       # 封装本地缓存/数据库（配置持久化由此完成）
│       │   └── ipc_network/   # 封装多进程/网络底层通信
│       └── utils/             # 纯函数工具箱；无状态，无生命周期
```

补充目录约束：
- `Config/` 只允许存放配置相关文件，禁止混入缓存、导出结果、临时数据。
- `data/` 只允许存放运行数据，禁止把默认配置、静态配置模板和配置定义文件放入 `data/`。
- 配置文件的读取入口、写入入口、版本迁移入口由 L3 直属的 `ConfigWindow` 通过 `support/infra/storage` 统一管理，禁止多个业务模块各自直连多个配置文件。
- `support/utils/` 与 `scheduler/l0_action/` 的边界：utils 是无状态纯函数工具；L0 是拥有独立 UI 窗口和完整交互生命周期的最小调度单元。两者不可混用。

---

## 4. 统一模块格式规范

**全工程所有模块，无论大小，无论所属层级，一律采用以下标准格式**。该规范适用范围从最底层的日志工具类，到最顶层的 L3 调度器，没有例外。

```text
<module_name>/
├── src/          # 实现文件（.cpp）
├── include/      # 对外头文件（.h）；外部调用者只允许依赖此目录
├── test/         # 自测代码；含独立 main 函数，自行负责，对上层不透明
├── docs/         # 本模块的接口说明、状态机说明、注意事项
└── example/      # 使用示例；仅引用 include/ 下的头文件，是接口契约的活文档
```

### 4.1 各目录的职责边界

**`include/`** 是对外的唯一窗口。外部模块只允许 `#include` 此目录下的文件，任何跨模块的直接 `src/` 引用均属于架构违规，Code Review 阶段机械拦截。

**`example/`** 的地位与 `docs/` 等价，但比文档更可信。example 的写法强制要求开发者以调用方视角审视接口，是接口设计收敛的内在约束。一旦 example 写起来别扭，说明接口本身需要重构，而不是 example 写法有问题。

**`test/`** 的测试边界与所属层级对齐：
- 业务模块 / UI 模块的 `test/`：自测本模块的单一功能，mock 外部依赖，不引入上层对象。
- L1 的 `test/`：集成测试页面调度逻辑，引入直属 UI 模块和 L0 的 `include/`，验证编排是否正确。
- L2 的 `test/`：集成测试业务域流程，引入直属 L1 和业务模块的 `include/`，验证重试/回滚逻辑。
- L3 的 `test/`：端到端冒烟测试，验证全局装配、线程池生命周期、驻守工人注册表生命周期、`ConfigWindow` 懒加载、致命异常兜底路径。

**上层对下层的 `test/` 实现细节永远不可见**，也不应关心——上层只依赖下层的 `include/`，下层自己保证合约履行。

### 4.2 强制约束

- 禁止跨模块共用 `test/` 文件夹。每个模块的测试完全自治。
- 禁止在 `example/` 中引用 `src/` 下的实现文件。example 只能使用公开接口，否则它就不是 example，而是白盒测试。
- 每个模块的 `include/` 必须能在不依赖 `src/` 的前提下被外部编译通过（纯声明，无实现泄漏）。

---

## 5. 异常防御与流转机制

贯彻"防御性编程"与"零信任"原则，异常体系必须基于元数据驱动，而非类名硬编码。

### 5.1 异常基类设计（`AppBaseException`）

所有异常必须派生自 `support/core/` 下的统一基类，基类必须携带以下元信息：
- **错误码（`errorCode`）**：统一错误字典定义。
- **严重等级（`level`）**：`FATAL`（崩溃）、`ERROR`（局部失败）、`WARN`（可忽略）。
- **处理策略**：`shouldRetry`（是否允许重试），`shouldShowToUser`（是否弹窗提示）。
- **信息隔离**：`developerLog`（含堆栈，写入本地日志），`userMessage`（友好的用户提示）。

### 5.2 预期错误与崩溃的分离

- **Result 模式**：用于预期内的业务分支（如搜索不到设备、密码错误），不抛异常。
- **Exception 模式**：用于预期外的崩溃（如断网、文件损坏、断言失败），直接抛出。

### 5.3 异常流转接力赛

1. **抛出点（底层）**：业务层或基建层遇到致命错误直接抛出派生异常，绝对不处理用户提示，只负责把错误事实抛给直属上一层。
2. **微调点（L1）**：L1 只处理自己直属下一层抛出的 `WARN` 级异常，转换为界面红字/黄牌警告，拦截异常防止页面崩溃。
3. **恢复点（L2）**：L2 只处理自己直属 L1 或直属业务模块抛出的 `ERROR` 级异常，根据 `shouldRetry` 执行模块级重连或流程回滚。
4. **兜底点（L3）**：L3 只兜底自己直属 L2 未消化的 `FATAL` 异常。记录崩溃日志，弹出全局致命错误框，安全释放多线程资源后退出。
5. **禁止事项**：禁止 L3 为了接管所有异常而额外创建一个全局扫描线程轮询所有层级对象；异常治理必须依赖逐层上抛和逐层收口，而不是全局轮询。

### 5.4 健康状态治理

- 健康状态不是"谁发现谁全局管理"，而是"谁是直属上层谁负责管理"。
- 下层模块应提供轻量的健康信号、状态快照或心跳事件，但这些信息只能先汇报给直属父层。
- 父层收到健康变化后，应立即更新自己的健康视图，并按需决定是否继续向上汇总，而不是把原始对象引用交给更高层直接扫描。
- 对于页面销毁、模块卸载、线程退出等生命周期变化，直属上层必须同步注销其健康记录，避免出现失效对象仍被全局表追踪的脏状态。

---

## 6. 多线程与多进程并发模型

### 6.1 双轨线程模型：共享任务池 与 专属驻守工人

全系统线程分为性质截然不同的两类，L3 在 `l3_app/` 内分别持有两套基础设施，统一拥有创建权与销毁权。

#### 轨道一：共享任务池（AppThreadPool）

适合**无状态短任务**：一次性计算、文件读写、数据库查询等。线程数量固定，任务随机调度，调用方不关心由哪个线程执行。

```cpp
// scheduler/l3_app/thread_pool/AppThreadPool.h
class AppThreadPool {
public:
    // 子单元唯一合法的异步入口；支持 TaskID 注册，便于 L3 感知任务生命周期
    static void submit(std::function<void()> task, TaskID id);

    // 仅供 L3 内部调用
    static void init(int maxThreads);
    static void shutdown();

private:
    QThreadPool pool_;
};
```

#### 轨道二：专属驻守工人注册表（AppWorkerRegistry）

适合**有状态长驻任务**：网络连接维护、设备心跳监听、串口轮询、IPC 守护等需要独立事件循环持续运行的场景。每个 Worker 是一个带独立事件循环的 `QThread`，有名字，有归属域。

```cpp
// scheduler/l3_app/worker_registry/AppWorkerRegistry.h
class AppWorkerRegistry {
public:
    // 仅供 L3 内部调用：在懒加载装配阶段创建 Worker
    static WorkerHandle registerWorker(const QString& name, AppWorkerBase* worker);

    // 仅供 L3 内部调用：在业务域卸载时销毁 Worker
    static void shutdown(const QString& name);

private:
    QMap<QString, AppWorkerBase*> workers_;
};
```

`WorkerHandle` 是 L2 持有的唯一使用凭证，其所有方法内部强制走 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`，将调用自动序列化到 Worker 自身的线程事件循环中，调用方无需关心线程切换。

#### 两轨线程的归属模型

**L3 创建并持有生命周期，L2 持有使用权。** 这与操作系统的资源管理模型一致：进程向内核申请线程，内核持有资源，进程拿到句柄使用。

```cpp
// L3 懒加载装配某业务域时的伪代码示意
void L3::activateNetworkDomain() {
    // 1. 创建专属驻守工人（L3 的权力）
    auto handle = AppWorkerRegistry::registerWorker("network_guard", new NetworkGuardWorker());

    // 2. 创建 L2，WorkerHandle 通过构造函数注入（依赖注入，不是 L2 自己拿）
    auto l2 = new L2_NetworkDomain(handle);

    // 3. 连接信号槽（L3 装配阶段）
    connect(handle.worker(), &NetworkGuardWorker::statusChanged,
            l2, &L2_NetworkDomain::onNetworkStatus);
}
```

L2 通过 `WorkerHandle` 只能向 Worker 提交指令或连接其信号，**不能**通过 Handle 销毁、重建或迁移线程。

#### 双轨线程与懒加载的协同

| 时机 | 共享任务池 | 专属驻守工人 |
|---|---|---|
| 启动时 | 预创建固定数量线程 | 不创建任何 Worker |
| 用户触发业务域 | 不变 | L3 为该域创建 Worker，注入对应 L2 |
| 业务域被关闭/卸载 | 不变 | L3 调用 `shutdown(name)`，销毁 Worker |

L3 的实际线程数量始终与用户正在使用的业务域数量对齐，而非工程中所有业务域的总数。

### 6.2 线程创建权的铁律

禁止在 `l3_app/` 之外的任何地方直接调用 `QThreadPool::globalInstance()`、`QtConcurrent::run()`、`std::thread`、`QThread` 构造函数。Code Review 阶段可通过关键字搜索机械检查，将"文档约定"转化为"可验证约束"。

### 6.3 依赖反转与契约通信

业务层（包括独立的网络线程或 IPC 通信进程）绝对禁止直接调用调度层或 UI。数据返回必须通过**标准契约**：即回调注入（`std::function`）或发布/订阅（信号与槽机制）。业务模块只负责"无脑"触发回调，不关心数据去向。

- `UI -> 调度`、`业务 -> 调度`、`L2 -> L3` 这类反向通知，本质上都属于"向上汇报而非反向持有"，实现上统一优先采用装配阶段注入的回调函数或标准通知接口，禁止为了通知方便而把父层对象指针长期下发给子层。
- `L2 <-> L2` 之间的通信，优先采用 Qt 的信号槽机制与事件系统完成，**信号连接必须在 L3 装配阶段完成，L2 代码文件内禁止出现另一个 L2 的头文件 include**，避免平级横向依赖悄然形成。
- 如果遇到跨进程、跨模块边界更重、或 Qt 信号槽不适合覆盖的特殊场景，最低可以退到 Redis 这类外部消息中介，但该方案默认不作为第一选择。

### 6.4 统一主线程事件序列化

- **绝对禁止非主线程直接修改 UI。**
- 当后台子进程/子线程通过回调或信号返回数据时，调度层必须将该行为封装为"事件"，压入**主线程事件队列（Event Loop）**。
- 所有并行到达的任务，在主线程中都会被序列化排队执行，从根本上消灭并发资源争抢。

### 6.5 基于 TaskID 的精确路由

为了将后台进程的结果准确送达对应的 L1 页面：

1. **发起**：L1 触发任务时，附带唯一的 `TaskID` 交给 L2。
2. **执行与返回**：底层业务/子进程原路带回 `TaskID`。
3. **路由分发**：L3/L2 根据内部映射表，将带有特定 `TaskID` 的结果派发给对应的 L1。若 L1 已被销毁，则安全丢弃数据，防止内存崩溃。

---

## 7. 架构落地的"五大铁律"

此五条作为 Code Review 的一票否决项：

**铁律一：跨层防腐**
UI 绝不能 `#include` 业务；底层代码绝不能包含任何第三方库的原始头文件（必须通过 `support/infra` 防腐层中转）。

**铁律二：权力下放禁止**
低层调度器（L1/L0）绝不能持有或私自创建高层调度器（L3/L2）；所有依赖必须自上而下通过构造函数注入。

**铁律三：线程权力分层**
全项目线程的**创建与销毁**只能发生在 `L3` 的专属目录内，涵盖共享任务池（`AppThreadPool`）与专属驻守工人注册表（`AppWorkerRegistry`）两套基础设施。L0/L1/L2 如需短任务异步能力，必须通过 `AppThreadPool::submit()` 提交；如需驻守线程，必须在 L3 装配阶段通过 `WorkerHandle` 注入，严禁自行构造 `std::thread`、`QThread`、`QThreadPool::globalInstance()` 或 `QtConcurrent::run()`。

**铁律四：配置专线**
用户触发配置的唯一入口是 `SettingsEntryAction`（L0），配置界面的唯一归属者是 L3 直属的 `ConfigWindow`。配置变更的下发必须严格经过 `L3 → L2/L1::upConfig(...) → Business/UI::setConfig(...)` 这条链路。禁止业务层直接改 UI，禁止任何层绕过 `setConfig` 私自同步全局配置，禁止 L2 承担任何配置感知或通知职责。

**铁律五：逐层托管**
下一层的异常由上一层负责处理，下一层的健康由上一层负责管理。禁止 L3 通过全局扫描线程统一轮询所有层级对象，禁止任何层越级接管不直属的对象。

---

## 附录：关键边界速查表

| 场景 | 正确做法 | 禁止做法 |
|---|---|---|
| 用户发起配置 | 点击 `SettingsEntryAction`（L0），L3 监听信号并拉起 `ConfigWindow` | L2 持有配置窗口；L2 主动通知 L3 |
| 配置变更下发 | L3::onConfigChanged → 各 L2::upConfig() → L1::upConfig() → setConfig() | 任何层级绕过 L3 直接跨层灌配置 |
| 配置持久化 | `ConfigWindow` 通过 `support/infra/storage` 写入 | 业务模块各自直连配置文件 |
| 换主题/换字体 | `ConfigWindow` 通知 L3，L3 通过 upConfig 逐层下发 | L2 直接调用 UI 控件接口 |
| 后台短任务异步执行 | `AppThreadPool::submit()` | `std::thread` / `QThread` 直接创建 |
| 后台长驻有状态任务 | L3 装配阶段 `AppWorkerRegistry::registerWorker()`，WorkerHandle 注入 L2 | L2 自行创建 `QThread` 或持有裸线程指针 |
| L2 通知 L3 | 装配阶段注入的回调 / Qt 信号槽 | L2 持有 L3 指针 |
| L2 间通信 | L3 装配阶段连接的信号槽 | L2 的 .cpp 中 include 另一个 L2 头文件 |
| 模块外部调用 | 只引用目标模块 `include/` 下的头文件 | 直接引用 `src/` 下的实现文件 |
| L3 感知深层健康状态 | L2 汇总后向 L3 上报 | L3 自行下钻扫描 L1/L0/业务对象 |
| 新业务域装配 | 用户触发时懒加载，L3 按需建立信号槽与 Worker | 启动时统一装配所有 L2 和 Worker |
| 预期内业务失败（设备未找到等） | Result 模式返回，不抛异常 | 抛出异常让上层 catch |
| 预期外崩溃（断网、文件损坏等） | 抛出派生自 `AppBaseException` 的异常 | 在底层自行处理并弹窗 |