﻿#include <iostream>
#include<thread>
#include<set>
#include<mutex>
#include"lock_free_stack.h"
#include <cassert>
//利用原子变量的compare_exchange来实现

using namespace std;

void TestLockFreeStack() {
    lock_free_stack<int> lk_free_stack;
    std::set<int>  rmv_set;
    std::mutex set_mtx;
    std::thread t1([&]() {
        for (int i = 0; i < 20000; i++) {
            lk_free_stack.push(i);
            std::cout << "push data " << i << " success!" << std::endl;
        }
        });
    std::thread t2([&]() {
        for (int i = 0; i < 10000;) {
            auto head = lk_free_stack.pop();
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
            auto head = lk_free_stack.pop();
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
    TestLockFreeStack();
}
