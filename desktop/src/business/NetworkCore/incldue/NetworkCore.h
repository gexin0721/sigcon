#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>


namespace networkcore
{
    class NetworkServer
    {
        public:
            /*
             *   @brief 初始化网络服务器
             *   @param port 端口号
             *   @param onClientMessageCallback 客户端消息回调函数
             *   @note 
             */ 
            NetworkServer(int port,std::function<void(ix::WebSocket&, const ix::WebSocketMessagePtr&)> onClientMessageCallback);
            
            /*
                @brief 析构函数
                
            */
            ~NetworkServer();

            /*
                @brief 启动网络服务器
            */
            void start();

            /*
                @brief 停止网络服务器
            */
            void stop();

            bool send(uint8_t data);

        private:
            ix::WebSocketServer server;
            // 端口号
            int Port = 8000;
            // 运行状态
            bool running = 0;
    };
}