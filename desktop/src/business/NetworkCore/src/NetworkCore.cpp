#include "../include/NetworkCore.h"

namespace networkcore
{
    NetworkServer::NetworkServer(int port, std::function<void(ix::WebSocket&, const ix::WebSocketMessagePtr&)> onClientMessageCallback)
    {
        // 1. 创建服务器，监听 8080 端口
        server(port);

        // 2. 设置"有消息/事件来了"的处理函数
        server.setOnClientMessageCallback(onClientMessageCallback);

        // 3. 开始监听
        auto [ok, errMsg] = server.listen();
        if (!ok) {
            throw std::runtime_error("NetworkServer启动失败: " + errMsg);
        }


    }

    NetworkServer::~NetworkServer()
    {

    }

    void NetworkServer::start()
    {      
        server.start();
    }

    void NetworkServer::stop()
    {
        server.stop();
    }
