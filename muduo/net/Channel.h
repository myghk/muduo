// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : noncopyable
{
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  //处理网络事件
  void handleEvent(Timestamp receiveTime);
  //设置四个回调函数,read,write,close,error Callback函数,在处理event时被调用
  void setReadCallback(ReadEventCallback cb)
  { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb)
  { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb)
  { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb)
  { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; } //channel所负责IO事件的那个fd
  int events() const { return events_; } //返回当前channel所注册的网络事件
  void set_revents(int revt) { revents_ = revt; } // used by pollers //设置网络事件
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; } //判断当前channel是否注册了事件

  //在m_event上注册读/写事件
  void enableReading() { events_ |= kReadEvent; update(); } 
  void disableReading() { events_ &= ~kReadEvent; update(); }
   //在m_event上取消读/写事件
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  //取消m_event所有事件
  void disableAll() { events_ = kNoneEvent; update(); }

  //判断m_event是否注册了读/写事件
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller, 当前channel在poller::m_pollfds中的位置
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  string reventsToString() const;
  string eventsToString() const;

  //是否打印日志
  void doNotLogHup() { logHup_ = false; }

  //返回当前channel所在的那个eventloop
  EventLoop* ownerLoop() { return loop_; }
  void remove();  //让eventloop移除自身这个channel

 private:
  static string eventsToString(int fd, int ev);
  //让本channel 所属于的那个eventloop回调channel::update()完成channel的更新
  //实际上最终在poller中被更新
  void update();
  //在handleEvent()内部使用的具体的实现
  void handleEventWithGuard(Timestamp receiveTime);

  //这三个静态常量分别表示:无网络事件,读网络事件,写网络事件
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_; //channel所属的那个eventloop
  const int  fd_;  //每个channel负责处理一个sockfd上的网络事件
  int        events_; //channel注册(要监听)的网络事件
  int        revents_; // it's the received event types of epoll or poll //poll()返回的网络事件,具体发生的事件
  int        index_; // used by Poller. //这个channel在poller中m_pollfds中的序号,默认-1表示不在其中
  bool       logHup_; //是否打印日志

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_; //是否正在处理网络事件
  bool addedToLoop_;  //是否被添加到eventloop中执行
  //当发生了读/写/错误网络事件时,下面几个函数会被调用
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H
