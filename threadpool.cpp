#include "threadpool.h"
#include <benchmark/benchmark.h>
#include "threadpool.h"
#include <iostream>

void fun1(int slp) {
    // std::cout << "  hello, fun1 !" << std::this_thread::get_id() << "\n";
    if (slp > 0) {
        // std::cout << " ======= fun1 sleep " << slp << "=========" << std::this_thread::get_id();
        std::this_thread::sleep_for(std::chrono::milliseconds(slp));
    }
}

struct gfun {
    int operator()(int n) {
        // std::cout << n << " hello, gfun ! " << std::this_thread::get_id();
        return 42;
    }
};

class A {    // The function must be static to be used in a thread pool
public:
    static int Afun(int n = 0) {
        // std::cout << n << "  hello, Afun !  " << std::this_thread::get_id() << std::endl;
        return n;
    }

    static std::string Bfun(int n, std::string str, char c) {
        // std::cout << n << "  hello, Bfun !  " << str.c_str() << "  " << (int)c << "  " << std::this_thread::get_id() << std::endl;
        return str;
    }
};

static void BM_ThreadpoolConcurrency(benchmark::State& state) {
    try {
        size_t num_threads = std::thread::hardware_concurrency();
        ccy::ThreadPool executor{ num_threads };
        A a;
        std::future<void> ff = executor.commit(fun1, 0);
        std::future<int> fg = executor.commit(gfun{}, 0);
        std::future<int> gg = executor.commit(a.Afun, 9999);
        std::future<std::string> gh = executor.commit(A::Bfun, 9998, "mult args", 123);
        std::future<std::string> fh = executor.commit([]()->std::string { std::cout << "hello, fh !  " << std::this_thread::get_id() << std::endl; return "hello,fh ret !"; });

        std::cout << " =======  sleep ========= " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(900));

        for (int i = 0; i < num_threads; i++) {
            executor.commit(fun1, i * 100);
        }
        std::cout << " =======  commit all ========= " << std::this_thread::get_id() << " idlsize=" << executor.idlCount() << std::endl;

        std::cout << " =======  sleep ========= " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));

        ff.get(); // Calling .get() to retrieve the return value will wait for the thread to finish and get the return value
        std::cout << fg.get() << "  " << fh.get().c_str() << "  " << std::this_thread::get_id() << std::endl;

        std::cout << " =======  sleep ========= " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << " =======  fun1,55 ========= " << std::this_thread::get_id() << std::endl;
        executor.commit(fun1, 55).get();    // Calling .get() to retrieve the return value will wait for the thread to finish

        std::cout << "end... " << std::this_thread::get_id() << std::endl;

        ccy::ThreadPool pool(4);
        std::vector< std::future<int> > results;

        for (int i = 0; i < 8; ++i) {
            results.emplace_back(
                pool.commit([i] {
                    std::cout << "hello " << i << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout << "world " << i << std::endl;
                    return i * i;
                })
            );
        }
        std::cout << " =======  commit all2 ========= " << std::this_thread::get_id() << std::endl;

        for (auto&& result : results)
            std::cout << result.get() << ' ';
        std::cout << std::endl;
    }
    catch (std::exception& e) {
        std::cout << "some unhappy happened...  " << std::this_thread::get_id() << e.what() << std::endl;
    }
}

// Define the range of the number of threads (e.g., from 1 to 16)
BENCHMARK(BM_ThreadpoolConcurrency)->Range(1, 16);

// Run the benchmark
BENCHMARK_MAIN();
