// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "muduo/base/Mutex.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Channel.h"

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : noncopyable
{
 public:
 //explicit隐式类型转换,loop为timerqueue所属于的那个eventloop
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  
  //添加定时器到定时器集合中去,供eventloop使用来封装eventloop::runAt(),runAfter()...
  TimerId addTimer(TimerCallback cb,
                   Timestamp when,
                   double interval);

  //取消一个定时器
  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  typedef std::pair<Timestamp, Timer*> Entry; //<时间戳,定时器指针>对
  typedef std::set<Entry> TimerList; //定时器集合
  typedef std::pair<Timer*, int64_t> ActiveTimer; //<定期器指针,int64_t> 活动定时器
  typedef std::set<ActiveTimer> ActiveTimerSet; //活动定时器集合

  //下面这三个函数是用于eventloop的回调使用,用于在eventloop内部管理定时器的添加删除和read
  //在eventloop循环内加入一个定时器
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId); //关闭eventloop内的定时器id
  // called when timerfd alarms
  void handleRead(); //当定时器触发时调用
  // move out all expired timers
  std::vector<Entry> getExpired(Timestamp now); //获取所有已过期的定时器集合
  void reset(const std::vector<Entry>& expired, Timestamp now); //重设已过期的定时器集合

  bool insert(Timer* timer); //插入一个定时器

  EventLoop* loop_; //所属于的那个eventloop
  const int timerfd_; //timerfd,关联channel注册可读事件
  Channel timerfdChannel_; //与timerfd关联,发生可读事件就执行timer::run()
  // Timer list sorted by expiration
  TimerList timers_; //定时器的集合

  // for cancel()
  ActiveTimerSet activeTimers_; //当前活动定时器的集合
  bool callingExpiredTimers_; /* atomic */ //是否正在调用定时器回调
  ActiveTimerSet cancelingTimers_; //是否正在停止定时器
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_TIMERQUEUE_H
