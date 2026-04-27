#pragma once

#ifndef WEBSOCKET_SERVER_MODULE_H
#define WEBSOCKET_SERVER_MODULE_H

// ===========================================================================
// IXWebSocket 服务器模块
// ---------------------------------------------------------------------------
// 用途：作为业务层使用的 WebSocket 服务器封装。
// 设计约束：
//   1) 模块本身不创建线程（架构禁止），由顶层调度模块以 run() 阻塞调用。
//   2) 模块不知道 UI 层 / 调度层的存在，所有事件通过构造时注入的回调上报。
//   3) 客户端用 int 编号(从 1 开始)标识，0 号保留为广播专用。
// ===========================================================================

#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXConnectionState.h>

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <deque>

namespace networkcore
{


    class WebSocketServerModule {
    public:
        // =======================================================================
        // 回调函数签名定义
        // -----------------------------------------------------------------------
        // 这些回调会在「IXWebSocket 库内部的 IO 线程」中被调用。
        // 也就是说：上层在回调里如果做耗时操作，会拖慢网络收发。
        // 推荐做法：在回调里只做"投递到业务队列"，让业务线程慢慢消化。
        // =======================================================================

        // ---- 收到一帧数据时的回调 ----
        // clientId : 数据来自哪个客户端（>=1）
        // data     : 原始字节数组（文本帧也会用 vector<uint8_t> 装着，自己转）
        // isBinary : true = 二进制帧，false = 文本帧
        using OnMessageCallback =
            std::function<void(int clientId,
                            const std::vector<uint8_t>& data,
                            bool isBinary)>;

        // ---- 新客户端连接成功时的回调 ----
        // clientId   : 服务器分配的新编号
        // remoteAddr : 客户端地址，格式 "ip:port"，方便日志记录
        using OnConnectCallback =
            std::function<void(int clientId, const std::string& remoteAddr)>;

        // ---- 客户端断开（不论主动/被动）时的回调 ----
        // reason : 断开原因的可读字符串（错误码 + 文本）
        using OnDisconnectCallback =
            std::function<void(int clientId, const std::string& reason)>;

        // ---- 出错时的回调 ----
        // 包括：监听失败、握手失败、收发异常等。
        // clientId == 0 表示是「服务器级别」的错误，不属于某个具体客户端。
        using OnErrorCallback =
            std::function<void(int clientId, const std::string& errorMsg)>;

        // =======================================================================
        // 构造 / 析构
        // =======================================================================

        // 构造函数
        // 参数:
        //   port         : 监听端口
        //   onMessage    : 收到数据时调用（必填，传 nullptr 也允许，但等于丢弃数据）
        //   onConnect    : 新连接时调用（可传 nullptr）
        //   onDisconnect : 断开时调用（可传 nullptr）
        //   onError      : 出错时调用（可传 nullptr，但强烈建议提供，便于排查）
        //
        // 注意：构造函数只做"初始化"，不会真正去监听端口。
        //       真正开始干活在 run() 里。
        WebSocketServerModule(int port,
                            OnMessageCallback     onMessage,
                            OnConnectCallback     onConnect,
                            OnDisconnectCallback  onDisconnect,
                            OnErrorCallback       onError);

        // 析构：会自动调用 stop()，清理所有连接
        ~WebSocketServerModule();

        // 禁止拷贝/赋值（持有网络资源，拷贝没有意义且容易出 bug）
        WebSocketServerModule(const WebSocketServerModule&)            = delete;
        WebSocketServerModule& operator=(const WebSocketServerModule&) = delete;

        // =======================================================================
        // 启动 / 停止
        // =======================================================================

        // 阻塞式启动服务器
        // ---------------------------------------------------------------
        // 调用方应当在「专属业务线程」里调用本函数。
        // 调用后函数不会返回，直到外部调用 stop()。
        //
        // 返回值:
        //   true  - 正常退出（stop() 被调用了）
        //   false - 启动失败（端口被占用、权限不够等）。错误详情会通过 onError 上报。
        bool run();

        // 通知服务器停止
        // ---------------------------------------------------------------
        // 这个函数是线程安全的，可以被任意线程调用。
        // 调用后 run() 会从阻塞中返回。
        void stop();

        // =======================================================================
        // 数据发送接口
        // =======================================================================

        // 发送二进制数据
        // ---------------------------------------------------------------
        // clientId : 目标客户端编号；==0 表示广播给所有在线客户端
        // data     : 要发送的字节数组（uint8_t）
        // 返回值   : 实际成功发送的客户端数量
        //            - 单播时正常是 0 或 1
        //            - 广播时是当前在线数量
        int sendBinary(int clientId, const std::vector<uint8_t>& data);

        // 发送文本（如果以后需要传 JSON 等场景方便些）
        // 同样支持 clientId == 0 广播
        int sendText(int clientId, const std::string& text);

        // =======================================================================
        // 连接管理
        // =======================================================================

        // 主动踢掉某个客户端
        // 返回值: true = 找到并发起断开；false = 该 id 不存在
        // 注意：实际断开是异步的，断开完成后会触发 onDisconnect 回调
        bool disconnectClient(int clientId);

        // 获取当前在线的所有客户端 id（已排序）
        std::vector<int> getOnlineClients();

        // 获取当前在线数量
        size_t getOnlineCount();

        // =======================================================================
        // 健康检查 / 异常记录
        // -----------------------------------------------------------------------
        // 库内部已经有 ping/pong 心跳保活机制，这里我们只「记账」：
        // - 谁什么时候断的、什么原因。
        // - 上层可以随时来查最近 N 条断开记录。
        // 正常情况下完全静默；出问题时通过 onError/onDisconnect 主动上报，
        // 同时本地保留一份历史，方便事后排查。
        // =======================================================================

        // 单条断开记录
        struct DisconnectRecord {
            int          clientId;
            std::string  remoteAddr;
            std::string  reason;
            std::chrono::system_clock::time_point time;
        };

        // 取最近 N 条断开记录（默认全部，最多保留 m_maxHistory 条）
        std::vector<DisconnectRecord> getDisconnectHistory(size_t maxCount = 0);

        // 清空断开记录
        void clearDisconnectHistory();

    private:
        // -----------------------------------------------------------------------
        // 内部数据结构：一个在线客户端的全部信息
        // -----------------------------------------------------------------------
        struct ClientInfo {
            int                              id;          // 我们分配的编号
            std::shared_ptr<ix::WebSocket>   ws;          // 库提供的连接对象
            std::string                      remoteAddr;  // "ip:port"
            std::chrono::system_clock::time_point connectTime;
        };

        // -----------------------------------------------------------------------
        // 私有方法
        // -----------------------------------------------------------------------

        // 处理新客户端进来：分配 id、注册消息回调、加入 map、通知上层
        void onNewConnection(std::weak_ptr<ix::WebSocket> weakWs,
                            std::shared_ptr<ix::ConnectionState> connState);

        // 处理某个客户端断开：从 map 移除、记录历史、通知上层
        void onClientClosed(int clientId,
                            const std::string& reason);

        // 安全地通过 id 拿到客户端的 WebSocket 指针；找不到返回 nullptr
        std::shared_ptr<ix::WebSocket> findClient(int clientId);

        // -----------------------------------------------------------------------
        // 成员变量
        // -----------------------------------------------------------------------

        int                                      m_port;        // 监听端口
        std::unique_ptr<ix::WebSocketServer>     m_server;      // 库的服务器对象
        std::atomic<bool>                        m_running;     // 是否已启动
        std::atomic<int>                         m_nextClientId;// 下一个分配的id

        // 在线客户端表（id → 信息）
        std::unordered_map<int, ClientInfo>      m_clients;
        // 这把锁保护 m_clients，因为库的回调在 IO 线程里跑，
        // 而 sendBinary/disconnect 等接口可能从业务线程进来，存在并发。
        std::mutex                               m_clientsMutex;

        // 用户注入的回调
        OnMessageCallback     m_onMessage;
        OnConnectCallback     m_onConnect;
        OnDisconnectCallback  m_onDisconnect;
        OnErrorCallback       m_onError;

        // 断开历史（最多保留 m_maxHistory 条，旧的自动丢弃）
        std::deque<DisconnectRecord> m_disconnectHistory;
        std::mutex                   m_historyMutex;
        static constexpr size_t      m_maxHistory = 200;
    };

}

#endif // WEBSOCKET_SERVER_MODULE_H
