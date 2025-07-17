#pragma once
#include "QCL_Include.hpp"

namespace QCL
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * @class TcpServer
     * @brief 简单的多线程TCP服务器类，支持多个客户端连接，数据收发及断开处理
     *
     * 该类使用一个线程专门用于监听客户端连接，
     * 每当有客户端连接成功时，为其创建一个独立线程处理该客户端的数据收发。
     * 线程安全地管理所有客户端Socket句柄。
     */
    class TcpServer
    {
    public:
        /**
         * @brief 构造函数，指定监听端口
         * @param port 服务器监听端口号
         */
        TcpServer(int port);

        /**
         * @brief 析构函数，自动调用 stop() 停止服务器并清理资源
         */
        ~TcpServer();

        /**
         * @brief 启动服务器，创建监听socket，开启监听线程
         * @return 启动成功返回true，失败返回false
         */
        bool start();

        /**
         * @brief 停止服务器，关闭所有连接，释放资源，等待所有线程退出
         */
        void stop();

        /**
         * @brief 发送消息给指定客户端
         * @param clientSock 客户端Socket描述符
         * @param message 发送的字符串消息
         */
        void sendToClient(int clientSock, const std::string &message);

        /**
         * @brief 从指定客户端接收数据（单次调用）
         * @param clientSock 客户端Socket描述符
         */
        char *receiveFromClient(int clientSock);

        /**
         * @brief 获取当前所有已连接客户端Socket的副本
         * @return 包含所有客户端Socket的vector，线程安全
         */
        std::vector<int> getClientSockets();

    private:
        /**
         * @brief 监听并接受新的客户端连接（运行在独立线程中）
         */
        void acceptClients();

    private:
        int serverSock_;                         ///< 服务器监听Socket描述符
        int port_;                               ///< 服务器监听端口
        std::atomic<bool> running_;              ///< 服务器运行状态标志（线程安全）
        std::vector<std::thread> clientThreads_; ///< 用于处理每个客户端的线程集合
        std::thread acceptThread_;               ///< 负责监听新连接的线程
        std::mutex clientsMutex_;                ///< 保护clientSockets_的互斥锁
        std::vector<int> clientSockets_;         ///< 当前所有连接的客户端Socket集合
    };
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
