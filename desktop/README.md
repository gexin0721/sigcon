sigcon/
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