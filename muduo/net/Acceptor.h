// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>

#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"

namespace muduo
{
namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;
  //构造器负责创建一个TCP服务器,创建套接字->绑定->监听,三个步骤
  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  //析构函数,负责关闭套接字,取消m_acceptChannel上所有的网络事件,表示退出和不再接受处理任务网络事件.
  ~Acceptor();

  //设置接受连接时的回调函数,在成功接收一个客户机的连接时调用 m_newConnectionCallback
  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  //监听操作
  void listen();
  //是否正在监听
  bool listening() const { return listening_; }

  // Deprecated, use the correct spelling one above.
  // Leave the wrong spelling here in case one needs to grep it for error messages.
  // bool listenning() const { return listening(); }

 private:
  //处理服务器accept
  void handleRead();

  EventLoop* loop_; //acceptor所属于的那个eventloop
  Socket acceptSocket_; //接受连接的Socket,即server socket
  Channel acceptChannel_; //m_acceptSocket对应的的channel
  NewConnectionCallback newConnectionCallback_; //新连接建立时所调用的回调函数
  bool listening_; //是否正在监听
  int idleFd_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ACCEPTOR_H
