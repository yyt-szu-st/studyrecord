﻿#include <iostream>
#include<map>
#include<vector>
#include<mutex>
#include<thread>
#include<list>
#include<memory>
#include<shared_mutex>
#include"ThreadSafeLookUpTable.h"
#include"MyClass.h"
#include<set>
using namespace std;
void TestThreadSafeHash() {
    std::set<int> removeSet;
    threadsafe_lookup_table<int, std::shared_ptr<MyClass>> table;
    std::thread t1([&]() {
        for (int i = 0; i < 100; i++)
        {
            auto class_ptr = std::make_shared<MyClass>(i);
            table.add_or_update_mapping(i, class_ptr);
        }
        });
    std::thread t2([&]() {
        for (int i = 0; i < 100; )
        {
            auto find_res = table.value_for(i, nullptr);
            if (find_res)
            {
                table.remove_mapping(i);
                removeSet.insert(i);
                i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        });
    std::thread t3([&]() {
        for (int i = 100; i < 200; i++)
        {
            auto class_ptr = std::make_shared<MyClass>(i);
            table.add_or_update_mapping(i, class_ptr);
        }
        });
    t1.join();
    t2.join();
    t3.join();
    for (auto& i : removeSet)
    {
        std::cout << "remove data is " << i << std::endl;
    }
    auto copy_map = table.get_map();
    for (auto& i : copy_map)
    {
        std::cout << "copy data is " << *(i.second) << std::endl;
    }
}


int main()
{
    threadsafe_lookup_table<int, int, int> hash;

}