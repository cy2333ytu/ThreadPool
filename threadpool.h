#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>
#include <thread>
#include <functional>
#include <stdexcept>

namespace ccy {
// The maximum capacity of the thread pool, should be set as small as possible
#define THREADPOOL_MAX_SIZE 16
// Whether the thread pool can grow automatically (if needed and not exceeding THREADPOOL_MAX_SIZE)
//#define THREADPOOL_AUTO_GROW

// Thread pool that can execute variadic functions or lambda expressions, and retrieve the return values
// it supports static member functions or global functions, Opteron() functions, etc.
class ThreadPool {
    unsigned short _initSize;       // Initial number of threads
    using Task = std::function<void()>;
    std::vector<std::thread> _pool; // Thread pool
    std::queue<Task> _tasks;        // Task queue
    std::mutex _lock;               // Task queue synchronization lock
#ifdef THREADPOOL_AUTO_GROW
    std::mutex _lockGrow;           // Thread pool growth synchronization lock
#endif // !THREADPOOL_AUTO_GROW
    std::condition_variable _task_cv; // Conditional blocking
    std::atomic<bool> _run{true};      // Whether the thread pool is running
    std::atomic<int> _idlThrNum{0};   // Number of idle threads

public:
    inline ThreadPool(unsigned short size = 4) { _initSize = size; addThread(size); }
    inline ~ThreadPool() {
        _run = false;
        _task_cv.notify_all(); // Wake up all threads to execute
        for (std::thread& thread : _pool) {
            // thread.detach(); // Let the thread "live or die on its own"
            if (thread.joinable())
                thread.join(); // Wait for tasks to finish, prerequisite: threads must complete execution
        }
    }

public:
    // Submit a task
    // Calling .get() to retrieve the return value will wait for the task to complete and get the return value
    // There are two ways to call member functions,
    // One is to use bind: .commit(std::bind(&Dog::sayHello, &dog));
    // Another is to use mem_fn: .commit(std::mem_fn(&Dog::sayHello), this)
    template <class F, class... Args>
    auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if (!_run)    // Is it stopped?
            throw std::runtime_error("Commit on ThreadPool is stopped.");

        using RetType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<RetType> future = task->get_future();
        {    // Add the task to the queue
            std::lock_guard<std::mutex> lock{ _lock };
            _tasks.emplace([task]() {
                (*task)();
            });
        }
#ifdef THREADPOOL_AUTO_GROW
        if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_SIZE)
            addThread(1);
#endif // !THREADPOOL_AUTO_GROW
        _task_cv.notify_one(); // Wake up one thread to execute

        return future;
    }

    // Submit a task without parameters and no return value
    template <class F>
    void commit2(F&& task) {
        if (!_run) return;
        {
            std::lock_guard<std::mutex> lock{ _lock };
            _tasks.emplace(std::forward<F>(task));
        }
#ifdef THREADPOOL_AUTO_GROW
        if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_SIZE)
            addThread(1);
#endif // !THREADPOOL_AUTO_GROW
        _task_cv.notify_one();
    }

    // Number of idle threads
    int idlCount() { return _idlThrNum; }
    // Number of threads
    int thrCount() { return _pool.size(); }

#ifndef THREADPOOL_AUTO_GROW
private:
#endif // !THREADPOOL_AUTO_GROW
    // Add a specified number of threads
    void addThread(unsigned short size) {
#ifdef THREADPOOL_AUTO_GROW
        if (!_run)    // Is it stopped?
            throw std::runtime_error("Grow on ThreadPool is stopped.");
        std::unique_lock<std::mutex> lockGrow{ _lockGrow }; // Automatic growth lock
#endif // !THREADPOOL_AUTO_GROW
        for (; _pool.size() < THREADPOOL_MAX_SIZE && size > 0; --size) {
            _pool.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock{ _lock };
                        _task_cv.wait(lock, [this] {
                            return !_run || !_tasks.empty();
                        });
                        if (!_run && _tasks.empty())
                            return;
                        _idlThrNum--;
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    task();
#ifdef THREADPOOL_AUTO_GROW
                    if (_idlThrNum > 0 && _pool.size() > _initSize)
                        return;
#endif // !THREADPOOL_AUTO_GROW
                    {
                        std::unique_lock<std::mutex> lock{ _lock };
                        _idlThrNum++;
                    }
                }
            });
            {
                std::unique_lock<std::mutex> lock{ _lock };
                _idlThrNum++;
            }
        }
    }
};

}

#endif
