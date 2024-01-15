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
#include <unordered_map>

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

    void setMode(PoolMode mode){
        if(checkRunningState()){
            return;
        }
        poolMode_ = mode;
    }
// set task max num
    void setTaskQueMaxThreshHold(int threshhold){
        if(checkRunningState()){
            return;
        }
        taskQueMaxThreshHold_ = threshhold;
    }
// set max thread in cached mode 
    void setThreadSizeThreshHold(int threshhold){
        if(checkRunningState()){
            return;
        }
        if(poolMode_ == PoolMode::MODE_CACHED){
            threadSizeThreshHold_ = threshhold;
        }
    }
    template<typename Func, typename... Args>
    auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>{
        using RTtype = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<RTtype()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<RTtype> res = task->get_future();

        std::unique_lock<std::mutex> lock(taskQueMtx_);

        if(!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool{ return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
        {
            std::cerr << "task queue is full, submit task fail." << std::endl;
            auto task = std::make_shared<std::packaged_task<RTtype()>>(
                []()->RTtype{return RTtype(); });
            (*task)();
            return task->get_future();
        }
        
        taskQue_.emplace([task](){(*task)();});
        taskSize_++;
        notEmpty_.notify_all();
    
// Cached mode, used for quick && small task. based on num of task and idle thread
        if(poolMode_ == PoolMode::MODE_CACHED
        && taskSize_ > idleThreadSize_
        && curThreadSize_ < threadSizeThreshHold_){
            std::cout<< ">>>create new thread..." << std::endl;
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,
            this, std::placeholders::_1));

            int threadId = ptr->getId();
            threads_.emplace(threadId, std::move(ptr));
// start thread
        threads_[threadId]->start();
        curThreadSize_++;
        idleThreadSize_--;
        }
        return res;
    } 
    void start(int initThreadSize = std::thread::hardware_concurrency()){
        isPoolRunning_ = true;
        initThreadSize_ = initThreadSize;
        curThreadSize_ = initThreadSize;

// create threads
    for(int i = 0; i < initThreadSize_; i++){
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,
        std::placeholders::_1));
        
        int threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));
    }
// start threads
    for(int i = 0; i < initThreadSize_; i++){
        threads_[i]->start();
        idleThreadSize_++;
    }
    }
    ThreadPool(const ThreadPool&) = delete;   
    ThreadPool& operator=(const ThreadPool&) = delete;            

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

private:
    bool checkRunningState() const{
        return isPoolRunning_;
    }

    void threadFunc(int threadid){
        auto lastTime = std::chrono::high_resolution_clock().now();

        for(;;){
            Task task;
            {
                std::unique_lock<std::mutex> lock(taskQueMtx_);
                std::cout << "tid: " << std::this_thread::get_id() <<
                " try to get task..." << std::endl;
                while(taskQue_.size() == 0){
                    if(!isPoolRunning_){
                        threads_.erase(threadid);
                        std::cout << "threadid: " << std::this_thread::get_id()
                        << " exit!" << std::endl;

                        exitCond_.notify_all();
                        return;
                    }
                    if(poolMode_ == PoolMode::MODE_CACHED){
                        if(std::cv_status::timeout == 
                        notEmpty_.wait_for(lock, std::chrono::seconds(1))){

                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if(dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_){
                                threads_.erase(threadid);
                                curThreadSize_--;
                                idleThreadSize_--;

                                std::cout << "threadid: " << std::this_thread::get_id() << " exit!" << std::endl;
                                return;
                            }    
                        }
                    }
                else{
                    notEmpty_.wait(lock);
                }
                }
                idleThreadSize_--;
                std::cout << "tid: " <<std::this_thread::get_id() << "get task successfully" << std::endl;

                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;

                if(taskQue_.size() > 0){
                    notEmpty_.notify_all();
                }
                notFull_.notify_all();
            }
            if(task != nullptr){
                task();
            }
            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock().now();

        }
    }

};

#endif
