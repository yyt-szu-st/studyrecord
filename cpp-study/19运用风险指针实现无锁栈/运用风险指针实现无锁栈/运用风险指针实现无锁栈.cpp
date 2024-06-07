#include <iostream>
#include<mutex>
#include<thread>
#include<set>
#include<cassert>
#include"HazardPointerStack.h"
using namespace std;
//术语“风险指针”是指Maged Michael发明的一种技法， 后来被IBM申请为专利。
//前文我们设计了无锁并发栈的结构，对于pop操作回收节点采用的是延时删除的策略，
//即将要删除的节点放入待删除列表中。但是待删列表中的节点可能永远不会被回收，
//因为每次多个线程pop就不会触发回收待删列表的操作。上一节我们说可以通过执行pop的最后一个线程执行回收，
//那为了实现这个目的，我们就要换一种思路。就是我们将要删除的节点做特殊处理，如果有线程使用它，
//就将他标记为正在使用，那么这个节点的指针就是风险指针，也就是不能被其他线程删除。

void TestHazardPointer() {
    hazard_pointer_stack<int> hazard_stack;
    std::set<int>  rmv_set;
    std::mutex set_mtx;
    std::thread t1([&]() {
        for (int i = 0; i < 20000; i++) {
            hazard_stack.push(i);
            std::cout << "push data " << i << " success!" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        });
    std::thread t2([&]() {
        for (int i = 0; i < 10000;) {
            auto head = hazard_stack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
        });
    std::thread t3([&]() {
        for (int i = 0; i < 10000;) {
            auto head = hazard_stack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            std::cout << "pop data " << *head << " success!" << std::endl;
            i++;
        }
        });
    t1.join();
    t2.join();
    t3.join();
    assert(rmv_set.size() == 20000);
}
int main()
{
    TestHazardPointer();
    std::cout << "Hello World!\n";
}
