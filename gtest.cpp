#include "threadpool.h"
#include <gtest/gtest.h>
#include <future>
#include <string>
#include <iostream>

void fun1(int slp) {
    if (slp > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(slp));
    }
}

struct gfun {
    int operator()(int n) {
        return 42 + n;
    }
};

class A {
public:
    static int Afun(int n = 0) {
        return n;
    }

    static std::string Bfun(int n, std::string str, char c) {
        return str + " " + std::to_string(n) + " " + std::to_string(c);
    }
};

class ThreadPoolTest : public ::testing::Test {
protected:
    ccy::ThreadPool* executor;

    void SetUp() override {
        executor = new ccy::ThreadPool(50); 
    }

    void TearDown() override {
        delete executor;  
    }
};

TEST_F(ThreadPoolTest, TestThreadPoolExecution) {
    try {
        ccy::ThreadPool executor{ 50 };

        std::future<void> ff = executor.commit(fun1, 0);
        std::future<int> fg = executor.commit(gfun{}, 0);
        std::future<int> gg = executor.commit(A::Afun, 9999);
        std::future<std::string> gh = executor.commit(A::Bfun, 9998, std::string("mult args"), 123);
        std::future<std::string> fh = executor.commit([]()->std::string { return "hello, fh ret !"; });

        ff.wait(); // Wait for fun1 to complete
        EXPECT_EQ(fg.get(), 42); // Check if gfun returned 42 + input
        EXPECT_EQ(gg.get(), 9999); // Check if Afun returned input
        EXPECT_EQ(gh.get(), std::string("mult args 9998 123")); // Check Bfun concatenation
        EXPECT_EQ(fh.get(), "hello, fh ret !"); // Check lambda function result
    }
    catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}

TEST_F(ThreadPoolTest, TestTaskSubmission) {
    try {
        ccy::ThreadPool pool(4);
        std::vector<std::future<int>> results;

        for (int i = 0; i < 8; ++i) {
            results.emplace_back(
                pool.commit([i] {
                    return i * i;
                })
            );
        }

        for (size_t i = 0; i < results.size(); ++i) {
            EXPECT_EQ(results[i].get(), i * i); // Check if the function returned i squared
        }
    }
    catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}