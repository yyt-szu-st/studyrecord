#include <iostream>
#include<atomic>
#include<thread>
#include<cassert>

using namespace std;


std::atomic <bool> x, y;
std::atomic<int> z;

void write_x()
{
    x.store(true, std::memory_order_release); //1
}
void write_y()
{
    y.store(true, std::memory_order_release); //2
}
void read_x_then_y()
{
    while (!x.load(std::memory_order_acquire));
    if (y.load(std::memory_order_acquire))   //3
        ++z;
}
void read_y_then_x()
{
    while (!y.load(std::memory_order_acquire));
    if (x.load(std::memory_order_acquire))   //4
        ++z;
}

void TestAR()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    a.join();
    b.join();
    c.join();
    d.join();
    assert(z.load() != 0); //5

    //在极端情况下断言也可能会触发
    //在线程间并不一定会有x在y之前写入
    //线程c和线程d读取x和y的顺序不一定一样
    //relaese和racquire能形成同步必然由逻辑关系去维持的    
    //所以即使采用release和acquire的方式，也不能保证全局顺序一致。
    //如果一个线程对变量执行release内存序的store的操作
    //另一个线程不一定会马上读取到
    std::cout << "z value is " << z.load() << std::endl;
}

//通过栅栏保证指令的编排顺序
void write_x_then_y_fence()
{
    x.store(true, std::memory_order_relaxed);  //1
    std::atomic_thread_fence(std::memory_order_release);  //保证在栅栏之前的指令都不会编排在栅栏之后
    y.store(true, std::memory_order_relaxed);  //3
}
void read_y_then_x_fence()
{
    while (!y.load(std::memory_order_relaxed));  //4
    std::atomic_thread_fence(std::memory_order_acquire); //5
    if (x.load(std::memory_order_relaxed))  //6
        ++z;
}

void TestFence()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x_then_y_fence);
    std::thread b(read_y_then_x_fence);
    a.join();
    b.join();
    assert(z.load() != 0);   //7
}
int main()
{

}
