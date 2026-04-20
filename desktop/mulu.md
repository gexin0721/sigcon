Project_Root/
├── assets/                    # 【静态资源域】图片、图标、QSS样式表、多语言翻译包(.ts/.qm)
├── docs/                      # 【文档域】架构说明、API协议、错误码对照表
├── lib/                       # 【第三方库域】(绝对隔离，禁止业务/UI直接引用)
│   ├── spdlog/                # 第三方日志库
│   ├── nlohmann_json/         # 第三方 JSON 库
│   └── ...                    
│
├── src/                       # 【核心源码区】
│   │
│   ├── ui/                    # 模块一：【纯 UI 层】(盲区：不知道业务和调度的存在)
│   │   ├── components/        # 基础公共组件 (如：无业务逻辑的自定义按钮、图表控件)
│   │   ├── panels/            # 局部业务面板 (如：设备状态显示面板，只抛出交互信号)
│   │   └── windows/           # 独立顶层窗口 (如：主窗口、设置弹窗)
│   │
│   ├── business/              # 模块二：【纯业务层】(盲区：不知道 UI 的存在，绝对禁止弹窗)
│   │   ├── device_ctrl/       # 业务域：设备控制
│   │   │   ├── exceptions/    # 🎯 局部异常：如 DeviceException.h (继承自 AppBaseException)
│   │   │   ├── models/        # 数据模型：纯 C++ struct/class
│   │   │   └── DeviceManager.cpp 
│   │   └── algorithm/         # 业务域：核心算法
│   │
│   ├── scheduler/             # 模块三：【调度层】(中枢：系统唯一允许同时 import UI 和 Business 的地方)
│   │   ├── l0_action/         # L0 最小逻辑：单次无状态动作 (如：格式化设备指令动作)
│   │   ├── l1_page/           # L1 功能组装：组装 UI Window + 基础动作 (如：设置页面调度器，接住警告级异常)
│   │   ├── l2_domain/         # L2 业务编排：组装 L1 + Business (如：设备采集全流程调度，负责重试/中断流程)
│   │   └── l3_app/            # L3 软件装配：全局控制台
│   │       ├── thread_pool/   # 🎯 多线程独占区：全项目只有这里允许创建线程/Worker池
│   │       ├── app_context/   # 全局运行时上下文 (如：当前登录用户状态)
│   │       └── main_entry.cpp # 🎯 启动与兜底：向下装配 L2；包含全局 try-catch 捕获 AppBaseException
│   │
│   └── support/               # 【支撑域】(基石：被上层单向依赖，绝对禁止反向 import 上层)
│       ├── core/              # 核心规则区 (全工程通用)
│       │   ├── exceptions/    # 🎯 全局异常基类：AppBaseException.h (定义错误码、严重等级、重试策略)
│       │   ├── constants.h    # 全局常量、协议键名
│       │   ├── error_codes.h  # 全局统一错误码字典
│       │   └── shared_state.h # 极少量的、严格受控的全局状态
│       │
│       ├── infra/             # 基础设施区 (第三方库的“防腐层”)
│       │   ├── logger/        # 封装 spdlog (暴露 AppLogger::info)
│       │   ├── storage/       # 封装本地文件/缓存读写
│       │   ├── config/        # 封装配置文件解析 (ini/json)
│       │   └── network/       # 封装 HTTP/Socket 通信
│       │
│       └── utils/             # 纯函数工具箱 (时间转换、字符串处理、校验工具)