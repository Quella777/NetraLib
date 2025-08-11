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
         * @param flag:false 非阻塞模式,true 阻塞模式
         */
        char *receiveFromClient(int clientSock, bool flag = true);

        /**
         * @brief 获取连接客户端的IP和端口
         * @param clientSock 客户端Socket描述符
         */
        char *getClientIPAndPort(int clientSock);

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
    /**
     * @brief 文件写入工具类（线程安全）
     *
     * 该类支持多种文件写入方式：
     *  - 覆盖写文本
     *  - 追加写文本
     *  - 按位置覆盖原文写入
     *  - 二进制覆盖写
     *  - 二进制追加写
     *
     * 特点：
     *  - 文件不存在时自动创建
     *  - 支持覆盖和追加两种模式
     *  - 支持二进制模式，适合写入非文本数据
     *  - 内部使用 std::mutex 实现线程安全
     */
    class WriteFile
    {
    public:
        /**
         * @brief 构造函数
         * @param filePath 文件路径
         */
        explicit WriteFile(const std::string &filePath);

        /**
         * @brief 覆盖写文本文件（线程安全）
         * @param content 要写入的文本内容
         * @return true 写入成功
         * @return false 写入失败
         */
        bool overwriteText(const std::string &content);

        /**
         * @brief 追加写文本文件（线程安全）
         * @param content 要写入的文本内容
         * @return true 写入成功
         * @return false 写入失败
         */
        bool appendText(const std::string &content);

        /**
         * @brief 按指定位置写入文本（原文写，线程安全）
         *
         * 不清空文件内容，仅替换指定位置的内容。
         * 若文件不存在，则会自动创建空文件。
         *
         * @param content 要写入的内容
         * @param position 写入位置（默认为文件开头）
         * @return true 写入成功
         * @return false 写入失败
         */
        bool writeOriginal(const std::string &content, std::streampos position = 0);

        /**
         * @brief 覆盖写二进制文件（线程安全）
         * @param data 要写入的二进制数据
         * @return true 写入成功
         * @return false 写入失败
         */
        bool overwriteBinary(const std::vector<char> &data);

        /**
         * @brief 追加写二进制文件（线程安全）
         * @param data 要写入的二进制数据
         * @return true 写入成功
         * @return false 写入失败
         */
        bool appendBinary(const std::vector<char> &data);

        /**
         * @brief 计算第一个指定字节序列前的字节数（包含该字节序列本身）
         *
         * 例如：文件内容是 "ABC***--***XYZ"，模式是 "***--***"
         * 返回值应为 11（"ABC"=3字节 + "***--***"=8字节）。
         *
         * @param pattern 要查找的字节模式（支持多字节）
         * @return long 字节数（包含匹配模式本身），未找到返回 -1
         */
        long countBytesBeforePattern(const std::string &pattern);

    private:
        std::string filePath_;  ///< 文件路径
        std::mutex writeMutex_; ///< 线程锁，保证多线程写入安全

        /**
         * @brief 通用文本写入接口（线程安全）
         * @param content 要写入的内容
         * @param mode 打开模式（追加/覆盖等）
         * @return true 写入成功
         * @return false 写入失败
         */
        bool writeToFile(const std::string &content, std::ios::openmode mode);

        /**
         * @brief 通用二进制写入接口（线程安全）
         * @param data 要写入的二进制数据
         * @param mode 打开模式（追加/覆盖等）
         * @return true 写入成功
         * @return false 写入失败
         */
        bool writeBinary(const std::vector<char> &data, std::ios::openmode mode);
    };

    // 读文件操作
    class ReadFile
    {
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 屏蔽所有信号
    void blockAllSignals();
}
