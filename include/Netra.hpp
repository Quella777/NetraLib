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
        std::string receiveFromClient(int clientSock, bool flag = true);

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
         * @brief 计算第一个指定字节序列前的字节数
         * @param pattern 要查找的字节序列
         * @param includePattern true 表示返回值包含 pattern 自身长度，false 表示不包含
         * @return size_t 字节数，如果文件没打开或 pattern 为空则返回 0
         */
        size_t countBytesPattern(const std::string &pattern, bool includePattern = false);

        /**
         * @brief 在文件中查找指定字节序列并在其后写入内容，如果不存在则追加到文件末尾
         * @param pattern 要查找的字节序列
         * @param content 要写入的内容
         * @return true 写入成功，false 文件打开失败
         *
         * 功能说明：
         * 1. 若文件中存在 pattern，则删除 pattern 之后的所有内容，并在其后插入 content。
         * 2. 若文件中不存在 pattern，则在文件末尾追加 content，若末尾无换行符则先补充换行。
         */
        bool writeAfterPatternOrAppend(const std::string &pattern, const std::string &content);

        /**
         * @brief 在文件指定位置之后插入内容
         * @param content 要插入的内容
         * @param pos 插入位置（从文件开头算起的字节偏移量）
         * @param length 插入的长度（>= content.size() 时，多余部分用空字节填充；< content.size() 时只截取前 length 个字节）
         * @return true 插入成功，false 文件打开失败或参数不合法
         *
         * 功能说明：
         * 1. 不会覆盖原有数据，而是将 pos 之后的内容整体向后移动 length 个字节。
         * 2. 如果 length > content.size()，则在 content 后补充 '\0'（或空格，可按需求改）。
         * 3. 如果 length < content.size()，则只写入 content 的前 length 个字节。
         * 4. 文件整体大小会增加 length 个字节。
         *
         * 举例：
         * 原始文件内容: "ABCDEFG"
         * insertAfterPos("XY", 2, 3)  // 在索引 2 后插入
         * 结果: "ABX Y\0CDEFG"   (这里 \0 代表补充的空字节)
         */
        bool insertAfterPos(const std::string &content, size_t pos, size_t length);

        /**
         * @brief 在文件指定位置覆盖写入内容
         * @param content 要写入的内容
         * @param pos 覆盖起始位置（从文件开头算起的字节偏移量）
         * @param length 覆盖长度
         * @return true 覆盖成功，false 文件打开失败或 pos 越界
         *
         * 功能说明：
         * 1. 从 pos 开始覆盖 length 个字节，不会移动或增加文件大小。
         * 2. 如果 content.size() >= length，则只写入前 length 个字节。
         * 3. 如果 content.size() < length，则写入 content，并用 '\0' 补齐至 length。
         * 4. 如果 pos + length 超过文件末尾，则只覆盖到文件尾部，不会越界。
         *
         * 举例：
         * 原始文件内容: "ABCDEFG"
         * overwriteAtPos("XY", 2, 3)
         * 结果: "ABXYEFG"   （原 "CDE" 被 "XY\0" 覆盖，\0 实际不可见）
         */
        bool overwriteAtPos(const std::string &content, size_t pos, size_t length);

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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * @brief ReadFile 类 - 读文件操作工具类
     *
     * 功能：
     *  1. 读取全文（文本 / 二进制）
     *  2. 按行读取
     *  3. 按字节数读取
     *  4. 获取指定字节序列前的字节数（包含该字节序列）
     *  5. 检查文件是否存在
     *  6. 获取文件大小
     *
     * 设计：
     *  - 与 WriteFile 类风格保持一致
     *  - 支持文本文件与二进制文件
     *  - 自动关闭文件（析构时）
     *  - 内部使用 std::mutex 实现线程安全
     */
    class ReadFile
    {
    public:
        /**
         * @brief 构造函数
         * @param filename 文件路径
         */
        explicit ReadFile(const std::string &filename);

        /**
         * @brief 析构函数，自动关闭文件
         */
        ~ReadFile();

        /**
         * @brief 打开文件（以二进制方式）
         * @return true 打开成功
         * @return false 打开失败
         */
        bool Open();

        /**
         * @brief 关闭文件
         */
        void Close();

        /**
         * @brief 文件是否已经打开
         */
        bool IsOpen() const;

        /**
         * @brief 读取全文（文本模式）
         * @return 文件内容字符串
         */
        std::string ReadAllText();

        /**
         * @brief 读取全文（二进制模式）
         * @return 文件内容字节数组
         */
        std::vector<char> ReadAllBinary();

        /**
         * @brief 按行读取文本
         * @return 每行作为一个字符串的 vector
         */
        std::vector<std::string> ReadLines();

        /**
         * @brief 读取指定字节数
         * @param count 要读取的字节数
         * @return 实际读取到的字节数据
         */
        std::vector<char> ReadBytes(size_t count);

        /**
         * @brief 获取指定字节序列前的字节数（包含该字节序列）
         * @param marker 要查找的字节序列（可能不止一个字节）
         * @return 如果找到，返回前面部分字节数；找不到返回0
         */
        size_t GetBytesBefore(const std::string &marker, bool includeMarker = false);

        /**
         * @brief 从指定位置读取指定字节数，默认读取到文件末尾
         * @param pos 起始位置（字节偏移）
         * @param count 要读取的字节数，默认为0表示读取到文件末尾
         * @return 读取到的字节数据
         */
        std::vector<char> ReadBytesFrom(size_t pos, size_t count = 0);

        /**
         * @brief 检查文件是否存在
         */
        bool FileExists() const;

        /**
         * @brief 获取文件大小（字节数）
         */
        size_t GetFileSize() const;

        /**
         * @brief 重置读取位置到文件开头
         */
        void Reset();

    private:
        std::string filename_;   // 文件路径
        std::ifstream file_;     // 文件流对象
        mutable std::mutex mtx_; // 可变，保证 const 方法也能加锁
    };
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // 屏蔽所有信号
    void blockAllSignals();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 字符串操作
    //  去除字符串的左空格
    std::string Ltrim(const std::string &s);

    // 去除字符串右侧的空格
    std::string Rtrim(const std::string &s);

    // 去除字符串左右两侧的空格
    std::string LRtrim(const std::string &s);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // c++进行格式化输出
    //  通用类型转字符串
    template <typename T>
    std::string to_string_any(const T &value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    // 递归获取 tuple 中 index 对应参数
    template <std::size_t I = 0, typename Tuple>
    std::string get_tuple_arg(const Tuple &tup, std::size_t index)
    {
        if constexpr (I < std::tuple_size_v<Tuple>)
        {
            if (I == index)
                return to_string_any(std::get<I>(tup));
            else
                return get_tuple_arg<I + 1>(tup, index);
        }
        else
        {
            throw std::runtime_error("Too few arguments for format string");
        }
    }

    // format 函数
    template <typename... Args>
    std::string format(const std::string &fmt, const Args &...args)
    {
        std::ostringstream oss;
        std::tuple<const Args &...> tup(args...);
        size_t pos = 0;
        size_t arg_idx = 0;

        while (pos < fmt.size())
        {
            if (fmt[pos] == '{' && pos + 1 < fmt.size() && fmt[pos + 1] == '{')
            {
                oss << '{';
                pos += 2;
            }
            else if (fmt[pos] == '}' && pos + 1 < fmt.size() && fmt[pos + 1] == '}')
            {
                oss << '}';
                pos += 2;
            }
            else if (fmt[pos] == '{' && pos + 1 < fmt.size() && fmt[pos + 1] == '}')
            {
                oss << get_tuple_arg(tup, arg_idx++);
                pos += 2;
            }
            else
            {
                oss << fmt[pos++];
            }
        }

        if (arg_idx < sizeof...(Args))
            throw std::runtime_error("Too many arguments for format string");

        return oss.str();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
