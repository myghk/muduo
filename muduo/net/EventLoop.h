// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : noncopyable
{
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  //eventloop的核心函数,用于不断循环,在其中调用poller::poll()用于获取发生的网络事件
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  //停止loop()
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  //poller::poll返回时,得到其返回的事件
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.

  //timerqueue相关,讲Functor添加到任务队列m_pendingFunctors中准备执行
  void runInLoop(Functor cb); 
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId runAt(Timestamp time, TimerCallback cb);  //在XXX时间戳执行cb
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  TimerId runAfter(double delay, TimerCallback cb); //在XXX秒之后执行cb
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  TimerId runEvery(double interval, TimerCallback cb); //每过XXX秒执行cb
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void cancel(TimerId timerId); //取消定时器timerid

  // internal usage
  void wakeup(); //用于在m_wakeupFd发起一个读事件,让poller::poll()立即返回,让其处理用户任务
  void updateChannel(Channel* channel); //更新channel,实际上调用了poller::updatechannel,更新poller的m_pollfds数组
  void removeChannel(Channel* channel); //实质让poller删除m_pollfds中的channel相关套接字
  bool hasChannel(Channel* channel); //判断是否有该channel

  // pid_t threadId() const { return threadId_; }
  //断言,eventloop所属于的线程ID就是当前线程的ID
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  //判断是否eventloop所属于的线程ID就是当先线程的ID
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }

  bool eventHandling() const { return eventHandling_; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  //获得当前线程的那个eventloop*
  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up //定时器相关的,一旦m_wakeupFd发起读网络事件,就执行这个handleRead
  void doPendingFunctors(); //处理用户任务队列

  //打印当前活动的所有channel
  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */ //eventloop是否正在loop
  std::atomic<bool> quit_; //是否退出
  bool eventHandling_; /* atomic */ //是否正在处理网络事件
  bool callingPendingFunctors_; /* atomic */ //是否正在执行任务队列中的任务
  int64_t iteration_; //迭代次数
  const pid_t threadId_; //eventloop所在的那个线程ID,要求one eventloop one thread
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_; //用于在loop()中调用poller::poll()
  std::unique_ptr<TimerQueue> timerQueue_; //定时器队列
  int wakeupFd_; //用于唤醒poller::poll()的FD
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeupChannel_; //m_wakeupFD对应的channel
  boost::any context_;

  // scratch variables //在poller::poll()中返回的activeChannels,也就是每个网络事件对应的channel
  ChannelList activeChannels_; //当前活动的channel,用于handleEvent
  Channel* currentActiveChannel_; //当前处理到的那个channel

  mutable MutexLock mutex_; //任务队列的锁
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_); //用户任务队列
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
