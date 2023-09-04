/* ************************************************************************
> File Name:     threadpool.h
> Author:        Yunzhe Su
> Created Time:  Пн 04 сен 2023 15:15:22
> Description:   
 ************************************************************************/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <future>

const int TASK_MAX_THRESHHOLD = 2; // INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60; // 单位：秒

enum class PoolMode{
    MODE_FIXED,
    MODE_CACHED
};

class Thread{
public:
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func)
    :func_{func}
    ,threadId_{generateId_++}
    {}
    
    ~Thread() = default;

    void start(){
        std::thread t(func_, threadId_);
        t.detach();
    }

    int getId() const{
        return threadId_;
    }

private:
    ThreadFunc func_;
    static int generateId_;
    int threadId_;
};
int Thread::generateId_ = 0;

class ThreadPool{
public:
    ThreadPool()
    : initThreadSize_{0}
    , curThreadSize_{0}
    , idleThreadSize_{0}
    , taskSize_{0}
    , threadSizeThreshHold_{THREAD_MAX_THRESHHOLD}
    , taskQueMaxThreshHold_{TASK_MAX_THRESHHOLD}
    , poolMode_{PoolMode::MODE_FIXED}
    , isPoolRunning_{false}
    {} 

~ThreadPool(){
    isPoolRunning_ = false;
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();
    exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0;});
}
private:
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    
    int initThreadSize_;
    int threadSizeThreshHold_;
    std::atomic_int curThreadSize_;
    std::atomic_int idleThreadSize_;

    using Task = std::function<void()>;
    std::queue<Task> taskQue_;
    std::atomic_int taskSize_;
    int taskQueMaxThreshHold_;

    std::mutex taskQueMtx_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::condition_variable exitCond_;

    PoolMode poolMode_;
    std::atomic_bool isPoolRunning_;
};

#endif