#include <iostream>
#include<time.h>
#include<thread>
#include<vector>
#include<numeric>
#include<chrono>
//尽可能开辟线程数要少于cpu的核心数目
//cpp不允许thread执行拷贝构造和拷贝赋值
void some_function() {
        std::cout << "some function is running" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
void some_other_function() {
        std::cout << "some other functino is running" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
void danger_use() {
    //t1 绑定some_function
    std::thread t1(some_function);
    //2 转移t1管理的线程给t2，转移后t1无效
    std::thread t2 = std::move(t1);
    //3 t1 可继续绑定其他线程,执行some_other_function
    t1 = std::thread(some_other_function);//返回一个局部变量，而且是右值
    //4  创建一个线程变量t3
    std::thread t3;
    //5  转移t2管理的线程给t3
    t3 = std::move(t2);
    //6  转移t3管理的线程给t1
    t1 = std::move(t3);//t1已经绑定了一个线程，再重新绑定的话会导致崩溃
    std::this_thread::sleep_for(std::chrono::seconds(2000));
}

std::thread f() {
    return std::thread(some_function);
    //移动构造函数返回
}

class joining_thread {
    std::thread  _t;
public:
    joining_thread() noexcept = default;
    template<typename Callable, typename ...  Args>
    explicit  joining_thread(Callable&& func, Args&& ...args) :
        _t(std::forward<Callable>(func), std::forward<Args>(args)...) {}
    explicit joining_thread(std::thread  t) noexcept : _t(std::move(t)) {}
    joining_thread(joining_thread&& other) noexcept : _t(std::move(other._t)) {}
    joining_thread& operator=(joining_thread&& other) noexcept
    {
        //如果当前线程可汇合，则汇合等待线程完成再赋值
        if (joinable()) {
            join();
        }
        _t = std::move(other._t);
        return *this;
    }
    //joining_thread& operator=(joining_thread other) noexcept
    //{
    //    //如果当前线程可汇合，则汇合等待线程完成再赋值
    //    if (joinable()) {
    //        join();
    //    }
    //    _t = std::move(other._t);
    //    return *this;
    //}
    ~joining_thread() noexcept {
        if (joinable()) {
            join();
        }
    }
    void swap(joining_thread& other) noexcept {
        _t.swap(other._t);
    }
    std::thread::id   get_id() const noexcept {
        return _t.get_id();
    }
    bool joinable() const noexcept {
        return _t.joinable();
    }
    void join() {
        _t.join();
    }
    void detach() {
        _t.detach();
    }
    std::thread& as_thread() noexcept {
        return _t;
    }
    const std::thread& as_thread() const noexcept {
        return _t;
    }
};
void use_jointhread() {
    //1 根据线程构造函数构造joiningthread
    joining_thread j1([](int maxindex) {
        for (int i = 0; i < maxindex; i++) {
            std::cout << "in thread id " << std::this_thread::get_id()
                << " cur index is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }, 10);
    //2 根据thread构造joiningthread
    joining_thread j2(std::thread([](int maxindex) {
        for (int i = 0; i < maxindex; i++) {
            std::cout << "in thread id " << std::this_thread::get_id()
                << " cur index is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }, 10));
    //3 根据thread构造j3
    joining_thread j3(std::thread([](int maxindex) {
        for (int i = 0; i < maxindex; i++) {
            std::cout << "in thread id " << std::this_thread::get_id()
                << " cur index is " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }, 10));
    //4 把j3赋值给j1，joining_thread内部会等待j1汇合结束后
    //再将j3赋值给j1
    j1 = std::move(j3);
}

//std::thread::hardware_concurrenct()函数的返回值是一个指标
// 表示程序在各次运行中可真正并发的线程数量
void param_function(int i) {
    std::cout << "param is" << i;
}
void use_vector() 
{
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 10; i++) {
        threads.emplace_back(param_function, i);

        //auto t = std::thread(param_function, i);
        //threads.push_back(std::move(t));
        //这两段代码等值
    }
    for (auto& entry : threads) {
        entry.join();
    }
}

//利用并行计算求和
template<typename Iterator,typename T>
struct accumulate_block 
{
    //重载 () 实现伪函数 
    void operator()(Iterator first,Iterator end,T& result) 
    {
        result =std::accumulate(first, end, result);
    }
};
template<typename Iterator,typename T>
T parallel_accumulate(Iterator first, Iterator end, T init) {
    unsigned long const length = std::distance(first, end);//计算初始和末尾的距离
    if (!length)
        return init;
    unsigned long const min_per_thread = 25;//定义每次计算的数量
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;//计算出所需的线程数量
    unsigned long const hareware_threads = std::thread::hardware_concurrency();//获得cpu核心数
    unsigned long const num_threads = std::min(hareware_threads != 0 ? hareware_threads : 2, max_threads);//综合计算实际要开辟的线程数
    unsigned long const block_size = length / num_threads;//算出每个线程负责的小区块的数量
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;

    for (unsigned long i = 0; i < (num_threads - 1); i++) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);//移动block_end
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i]));//创建线程
        block_start = block_end;//移动block_start;
    }
    accumulate_block<Iterator, T>()(block_start, end, results[num_threads - 1]);//还有一个主线程

    for (auto& entry : threads) {
        entry.join();
    }
    return std::accumulate(results.begin(), results.end(), init);//通过主线程求和
}

#define max 1000000000
void use_parallel_acc() {
    std::vector<int> vec(max);
    for (long long i = 0; i < max; i++) {
        vec.push_back(1);
    }

    long long sum = 0;
    sum = parallel_accumulate<std::vector<int>::iterator, long> (vec.begin(), vec.end(), sum);
    std::cout << "use_parallel_acc sum is " << sum << std::endl;
}
void accumulate_sum() 
{
    std::vector<int> vec(max);
    for (int i = 0; i < max; i++) {
        vec[i]=1;
    }

    long long sum = 0;
    for (long long i = 0; i < max; i++) {
        sum += vec[i];
    }

    std::cout << "sum is " << sum << std::endl;
}
int main()
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();
    use_parallel_acc();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "use_parallel_acc use time " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<<std::endl;
    start = std::chrono::high_resolution_clock::now();
    accumulate_sum();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "nomoal use time " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
}