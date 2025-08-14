#include "Netra.hpp"

namespace QCL
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
     */
    std::string TcpServer::receiveFromClient(int clientSock, bool flag)
    {
        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));

        int flags = flag ? 0 : MSG_DONTWAIT;
        ssize_t bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, flags);

        if (bytesReceived <= 0)
            return {};

        return std::string(buffer, bytesReceived);
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

    /**
     * @brief 获取连接客户端的IP和端口
     * @param clientSock 客户端Socket描述符
     */
    char *TcpServer::getClientIPAndPort(int clientSock)
    {
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);

        // 获取客户端地址信息
        if (getpeername(clientSock, (struct sockaddr *)&addr, &addr_size) == -1)
        {
            perror("getpeername failed");
            return NULL;
        }

        // 分配内存存储结果(格式: "IP:PORT")
        char *result = (char *)malloc(INET_ADDRSTRLEN + 10);
        if (!result)
            return NULL;

        // 转换IP和端口
        char *ip = inet_ntoa(addr.sin_addr);
        unsigned short port = ntohs(addr.sin_port);

        snprintf(result, INET_ADDRSTRLEN + 10, "%s:%d", ip, port);
        return result;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    WriteFile::WriteFile(const std::string &filePath)
        : filePath_(filePath) {}

    /**
     * @brief 覆盖写文本（线程安全）
     */
    bool WriteFile::overwriteText(const std::string &content)
    {
        std::lock_guard<std::mutex> lock(writeMutex_); // 加锁
        return writeToFile(content, std::ios::out | std::ios::trunc);
    }

    /**
     * @brief 追加写文本（线程安全）
     */
    bool WriteFile::appendText(const std::string &content)
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        return writeToFile(content, std::ios::out | std::ios::app);
    }

    /**
     * @brief 按位置写入（线程安全）
     */
    bool WriteFile::writeOriginal(const std::string &content, std::streampos position)
    {
        std::lock_guard<std::mutex> lock(writeMutex_);

        std::ofstream file(filePath_, std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            // 文件不存在则创建
            file.open(filePath_, std::ios::out);
            file.close();
            file.open(filePath_, std::ios::in | std::ios::out);
        }
        if (!file.is_open())
            return false;

        file.seekp(position);
        file << content;
        file.close();
        return true;
    }

    /**
     * @brief 覆盖写二进制（线程安全）
     */
    bool WriteFile::overwriteBinary(const std::vector<char> &data)
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        return writeBinary(data, std::ios::out | std::ios::trunc | std::ios::binary);
    }

    /**
     * @brief 追加写二进制（线程安全）
     */
    bool WriteFile::appendBinary(const std::vector<char> &data)
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        return writeBinary(data, std::ios::out | std::ios::app | std::ios::binary);
    }

    /**
     * @brief 通用文本写入（私有）
     */
    bool WriteFile::writeToFile(const std::string &content, std::ios::openmode mode)
    {
        std::ofstream file(filePath_, mode);
        if (!file.is_open())
            return false;
        file << content;
        file.close();
        return true;
    }

    /**
     * @brief 通用二进制写入（私有）
     */
    bool WriteFile::writeBinary(const std::vector<char> &data, std::ios::openmode mode)
    {
        std::ofstream file(filePath_, mode);
        if (!file.is_open())
            return false;
        file.write(data.data(), data.size());
        file.close();
        return true;
    }

    size_t WriteFile::countBytesBeforePattern(const std::string &pattern, bool includePattern)
    {
        std::lock_guard<std::mutex> lock(writeMutex_);

        if (pattern.empty())
            return 0;

        std::ifstream file(filePath_, std::ios::binary);
        if (!file.is_open())
            return 0;

        file.clear();                 // 清除EOF和错误状态
        file.seekg(0, std::ios::beg); // 回到文件开头

        const size_t chunkSize = 4096;
        std::string buffer;
        buffer.reserve(chunkSize * 2);

        size_t totalRead = 0;
        char chunk[chunkSize];

        while (file.read(chunk, chunkSize) || file.gcount() > 0)
        {
            buffer.append(chunk, file.gcount());
            size_t pos = buffer.find(pattern);
            if (pos != std::string::npos)
            {
                return includePattern ? (pos + pattern.size()) : pos;
            }

            // 保留末尾部分，避免 buffer 无限增长
            if (buffer.size() > pattern.size())
                buffer.erase(0, buffer.size() - pattern.size());

            totalRead += file.gcount();
        }

        return 0; // 没找到 pattern，返回0
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ReadFile::ReadFile(const std::string &filename) : filename_(filename) {}

    ReadFile::~ReadFile()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        Close();
    }

    bool ReadFile::Open()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (file_.is_open())
            file_.close();
        file_.open(filename_, std::ios::in | std::ios::binary);
        return file_.is_open();
    }

    void ReadFile::Close()
    {
        if (file_.is_open())
        {
            std::lock_guard<std::mutex> lock(mtx_);
            file_.close();
        }
    }

    bool ReadFile::IsOpen() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return file_.is_open();
    }

    std::string ReadFile::ReadAllText()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!file_.is_open() && !Open())
            return "";

        std::ostringstream ss;
        ss << file_.rdbuf();
        return ss.str();
    }

    std::vector<char> ReadFile::ReadAllBinary()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!file_.is_open() && !Open())
            return {};

        return ReadBytes(GetFileSize());
    }

    std::vector<std::string> ReadFile::ReadLines()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!file_.is_open() && !Open())
            return {};

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file_, line))
        {
            lines.push_back(line);
        }
        return lines;
    }

    std::vector<char> ReadFile::ReadBytes(size_t count)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!file_.is_open() && !Open())
            return {};

        std::vector<char> buffer(count);
        file_.read(buffer.data(), count);
        buffer.resize(file_.gcount());
        return buffer;
    }

    size_t ReadFile::GetBytesBefore(const std::string &marker, bool includeMarker)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!file_.is_open() && !Open())
            return 0;

        file_.clear();                 // 清除EOF和错误状态
        file_.seekg(0, std::ios::beg); // 回到文件开头

        const size_t chunkSize = 4096;
        std::string buffer;
        buffer.reserve(chunkSize * 2);

        size_t totalRead = 0;
        char chunk[chunkSize];

        while (file_.read(chunk, chunkSize) || file_.gcount() > 0)
        {
            buffer.append(chunk, file_.gcount());
            size_t pos = buffer.find(marker);
            if (pos != std::string::npos)
            {
                // 如果 includeMarker 为 true，返回包含 marker 的长度
                if (includeMarker)
                    return pos + marker.size();
                else
                    return pos;
            }

            // 保留末尾部分，避免 buffer 无限增长
            if (buffer.size() > marker.size())
                buffer.erase(0, buffer.size() - marker.size());

            totalRead += file_.gcount();
        }

        return 0;
    }

    std::vector<char> ReadFile::ReadBytesFrom(size_t pos, size_t count)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!file_.is_open() && !Open())
            return {};

        size_t filesize = GetFileSize();
        if (pos >= filesize)
            return {}; // 起始位置超出文件大小

        file_.clear(); // 清除 EOF 和错误状态
        file_.seekg(pos, std::ios::beg);
        if (!file_)
            return {};

        size_t bytes_to_read = count;
        if (count == 0 || pos + count > filesize)
            bytes_to_read = filesize - pos; // 读取到文件末尾

        std::vector<char> buffer(bytes_to_read);
        file_.read(buffer.data(), bytes_to_read);

        // 实际读取的字节数可能少于请求的数量
        buffer.resize(file_.gcount());

        return buffer;
    }

    bool ReadFile::FileExists() const
    {
        return std::filesystem::exists(filename_);
    }

    size_t ReadFile::GetFileSize() const
    {
        if (!FileExists())
            return 0;
        return std::filesystem::file_size(filename_);
    }

    void ReadFile::Reset()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (file_.is_open())
        {
            file_.clear();
            file_.seekg(0, std::ios::beg);
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // 屏蔽所有信号
    void blockAllSignals()
    {
        // 忽略全部的信号
        for (int ii = 1; ii <= 64; ii++)
            signal(ii, SIG_IGN);
    }

    std::string Ltrim(const std::string &s)
    {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        {
            ++start;
        }
        return s.substr(start);
    }

    std::string Rtrim(const std::string &s)
    {
        if (s.empty())
            return s;

        size_t end = s.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1])))
        {
            --end;
        }
        return s.substr(0, end);
    }

    std::string LRtrim(const std::string &s)
    {
        return Ltrim(Rtrim(s));
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}