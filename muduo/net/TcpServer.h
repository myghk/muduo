// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "muduo/base/Atomic.h"
#include "muduo/base/Types.h"
#include "muduo/net/TcpConnection.h"

#include <map>

namespace muduo
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;
  //端口复用/地址复用
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };

  //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

  //获得ipPort,name,eventloop*
  const string& ipPort() const { return ipPort_; }
  const string& name() const { return name_; }
  EventLoop* getLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.

  //设置线程池内线程数量,设置线程回调函数
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback& cb)
  { threadInitCallback_ = cb; }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool()
  { return threadPool_; }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.

   //tcpserver启动...
  void start();

  /// Set connection callback.
  /// Not thread safe.

   //设置连接建立时,读消息时,写完成时的回调函数
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  //新建一个连接,accpetor默认的接受连接时的回调函数
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
   //移除一个连接
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  //loop内部移除一个连接
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  //每个tcpconnection都有一个名字
  typedef std::map<string, TcpConnectionPtr> ConnectionMap;

  //tcpserver所在的那个eventloop
  EventLoop* loop_;  // the acceptor loop 
  const string ipPort_; //tcpserver的ipPort
  const string name_; //name
   //acceptor智能指针,用于创建套接字,绑定,监听和接收一个连接,
  std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> threadPool_; //eventloop线程池智能指针
  ConnectionCallback connectionCallback_; //建立连接时的回调函数
  MessageCallback messageCallback_; //读消息时的回调函数
  WriteCompleteCallback writeCompleteCallback_; //写事件完成时的回调函数
  ThreadInitCallback threadInitCallback_;  //线程初始化回调
  AtomicInt32 started_; //tcpserver是否开始
  // always in loop thread
  int nextConnId_;  //先一个连接client的fd
  ConnectionMap connections_; //名字到tcpconenction的映射
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_TCPSERVER_H

/*
TcpServer实现了对于TCP服务的封装,功能是管理accept获得的TcpConnection
TcpServer是供用户直接使用的,生命期由用户控制,用户只需要设置好callback函数,再调用start即可

tcpserver维护当前所有的tcpconenction集合,便于对其进行管理
tcpserver构造函数中完成了对于acceptor类对象的构造,因此socket(),bind()操作在
tcpserver构造时就已经做好,而listen()则在tcpserver::start()中做好


构造函数中acceptor设置发生连接事件的回调函数就是tcpserver::newConnection,也就是说
acceptor中的acceptorsocket发生了可读网络事件(连接到来),首先acceptorcahnnel会调用
acceptor::handleRead()先accept()这个连接,此时连接已经接收完成.
但是还需要调用连接建立完成时的回调函数tcpserver::newConnection,在newConnection中
则是完成新建一个tcpconnection对象,并把它加入tcpconnection集合中来方便对所有的
tcpconnection连接进行管理.

需要注意一点就是,tcpserver并不是单线程的,其内部使用一个eventloopthreadpool
也就是说有多个IO线程,每个IO线程都有一个eventloop对象,因此也就有多个
while(1)
{
poll();
handleEvent();
}

这样的好处就是提高并发性,多个连接到来时,单eventloop可能会来不及处理.
这样子会带来一个问题,怎么统计当前所有连接进来的客户机呢?因此是多线程处理IO,
每个线程都有一个poller::m_pollfds,只对该线程的套接字集合进行管理,而tcpserver
如何知道哪些套接字正处于链接呢?
tcpserver使用tcpconenction来维护一个tcp连接集合,每次acceptor接受一个新的连接时,会
回调tcpserver::newConnection()新建一个tcpconnection加入到tcp连接集合,
每次断开连接时,会回调tcpserver::removeConnection()把退出的tcpconnection从tcp
连接集合中删除.


*/
