#pragma once



namespace networkcore
{
    class NetworkServer
    {
        public:
        // 
            NetworkServer(int port);
            ~NetworkServer();

            void start();
            void stop();

        private:
            
            // 端口号
            int Port = 8000;
            // 运行状态
            bool running = 0;
    };
}