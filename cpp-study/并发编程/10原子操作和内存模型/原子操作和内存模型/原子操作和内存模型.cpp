
#include <iostream>
#include <cstdlib> // For system()
#include<atomic>
#include<thread>
#include<cassert>
#include<vector>
using namespace std;
/*
enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
};

在cpu中不同的处理器会将数据存储到storeBuffer，此时不同逻辑处理器之间数据不互通，之后下放到Catche中
此时两个core可以实现数据互通，最后存到Memory中实现所有core互通数据
StoreBuffer就是一级Cache， Catche是二级Cache，Memory是三级Cache

当cpu1把变量A改为1，存储到StoreBuffer中，cpu2把变量改为3，也存储到StoreBuffer中
两者同时写入Catche就会导致数据混乱
所以要遵循MESI协议

共有六种状态值
最宽松的模型：momery_order_relaxed
1 作用于原子变量
2 不具有synchronizes-with关系
3 对于同一个原子变量，在同一个线程中具有happens-before关系, 
在同一线程中不同的原子变量不具有happens-before关系，可以乱序执行。
4 多线程情况下不具有happens-before关系。


*/


/*
Sequencial consistent ordering. 实现同步, 且保证全局顺序一致 (single total order) 的模型. 
是一致性最强的模型, 也是默认的顺序模型.
Acquire-release ordering. 实现同步, 但不保证保证全局顺序一致的模型.
Relaxed ordering. 不能实现同步, 只保证原子性的模型.
*/
void Test_atomic() 
{
    atomic_flag af;
    if (af.test_and_set())//在这里会检测值为false，然后设置为true
    {
        cout << "true";
    }
    else
    {
        cout << "false";
    }

    if (af.test_and_set())//在这里会为true
    {
        cout << "true";
    }
    else
    {
        cout << "false";
    }
}


//自旋锁demo
class Seqnlock
{
public:
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire));//自旋等待，直到获得锁
        //这里如果能加锁，flag会返回一个false，不会循环
        cout << "线程已加锁" << endl;
    }
    void unlock() 
    {
        flag.clear(std::memory_order_relaxed);//释放锁
        //清零之后线程2就可以加锁
        cout << "线程已解锁" << endl;
    }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;//初始化变量
};

void Test_sql() 
{
    Seqnlock s;
    thread t1([&s]()
        {
            
            for (int i = 0; i < 3; i++) {
                s.lock();
                cout << "线程1正在执行第" << i << endl;
                s.unlock();
            }
        });
    thread t2([&s]()
        {
            for (int i = 0; i < 3; i++) {
                s.lock();
                cout << "线程2正在执行第" << i << endl;
                s.unlock();
            }
        });


    //两个线程会相互的争夺资源，交错运行 
    t1.join();
    t2.join();
}

void funA()
{
    atomic<int> a, b;
    a.store(1000, memory_order_relaxed);
    b.store(100, memory_order_relaxed);
    //order-relaxer不能保证a比b先执行
    //即不遵循happens-before
}

std::atomic<bool> x, y;
std::atomic<int> z;
void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);  // 1
    y.store(true, std::memory_order_relaxed);  // 2
}
void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed)) { // 3
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_relaxed)) { //4
        ++z;
    }
}

void TestOrderRelaxed()
{
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    //理想情况在t1里下x先赋值，y再赋值
    //到t2中y==true会推出循环，然后++z
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5

}

void TestOderRelaxed2() {
    std::atomic<int> a{ 0 };
    std::vector<int> v3, v4;
    std::thread t1([&a]() {
        for (int i = 0; i < 10; i += 2) {
            a.store(i, std::memory_order_relaxed);
        }
        });
    std::thread t2([&a]() {
        for (int i = 1; i < 10; i += 2)
            a.store(i, std::memory_order_relaxed);
        });
    std::thread t3([&v3, &a]() {
        for (int i = 0; i < 10; ++i)
            v3.push_back(a.load(std::memory_order_relaxed));
        });
    std::thread t4([&v4, &a]() {
        for (int i = 0; i < 10; ++i)
            v4.push_back(a.load(std::memory_order_relaxed));
        });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    for (int i : v3) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    for (int i : v4) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
}
void TestDependency() 
{
    atomic<int> i;
    atomic<char> s[10];
    for (int i = 0; i < 9; i++)
        s[i] = i;
    thread t1([&i]() 
        {
            for (int n = 0; n < 9; i++) 
            {
                ++i;
            }
        });

    thread t2([&s, &i]() 
        {
            while (i < 9) 
            {
                char a = s[i].load();
                cout << a<<endl;
            }
        });

    t1.join();
    t2.join();

    //错误示范，不能成功同步
}
int main() {
    //std::atomic::is_always_lock_free。这个成员变量的值表示在任意给定的目标硬件上，
    //原子类型v否始终以无锁结构形式实现
    atomic<int> v = 6;
    //Test_atomic();
    //Test_sql();
    //TestOrderRelaxed();
    //TestOderRelaxed2();
    TestDependency();
    return 0;
}

