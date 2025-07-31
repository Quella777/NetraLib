#include "Netra.hpp"

namespace QCL
{
    TcpServer::TcpServer(int port)
        : port_(port), running_(false), serverSock_(-1) {}

    /**
     * @brief 析构函数中调用stop()确保服务器资源被释放
     */
    TcpServer::~TcpServer()
    {
        stop();
    }

    /**
     * @brief 启动服务器：
     * 1. 创建监听socket（TCP）
     * 2. 绑定端口
     * 3. 监听端口
     * 4. 启动监听线程acceptThread_
     *
     * @return 成功返回true，失败返回false
     */
    bool TcpServer::start()
    {
        // 创建socket
        serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSock_ < 0)
        {
            std::cerr << "Socket 创建失败\n";
            return false;
        }

        // 设置socket地址结构
        sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);      // 端口转网络字节序
        serverAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡IP

        // 允许端口重用，防止服务器异常关闭后端口被占用
        int opt = 1;
        setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 绑定端口
        if (bind(serverSock_, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            std::cerr << "绑定失败\n";
            return false;
        }

        // 开始监听，最大等待连接数为5
        if (listen(serverSock_, 5) < 0)
        {
            std::cerr << "监听失败\n";
            return false;
        }

        // 设置运行标志为true
        running_ = true;

        // 启动专门接受客户端连接的线程
        acceptThread_ = std::thread(&TcpServer::acceptClients, this);

        std::cout << "服务器启动，监听端口：" << port_ << std::endl;
        return true;
    }

    /**
     * @brief 停止服务器：
     * 1. 设置运行标志为false，通知线程退出
     * 2. 关闭监听socket
     * 3. 关闭所有客户端socket，清理客户端列表
     * 4. 等待所有线程退出
     */
    void TcpServer::stop()
    {
        running_ = false;

        if (serverSock_ >= 0)
        {
            close(serverSock_);
            serverSock_ = -1;
        }

        {
            // 线程安全关闭所有客户端socket
            std::lock_guard<std::mutex> lock(clientsMutex_);
            for (int sock : clientSockets_)
            {
                close(sock);
            }
            clientSockets_.clear();
        }

        // 等待监听线程退出
        if (acceptThread_.joinable())
            acceptThread_.join();

        // 等待所有客户端处理线程退出
        for (auto &t : clientThreads_)
        {
            if (t.joinable())
                t.join();
        }

        std::cout << "服务器已停止\n";
    }

    /**
     * @brief acceptClients函数循环监听客户端连接请求
     * 每当accept成功：
     * 1. 打印客户端IP和Socket信息
     * 2. 线程安全地将客户端Socket加入clientSockets_列表
     * 3. 创建新线程调用handleClient处理该客户端收发
     */
    void TcpServer::acceptClients()
    {
        while (running_)
        {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSock = accept(serverSock_, (sockaddr *)&clientAddr, &clientLen);
            if (clientSock < 0)
            {
                if (running_)
                    std::cerr << "接受连接失败\n";
                continue;
            }

            // 将客户端IP转换成字符串格式打印
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "客户端连接，IP: " << clientIP << ", Socket: " << clientSock << std::endl;

            {
                // 加锁保护共享的clientSockets_容器
                std::lock_guard<std::mutex> lock(clientsMutex_);
                clientSockets_.push_back(clientSock);
            }
        }
    }

    /**
     * @brief 发送消息给指定客户端
     * @param clientSock 客户端socket
     * @param message 发送消息内容
     */
    void TcpServer::sendToClient(int clientSock, const std::string &message)
    {
        send(clientSock, message.c_str(), message.size(), 0);
    }

    /**
     * @brief 单次接收指定客户端数据
     * @param clientSock 客户端socket
     *
     * 注意：此函数在当前设计中未在线程中使用，仅演示用。
     */
    char *TcpServer::receiveFromClient(int clientSock)
    {
        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0)
        {
            std::cerr << "接收数据失败或客户端断开连接\n";
            return nullptr;
        }

        return strdup(buffer); // 返回动态分配的字符串副本
    }

    /**
     * @brief 获取当前所有客户端Socket副本（线程安全）
     * @return 包含所有客户端socket的vector副本
     */
    std::vector<int> TcpServer::getClientSockets()
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        return clientSockets_;
    }

    // 屏蔽所有信号
    void blockAllSignals()
    {
        // 忽略全部的信号
        for (int ii = 1; ii <= 64; ii++)
            signal(ii, SIG_IGN);
    }
}