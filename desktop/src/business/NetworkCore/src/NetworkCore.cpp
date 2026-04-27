#include "WebSocketServerModule.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketMessageType.h>

#include <algorithm>
#include <utility>
#include <sstream>
namespace networkcore
{
    // ===========================================================================
    // 构造函数
    // ===========================================================================
    WebSocketServerModule::WebSocketServerModule(int port,
                                                OnMessageCallback    onMessage,
                                                OnConnectCallback    onConnect,
                                                OnDisconnectCallback onDisconnect,
                                                OnErrorCallback      onError)
        : m_port(port)
        , m_running(false)
        , m_nextClientId(1)                  // 客户端编号从 1 开始，0 留给广播
        , m_onMessage(std::move(onMessage))
        , m_onConnect(std::move(onConnect))
        , m_onDisconnect(std::move(onDisconnect))
        , m_onError(std::move(onError))
    {
        // Windows 下 IXWebSocket 需要先初始化 WSA(网络子系统)
        // Linux/macOS 下这个调用是空操作，加上没坏处。
        ix::initNetSystem();

        // 创建库提供的服务器对象，构造时只是登记一下端口，不会真的开始监听
        m_server = std::make_unique<ix::WebSocketServer>(m_port);

        // 关闭 SO_LINGER（让 stop 时连接能干净关闭，不残留 TIME_WAIT 一堆）
        // 这个根据需要可调；保持默认通常也没问题。
        // m_server->disablePerMessageDeflate();   // 如果担心压缩开销可以打开
    }

    // ===========================================================================
    // 析构函数
    // ===========================================================================
    WebSocketServerModule::~WebSocketServerModule()
    {
        // 保证退出时干净：通知服务器停掉
        stop();

        // 与 initNetSystem 配对
        ix::uninitNetSystem();
    }

    // ===========================================================================
    // run()  -- 阻塞式启动入口
    // ===========================================================================
    bool WebSocketServerModule::run()
    {
        // 防止重复启动：用原子的 exchange，只有从 false 变 true 才继续
        bool expected = false;
        if (!m_running.compare_exchange_strong(expected, true)) {
            // 说明已经在运行中
            if (m_onError) m_onError(0, "Server is already running.");
            return false;
        }

        // -----------------------------------------------------------------------
        // 第 1 步：注册「新连接到来」的回调
        // -----------------------------------------------------------------------
        // setOnConnectionCallback 一次连接只会调用一次，
        // 我们要在里面给这个连接挂上"收到消息"的子回调。
        m_server->setOnConnectionCallback(
            [this](std::weak_ptr<ix::WebSocket> weakWs,
                std::shared_ptr<ix::ConnectionState> connState) {
                this->onNewConnection(weakWs, connState);
            });

        // -----------------------------------------------------------------------
        // 第 2 步：调用 listen()，绑定端口、开始监听
        // -----------------------------------------------------------------------
        // listen() 返回一个 pair<bool, string>：第一个是成功与否，第二个是错误描述
        auto result = m_server->listen();
        if (!result.first) {
            if (m_onError) {
                m_onError(0, std::string("listen() failed: ") + result.second);
            }
            m_running = false;
            return false;
        }

        // -----------------------------------------------------------------------
        // 第 3 步：start()
        // -----------------------------------------------------------------------
        // 注意：start() 内部会让库自己起 IO 线程处理收发。
        // 这是库的实现细节，我们这层代码确实没有 std::thread，
        // 仍然符合「子模块不显式创建线程」的约束。
        m_server->start();

        // -----------------------------------------------------------------------
        // 第 4 步：wait()
        // -----------------------------------------------------------------------
        // 这里就是「阻塞」的关键。wait() 会一直卡住当前线程，
        // 直到 server 被 stop() 通知退出。
        m_server->wait();

        // 走到这里说明服务器已经停了
        m_running = false;
        return true;
    }

    // ===========================================================================
    // stop()  -- 通知服务器停止（线程安全）
    // ===========================================================================
    void WebSocketServerModule::stop()
    {
        if (!m_running.load()) return;   // 没在跑，不用动

        if (m_server) {
            // stop() 会让里面 wait() 立刻返回
            m_server->stop();
        }
    }

    // ===========================================================================
    // 新连接处理
    // ===========================================================================
    void WebSocketServerModule::onNewConnection(
            std::weak_ptr<ix::WebSocket> weakWs,
            std::shared_ptr<ix::ConnectionState> connState)
    {
        // weak_ptr 必须 lock() 成 shared_ptr 才能用，防止对端瞬连瞬断时野指针
        auto ws = weakWs.lock();
        if (!ws) return;

        // 1) 分配编号（原子自增，多线程安全）
        int clientId = m_nextClientId.fetch_add(1);

        // 2) 取对端地址
        std::string remoteAddr;
        {
            std::ostringstream oss;
            oss << connState->getRemoteIp() << ":" << connState->getRemotePort();
            remoteAddr = oss.str();
        }

        // 3) 把它加入在线表
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            ClientInfo info;
            info.id          = clientId;
            info.ws          = ws;
            info.remoteAddr  = remoteAddr;
            info.connectTime = std::chrono::system_clock::now();
            m_clients[clientId] = std::move(info);
        }

        // 4) 通知上层「来人了」
        if (m_onConnect) {
            m_onConnect(clientId, remoteAddr);
        }

        // 5) 给这个连接挂上消息回调
        //    这里捕获 clientId 而不是 ws/connState，避免循环引用。
        ws->setOnMessageCallback(
            [this, clientId, remoteAddr](const ix::WebSocketMessagePtr& msg) {

                switch (msg->type) {

                // ========== 收到一帧数据 ==========
                case ix::WebSocketMessageType::Message: {
                    // msg->str 是 std::string，但内容可以是任意字节（含 0x00），
                    // 直接用其 data()/size() 拷成 vector<uint8_t> 给上层。
                    std::vector<uint8_t> data(
                        reinterpret_cast<const uint8_t*>(msg->str.data()),
                        reinterpret_cast<const uint8_t*>(msg->str.data()) + msg->str.size());

                    if (m_onMessage) {
                        m_onMessage(clientId, data, msg->binary);
                    }
                    break;
                }

                // ========== 客户端断开 ==========
                case ix::WebSocketMessageType::Close: {
                    std::ostringstream oss;
                    oss << "code=" << msg->closeInfo.code
                        << ", reason=" << msg->closeInfo.reason
                        << (msg->closeInfo.remote ? " (by remote)" : " (by local)");
                    onClientClosed(clientId, oss.str());
                    break;
                }

                // ========== 出错 ==========
                case ix::WebSocketMessageType::Error: {
                    std::ostringstream oss;
                    oss << "ws error: "         << msg->errorInfo.reason
                        << ", retries="          << msg->errorInfo.retries
                        << ", wait_time(ms)="    << msg->errorInfo.wait_time
                        << ", http_status="      << msg->errorInfo.http_status;
                    if (m_onError) {
                        m_onError(clientId, oss.str());
                    }
                    break;
                }

                // 其它类型(Open/Ping/Pong/Fragment)我们暂不关心，让库自己处理保活
                default:
                    break;
                }
            });
    }

    // ===========================================================================
    // 客户端断开处理
    // ===========================================================================
    void WebSocketServerModule::onClientClosed(int clientId,
                                            const std::string& reason)
    {
        std::string remoteAddr;

        // 1) 从在线表移除，顺便记一下地址
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            auto it = m_clients.find(clientId);
            if (it != m_clients.end()) {
                remoteAddr = it->second.remoteAddr;
                m_clients.erase(it);
            }
        }

        // 2) 写入断开历史（环形保留最近 N 条）
        {
            std::lock_guard<std::mutex> lock(m_historyMutex);
            DisconnectRecord rec;
            rec.clientId   = clientId;
            rec.remoteAddr = remoteAddr;
            rec.reason     = reason;
            rec.time       = std::chrono::system_clock::now();
            m_disconnectHistory.push_back(std::move(rec));
            while (m_disconnectHistory.size() > m_maxHistory) {
                m_disconnectHistory.pop_front();
            }
        }

        // 3) 通知上层
        if (m_onDisconnect) {
            m_onDisconnect(clientId, reason);
        }
    }

    // ===========================================================================
    // 根据 id 找到 WebSocket 指针
    // ===========================================================================
    std::shared_ptr<ix::WebSocket>
    WebSocketServerModule::findClient(int clientId)
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it == m_clients.end()) return nullptr;
        return it->second.ws;
    }

    // ===========================================================================
    // 发送二进制
    // ===========================================================================
    int WebSocketServerModule::sendBinary(int clientId,
                                        const std::vector<uint8_t>& data)
    {
        // IXWebSocket 的发送接口需要 string 或 IXWebSocketSendData，
        // 这里用 string 视图把字节塞进去（string 不强制 utf-8，只是 char 容器）。
        std::string payload(reinterpret_cast<const char*>(data.data()),
                            data.size());

        int sentCount = 0;

        if (clientId == 0) {
            // ---- 广播 ----
            // 拷贝出当前在线列表，避免持锁时间过长 + 防止发送时阻塞别的线程
            std::vector<std::shared_ptr<ix::WebSocket>> snapshot;
            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                snapshot.reserve(m_clients.size());
                for (auto& kv : m_clients) snapshot.push_back(kv.second.ws);
            }

            for (auto& ws : snapshot) {
                if (!ws) continue;
                auto info = ws->sendBinary(payload);
                if (info.success) ++sentCount;
            }
        } else {
            // ---- 单播 ----
            auto ws = findClient(clientId);
            if (ws) {
                auto info = ws->sendBinary(payload);
                if (info.success) ++sentCount;
            } else {
                if (m_onError) {
                    m_onError(clientId,
                            "sendBinary: client id not found, id=" +
                            std::to_string(clientId));
                }
            }
        }
        return sentCount;
    }

    // ===========================================================================
    // 发送文本
    // ===========================================================================
    int WebSocketServerModule::sendText(int clientId, const std::string& text)
    {
        int sentCount = 0;

        if (clientId == 0) {
            std::vector<std::shared_ptr<ix::WebSocket>> snapshot;
            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                snapshot.reserve(m_clients.size());
                for (auto& kv : m_clients) snapshot.push_back(kv.second.ws);
            }
            for (auto& ws : snapshot) {
                if (!ws) continue;
                auto info = ws->sendText(text);
                if (info.success) ++sentCount;
            }
        } else {
            auto ws = findClient(clientId);
            if (ws) {
                auto info = ws->sendText(text);
                if (info.success) ++sentCount;
            } else {
                if (m_onError) {
                    m_onError(clientId,
                            "sendText: client id not found, id=" +
                            std::to_string(clientId));
                }
            }
        }
        return sentCount;
    }

    // ===========================================================================
    // 主动踢人
    // ===========================================================================
    bool WebSocketServerModule::disconnectClient(int clientId)
    {
        auto ws = findClient(clientId);
        if (!ws) return false;

        // close() 会发送 close 帧并最终触发 onMessage(Close) 回调，
        // onClientClosed() 会在那里被调用，把它从 map 中移除。
        ws->close();
        return true;
    }

    // ===========================================================================
    // 查询接口
    // ===========================================================================
    std::vector<int> WebSocketServerModule::getOnlineClients()
    {
        std::vector<int> ids;
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            ids.reserve(m_clients.size());
            for (auto& kv : m_clients) ids.push_back(kv.first);
        }
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    size_t WebSocketServerModule::getOnlineCount()
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        return m_clients.size();
    }

    std::vector<WebSocketServerModule::DisconnectRecord>
    WebSocketServerModule::getDisconnectHistory(size_t maxCount)
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        if (maxCount == 0 || maxCount >= m_disconnectHistory.size()) {
            return std::vector<DisconnectRecord>(m_disconnectHistory.begin(),
                                                m_disconnectHistory.end());
        }
        // 取最后 maxCount 条
        auto begin = m_disconnectHistory.end() - static_cast<long>(maxCount);
        return std::vector<DisconnectRecord>(begin, m_disconnectHistory.end());
    }

    void WebSocketServerModule::clearDisconnectHistory()
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_disconnectHistory.clear();
    }   
}