/**
 * @file qcl_include.hpp
 * @brief 通用C++开发头文件集合（Linux环境）
 * @note 使用前确保目标平台支持C++17标准
 */
#ifndef QCL_INCLUDE_HPP
#define QCL_INCLUDE_HPP

// ==================== C/C++基础运行时库 ====================
#include <iostream> // 标准输入输出流（cin/cout/cerr）
#include <string>   // std::string类及相关操作
#include <cstring>  // C风格字符串操作（strcpy/strcmp等）
#include <cstdlib>  // 通用工具函数（atoi/rand/malloc等）
#include <cstdio>   // C风格IO（printf/scanf）
#include <cassert>  // 断言宏（调试期检查）
#include <cmath>    // 数学函数（sin/pow等）
#include <ctime>    // 时间处理（time/clock）
#include <csignal>  // 信号处理（signal/kill）
#include <memory>   // 智能指针

// ==================== STL容器与算法 ====================
#include <vector>        // 动态数组（连续内存容器）
#include <list>          // 双向链表
#include <deque>         // 双端队列
#include <map>           // 有序键值对（红黑树实现）
#include <set>           // 有序集合
#include <unordered_map> // 哈希表实现的键值对
#include <unordered_set> // 哈希表实现的集合
#include <stack>         // 栈适配器（LIFO）
#include <queue>         // 队列适配器（FIFO）
#include <algorithm>     // 通用算法（sort/find等）
#include <numeric>       // 数值算法（accumulate等）
#include <iterator>      // 迭代器相关

// ==================== 字符串与流处理 ====================
#include <sstream>    // 字符串流（内存IO）
#include <fstream>    // 文件流（文件IO）
#include <iomanip>    // 流格式控制（setw/setprecision）
#include <regex>      // 正则表达式
#include <filesystem> // 文件系统(C++17)

// ==================== 并发编程支持 ====================
#include <thread>             // 线程管理（std::thread）
#include <mutex>              // 互斥锁（mutex/lock_guard）
#include <atomic>             // 原子操作（线程安全变量）
#include <condition_variable> // 条件变量（线程同步）

// ==================== Linux网络编程 ====================
#include <sys/socket.h> // 套接字基础API（socket/bind）
#include <netinet/in.h> // IPV4/IPV6地址结构体
#include <arpa/inet.h>  // 地址转换函数（inet_pton等）
#include <unistd.h>     // POSIX API（close/read/write）

#endif // QCL_INCLUDE_HPP
