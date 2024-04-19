#include <iostream>
#include<thread>
#include<future>
#include<chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
//std::async 是一个用于异步执行函数的模板函数，
//它返回一个 std::future 对象，该对象用于获取函数的返回值。

std::string FetchDataFromDB(std::string s) 
{
    std::this_thread::sleep_for(std::chrono::microseconds(5));
    return "Data:" + s;
}

void use_async() 
{
    std::future<std::string> Res = std::async(std::launch::async, FetchDataFromDB, "杨奕涛");
    //async(mode,function,paraameter)
    //launch有两个参数
    //async 异步执行，定于后立马执行
    //deferred 用std::future::get()或std::future::wait()函数时延迟执行，用deferred只会定义，不会执行
    //async||deferred 根据编译器来决定
    std::cout << "main thread is running"<<std::endl;

    std::string dbData = Res.get();//只有async执行完，才可以获得get
    //future的wait和get
    /*
    std::future::get() 和 std::future::wait() 是 C++ 中用于处理异步任务的两个方法，它们的功能和用法有一些重要的区别。
        std::future::get() :
        std::future::get() 是一个阻塞调用，用于获取 std::future 对象表示的值或异常。如果异步任务还没有完成，get() 会阻塞当前线程，直到任务完成。如果任务已经完成，
        get() 会立即返回任务的结果。重要的是，get() 只能调用一次，因为它会移动或消耗掉 std::future 对象的状态。一旦 get() 被调用，std::future 对象就不能再被用来获取结果。

        std::future::wait() :
        std::future::wait() 也是一个阻塞调用，但它与 get() 的主要区别在于 wait() 不会返回任务的结果。它只是等待异步任务完成。
        如果任务已经完成，wait() 会立即返回。如果任务还没有完成，wait() 会阻塞当前线程，直到任务完成。与 get() 不同，wait() 可以被多次调用，它不会消耗掉 std::future 对象的状态。

        总结一下，这两个方法的主要区别在于：

        std::future::get() 用于获取并返回任务的结果，而 std::future::wait() 只是等待任务完成。
        get() 只能调用一次，而 wait() 可以被多次调用。
        如果任务还没有完成，get() 和 wait() 都会阻塞当前线程，但 get() 会一直阻塞直到任务完成并返回结果，而 wait() 只是在等待任务完成。
    */

    /*
        你可以使用std::future的wait_for()或wait_until()方法来检查异步操作是否已完成。
        这些方法返回一个表示操作状态的std::future_status值。
    */
    std::cout << dbData<<std::endl;
}

int my_task() 
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "My task run 1 s " << std::endl;
    return 42;
}

void use_package() 
{
    // 创建一个包装了任务的 std::packaged_task 对象  
    std::packaged_task<int()> task(my_task);
    // 获取与任务关联的 std::future 对象  
    std::future<int> result = task.get_future();
    // 在另一个线程上执行任务  
    std::thread t(std::move(task));
    t.detach(); // 将线程与主线程分离，以便主线程可以等待任务完成  
    //主进程的主线程不会等待子线程返回

    // 等待任务完成并获取结果  
    int value = result.get();
    std::cout << "The result is: " << value << std::endl;
}

//相比于task的优势在于不用等待函数执行完在返回
void set_value(std::promise<int>prom) 
{
    //设置promise的值
    std::this_thread::sleep_for(std::chrono::seconds(1));
    prom.set_value(10);
    std::cout << "promise set sucess " << std::endl;

}

void use_promise() 
{
    //创建一个promise对象
    std::promise<int> prom;
    //获取与proimise相关联的future对象
    std::future<int> fut = prom.get_future();
    //在其他线程去设置prom的值
    std::thread t(set_value, std::move(prom));
    //只支持右值引用
    //在主线程获取future的值
    std::cout << "Waiting for the thread to set the value....\n";
    std::cout << "Value set by the thread: " << fut.get() << std::endl;
    t.join();

}

void set_exception(std::promise<void> prom) {
    try {
        // 抛出一个异常
        throw std::runtime_error("An error occurred!");
    }
    catch (...) {
        // 设置 promise 的异常
        prom.set_exception(std::current_exception());
    }
    //主线程一定要try catch捕获异常，不然会导致主线程崩溃
}
void use_promise_exception()
{
    std::promise<void> prom;
    // 创建一个 promise 对象
    std::future<void> fut = prom.get_future();
    // 获取与 promise 相关联的 future 对象
    std::thread t(set_exception, std::move(prom));
    // 在主线程中获取 future 的异常

    try {
        std::cout << "Waiting for the thread to set the exception...\n";
        fut.get();
    }
    catch (const std::exception& e) {
        std::cout << "Exception set by the thread: " << e.what() << '\n';
    }
    t.join();
}

void use_promise_destruct() {
    std::thread t;
    std::future<int> fut;
    {
        // 创建一个 promise 对象
        std::promise<int> prom;
        // 获取与 promise 相关联的 future 对象
        fut = prom.get_future();
        // 在新线程中设置 promise 的值
        t = std::thread(set_value, std::move(prom));//prom管理权已经转移给了t，所以不会报错
    }
    // 在主线程中获取 future 的值
    std::cout << "Waiting for the thread to set the value...\n";
    std::cout << "Value set by the thread: " << fut.get() << '\n';
    //在主线程还没有有取得future的值之前，一定要保证promise是存活的
    t.join();
}


//多个线程等待同一个结果
void myFunction(std::promise<int>&& promise) {
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::seconds(1));
    promise.set_value(42); // 设置 promise 的值
}
void threadFunction(std::shared_future<int> future) {
    try {
        int result = future.get();
        std::cout << "Result: " << result << std::endl;
    }
    catch (const std::future_error& e) {
        std::cout << "Future error: " << e.what() << std::endl;
    }
}
void use_shared_future() {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();//会获取std::future，会默认隐式转换
    std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中，设置值
    // 使用 share() 方法获取新的 shared_future 对象  
    std::thread myThread2(threadFunction, future);
    std::thread myThread3(threadFunction, future);
    myThread1.join();
    myThread2.join();
    myThread3.join();
}

//future不支持拷贝构造，只支持移动构造
//shared_future支持拷贝构造
void use_shared_future_error() {//错误的使用方式
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中

    //第一次移动future后就不可以再使用
    std::thread myThread2(threadFunction, std::move(future));
    std::thread myThread3(threadFunction, std::move(future));
    myThread1.join();
    myThread2.join();
    myThread3.join();
}

void may_throw()
{
    // 这里我们抛出一个异常。在实际的程序中，这可能在任何地方发生。
    throw std::runtime_error("Oops, something went wrong!");
}
void use_future_exception() 
{
    std::future<void> result(std::async(std::launch::async, may_throw));
    try
    {
        // 获取结果（如果在获取结果时发生了异常，那么会重新抛出这个异常）
        result.get();
    }
    catch (const std::exception& e)
    {
        // 捕获并打印异常
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}

//构建线程池
#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#endif
//线程池是无序的
//以下几种情况不能使用线程池
//如果要保证有顺序不能用线程池
//如果执行的任务之间互斥性很大，或者强关联
//
class ThreadPool
{

public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    static ThreadPool& instance()
    {
        static ThreadPool ins;
        return ins;;
    }

    using Task = std::packaged_task<void()>;//投递的任务类型
    //定义类型别名


    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {//投递任务
        //decltype自动推导函数返回值类型
        using RetType = decltype(f(args...));
        if (stop_.load())
            return std::future<RetType>;

        auto task = std::make_shared<std::packaged_task<RecType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        //通过std:bind的方式进行绑定，可以把一些函数的参数绑定起来，可以生成一些无参的函数
        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task] {(*task)() });
        }
        cv_lock_.notify_one();
        return ret;
    }
    ~ThreadPool()
    {
        stop();
    }
private:
    ThreadPool(unsigned int num = 5)
        :stop_(false)
    {
        if (num < 1)
        {
            thread_num_ = 1;
        }
        else
        {
            thread_num_ = num;
        }
        start();
    }

    void start() {
        for (int i = 0; i < thread_num_; ++i) {
            pool_.emplace_back([this]() {
                while (!this->stop_.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> cv_mt(cv_mt_);
                        this->cv_lock_.wait(cv_mt, [this] {
                            return this->stop_.load() || !this->tasks_.empty();
                            });
                        if (this->tasks_.empty())
                            return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;//空闲的线程数减一
                    task();
                    this->thread_num_++;
                }
                });
        }
    }

    void stop()
    {
        stop_.store(true);
        cv_lock_.notify_all();
        for (auto& td : pool_)
        {
            if (td.joinable())
            {
                std::cout << "join thread" << td.get_id() << std::endl;
                td.join();
            }
        }
    }
private:
    std::mutex               cv_mt_;
    std::condition_variable  cv_lock_;
    std::atomic_bool         stop_;
    std::atomic_int          thread_num_;
    std::queue<Task>         tasks_;
    std::vector<std::thread> pool_;
};

int main()
{
    //use_async();
    //use_package();
    //use_promise();
    //use_promise_exception();
    use_shared_future();
    //错误方式
    int m = 0;
    ThreadPool::instance().commit([](int& m)
        {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl;
            std::cout << "m address is " << &m << std::endl;
        }, m);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "m is " << m << std::endl;
    std::cout << "m addres is" << &m << std::endl;
    //当我们传递一个左值的时候
    //在模板里会变成左值引用
    //在使用bind的时候会生成一个bind类
    //在内部会通过decay_t转换成右值
    //子线程里使用的都是右值
    //所以不会改变值

    int m = 0;
    //传入的是warpperM &m
    ThreadPool::instance().commit([](int& m)
        {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl;
            std::cout << "m address is " << &m << std::endl;
        }, m);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "m is " << m << std::endl;
    std::cout << "m addres is" << &m << std::endl;
}

