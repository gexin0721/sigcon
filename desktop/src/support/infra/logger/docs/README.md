# Logger 模块文档

## 1. 模块目标（做什么）

`logger` 模块用于为业务程序提供统一、线程安全、可按等级过滤的本地日志能力，支持两种编译模式：

1. 测试模式（定义 `LOG_TEST_MODE`）：可构造多个实例，适合自治测试与示例验证。
2. 运行模式（不定义 `LOG_TEST_MODE`）：全局单例，适合正式业务进程按日期统一落盘。

## 2. 模块结构（怎么做）

目录对应关系（遵循仓库约定）：

1. `src`：实现代码。
2. `include`：对外头文件。
3. `test`：自测试程序与测试构建脚本。
4. `example`：最小可运行示例。
5. `docs`：使用说明文档。

当前关键文件：

1. `../include/logger.h`：接口声明与模式切换宏说明。
2. `../src/logger.cpp`：日志写入、等级过滤、文件管理与时间工具实现。
3. `../test/logger_self_test.cpp`：无第三方框架的自治测试程序。
4. `../example/test_mode_example.cpp`：测试模式示例。
5. `../example/runtime_mode_example.cpp`：运行模式示例。

## 3. 设计原因（为什么）

采用双模式设计的原因：

1. 测试/示例阶段需要快速构造多个 logger 实例，便于隔离不同模块日志。
2. 运行阶段需要全局唯一 logger，避免重复初始化与日志落盘分散。
3. 通过编译宏切换可以复用同一套接口与实现，降低维护成本。

## 4. 接口概览

公共接口（两种模式都支持）：

1. `setLevel(LogLevel level)`：设置最低输出等级。
2. `debug/info/warn/error`：按级别写日志。
3. `fatal`：写日志后调用 `abort()` 终止进程。

测试模式专用接口（定义 `LOG_TEST_MODE`）：

1. `Logger(prefix, id, fileLocation)`：可直接构造实例。
2. 文件命名格式：`<prefix>_<YYYYMMDD_HHmmss>.log`。

运行模式专用接口（不定义 `LOG_TEST_MODE`）：

1. `Logger::init(prefix, id, fileLocation)`：程序入口初始化一次。
2. `Logger::instance()`：任意模块获取单例引用。
3. 文件命名格式：`<prefix>_<YYYYMMDD>.log`，支持按天滚动。

## 5. 日志格式

单行日志格式如下：

```text
【YYYY-MM-DD HH:MM:SS.mmm】【业务ID】【来源描述】【级别】 消息体
```

示例：

```text
【2026-04-25 16:10:22.135】【PROCESS-MAIN】【example/runtime_mode_example.cpp】【INFO 】 runtime example exit
```

## 6. 编译与运行

### 6.1 运行示例（example）

在 `desktop/src/support/infra/logger/example` 目录执行：

```bash
cmake -S . -B build
cmake --build build
```

运行运行模式示例：

```bash
./build/runtime_mode_example
```

运行测试模式示例：

```bash
./build/test_mode_example
```

### 6.2 执行自治测试（test）

在 `desktop/src/support/infra/logger/test` 目录执行：

```bash
cmake -S . -B build
cmake --build build
./build/logger_self_test
```

返回码约定：

1. `0`：全部测试通过。
2. `1`：存在失败项。
3. `2`：编译模式错误（未启用 `LOG_TEST_MODE`）。

## 7. 自测覆盖点

`logger_self_test` 当前覆盖以下能力：

1. 等级过滤正确性：`WARN` 门限下仅 `WARN/ERROR` 可写入。
2. 并发写入完整性：多线程写入后，日志条数与消息集合完整。

## 8. 常见问题

1. 问：为什么不能在 `logger.h` 里直接写死 `#define LOG_TEST_MODE`？  
答：这样会导致所有目标都只能走测试模式，运行模式接口（`init/instance`）无法正常使用。应由构建系统按目标显式传入宏。

2. 问：`fatal` 什么时候用？  
答：仅在不可恢复错误使用；它会调用 `abort()`，程序会立即终止。

3. 问：日志文件生成在哪里？  
答：生成在程序运行时当前工作目录（`cwd`）中。
