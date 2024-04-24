#include <iostream>
#include<atomic>
#include<thread>
#include<cassert>
#include<vector>
#include<algorithm>
#include<mutex>
#include<memory>

using namespace std;
memory_order;

/*
#else // _HAS_CXX20
enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
};
#endif // _HAS_CXX20
*/

atomic<bool> x, y;
atomic<int> z;

//memory_order_seq_cst:最严格的同步模型，开销也最大
void write_x_then_y() {
    x.store(true, std::memory_order_seq_cst);  // 1
    y.store(true, std::memory_order_seq_cst);  // 2
}

void read_y_then_x() {
    while (!y.load(std::memory_order_seq_cst)) { // 3
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_seq_cst)) { //4
        ++z;
    }
}
void TestOrderSeqCst() {
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5
}

//memory_order_relaxed
//对原子变量的 load 可以使用 memory_order_acquire 内存顺序. 这称为 acquire 操作.

//对原子变量的 store 可以使用 memory_order_release 内存顺序.这称为 release 操作.
void TestReleaseAcquire() {
    std::atomic<bool> rx, ry;
    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed); // 1
        ry.store(true, std::memory_order_release); // 2
        //当编译器检测到relaxed就会强制1在2之前执行
        });
    std::thread t2([&]() {
        //acquire和release会保证2在3的前面
        while (!ry.load(std::memory_order_acquire)); //3
        //3一定在4之前，acquire保证了执行顺序
        assert(rx.load(std::memory_order_relaxed)); //4
        });
    t1.join();
    t2.join();
}

void ReleasAcquireDanger2() {
    std::atomic<int> xd{ 0 }, yd{ 0 };
    std::atomic<int> zd;
    std::thread t1([&]() {
        xd.store(1, std::memory_order_release);  // (1)
        yd.store(1, std::memory_order_release); //  (2)
        });
    std::thread t2([&]() {
        yd.store(2, std::memory_order_release);  // (3)
        });
    std::thread t3([&]() {
        while (!yd.load(std::memory_order_acquire)); //（4）
        assert(xd.load(std::memory_order_acquire) == 1); // (5)
        });

    //2和3都可以构成同步关系

    t1.join();
    t2.join();
    t3.join();
}
void ReleaseSequence() {
    std::vector<int> data;
    std::atomic<int> flag{ 0 };
    std::thread t1([&]() {
        data.push_back(42);  //(1)
        flag.store(1, std::memory_order_release); //(2)操作A
        });
    std::thread t2([&]() {
        int expected = 1;
        while (!flag.compare_exchange_strong(expected, 2, std::memory_order_relaxed)) // (3)
            expected = 1;
        });
    std::thread t3([&]() {
        while (flag.load(std::memory_order_acquire) < 2); // (4)
        assert(data.at(0) == 42); // (5)
        });
    t1.join();
    t2.join();
    t3.join();
    //如果一个 acquire 操作在同一个原子变量上读到了一个 release 操作写入的值, 或者读到了以这个 release 操作为首的 
    //release sequence 写入的值, 那么这个 release 操作 “synchronizes-with” 这个 acquire 操作
}


//memory_order_consume 可以用于 load 操作.使用 memory_order_consume 的 load 称为 consume 操作.
//如果一个 consume 操作在同一个原子变量上读到了一个 release 操作写入的值, 或以其为首的 release sequence 写入的值, 
//则这个 release 操作 “dependency - ordered before” 这个 consume 操作.
void ConsumeDependency() {
std::atomic<std::string*> ptr;
int data;
std::thread t1([&]() {
    std::string* p = new std::string("Hello World"); // (1)
    data = 42; // (2)
    ptr.store(p, std::memory_order_release); // (3)
    });
std::thread t2([&]() {
    std::string* p2;
    while (!(p2 = ptr.load(std::memory_order_consume))); // (4)
    assert(*p2 == "Hello World"); // (5)
    assert(data == 42); // (6)
    });
t1.join();
t2.join();
}
//memory_order_relaxed
//[&]表示捕获所有外部变量
//只关注1操作在2操作前面
void TestOrderRelaxed() {
    std::atomic<bool> rx, ry;
    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed); // 1
        ry.store(true, std::memory_order_relaxed); // 2

        });
    std::thread t2([&]() { 

        while (!ry.load(std::memory_order_relaxed)); //3
        assert(rx.load(std::memory_order_relaxed)); //4
        });
    t1.join();
    t2.join();
}

class SingleAuto 
{
private:
    SingleAuto() {};
    SingleAuto(const SingleAuto& s) = delete;
    SingleAuto operator=(const SingleAuto& s) = delete;
public:
    ~SingleAuto()
    {
        std::cout << "single auto delete success " << std::endl;
    }
    static std::shared_ptr<SingleAuto> GetInst()
    {
        // 1 处
        if (single != nullptr)
        {
            return single;
        }
        // 2 处
        mtx.lock();
        // 3 处
        if (single != nullptr)
        {
            mtx.unlock();
            return single;
        }
        // 4处
        single = std::shared_ptr<SingleAuto>(new SingleAuto);
        //1 为对象allocate一块内存空间
        //2 先将开辟的空间地址返回
        //3 调用construct构造对象
        //有可能返回了一个地址，但并没有构造对象，当其他线程去访问的时候，就会引发崩溃
        mtx.unlock();
        return single;
    }
private:
    static mutex mtx;
    static shared_ptr<SingleAuto> single;

};

shared_ptr<SingleAuto> SingleAuto::single = nullptr;
mutex SingleAuto::mtx;

void TestSingle() 
{
    thread t1([]() {
        cout << "thread t1 singleton address is 0X: " << SingleAuto::GetInst();
        });
    thread t2([](){
    
    });
}

//单例模式无锁实现
class SingleMemoryModel
{
private:
    SingleMemoryModel()
    {
    }
    SingleMemoryModel(const SingleMemoryModel&) = delete;
    SingleMemoryModel& operator=(const SingleMemoryModel&) = delete;
public:
    ~SingleMemoryModel()
    {
        std::cout << "single auto delete success " << std::endl;
    }
    static std::shared_ptr<SingleMemoryModel> GetInst()
    {
        // 1 处
        if (_b_init.load(std::memory_order_acquire))
        {
            return single;
        }
        // 2 处
        s_mutex.lock();
        // 3 处
        if (_b_init.load(std::memory_order_relaxed))
        {
            s_mutex.unlock();
            return single;
        }
        // 4处
        single = std::shared_ptr<SingleMemoryModel>(new SingleMemoryModel);
        _b_init.store(true, std::memory_order_release);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleMemoryModel> single;
    static std::mutex s_mutex;
    static std::atomic<bool> _b_init;
};
std::shared_ptr<SingleMemoryModel> SingleMemoryModel::single = nullptr;
std::mutex SingleMemoryModel::s_mutex;
std::atomic<bool> SingleMemoryModel::_b_init = false;
int main()
{
    //TestOrderSeqCst();
    //TestOrderRelaxed();
    TestReleaseAcquire();
    ReleasAcquireDanger2();
    ReleaseSequence();
    ConsumeDependency();
    TestOrderRelaxed();
}

