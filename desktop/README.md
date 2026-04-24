desktop/
├── assets/                    # 【静态资源域】图片、图标、样式表、多语言翻译包
├── Config/                    # 【配置文件域】默认配置、用户配置、配置模板、配置版本迁移文件
├── data/                      # 【运行数据域】缓存、导出结果、日志快照、运行期持久化数据
├── docs/                      # 【文档域】架构说明、API协议、错误码对照表
├── lib/                       # 【第三方库域】(绝对隔离，禁止业务/UI直接引用)
│   ├── spdlog/                # 第三方日志库
│   ├── nlohmann_json/         # 第三方 JSON 库
│   └── ...                    
│
├── src/                       # 【核心源码区】
│   │
│   ├── ui/                    # 模块一：【纯 UI 层】(只能依赖 support，盲区：不知道业务和调度)
│   │   ├── components/        # 基础公共组件 (如：无业务逻辑的自定义按钮)
│   │   ├── panels/            # 局部业务面板 (如：状态显示面板，只抛出交互信号)
│   │   └── windows/           # 独立顶层窗口 (如：主窗口、设置弹窗)
│   │
│   ├── business/              # 模块二：【纯业务层】(只能依赖 support，盲区：不知道 UI，禁止弹窗)
│   │   ├── device_ctrl/       # 业务域：设备控制
│   │   │   ├── exceptions/    # 局部异常定义：如 DeviceException.h
│   │   │   ├── models/        # 数据模型：纯 C++ struct/class
│   │   │   └── DeviceManager.cpp 
│   │   └── algorithm/         # 业务域：核心算法
│   │
│   ├── scheduler/             # 模块三：【调度层】(中枢：唯一允许同时 import UI 和 Business 的地方)
│   │   ├── l0_action/         # L0 动作编排
│   │   ├── l1_page/           # L1 页面调度：组装 UI Window + 基础动作
│   │   ├── l2_domain/         # L2 业务调度：组装 L1 + Business (处理重试/中断)
│   │   │   └── config/        # L2 Config 模块：仅负责设置、校验、持久化、通知 L3
│   │   └── l3_app/            # L3 软件装配：全局控制台
│   │       ├── thread_pool/   # 【核心管控】多线程独占区：全项目唯一定义后台 Worker 的地方
│   │       ├── app_context/   # 全局运行时上下文
│   │       └── main_entry.cpp # 启动与兜底：向下装配 L2；全局 try-catch 收口
│   │
│   └── support/               # 模块四：【支撑域】(基石：被上层单向依赖，禁止反向 import 上层)
│       ├── core/              # 核心规则区 
│       │   ├── exceptions/    # 全局异常基类：AppBaseException.h (定义错误码、重试策略)
│       │   ├── constants.h    # 全局常量、协议键名
│       │   └── shared_state.h # 极少量的、严格受控的全局状态
│       │
│       ├── infra/             # 基础设施区 (第三方库的“防腐层”)
│       │   ├── logger/        # 封装日志库 (业务层只能用这里的 AppLogger::info)
│       │   ├── storage/       # 封装本地缓存/数据库
│       │   └── ipc_network/   # 封装多进程/网络底层通信
│       └── utils/             # 纯函数工具箱 (时间、字符串、校验等)