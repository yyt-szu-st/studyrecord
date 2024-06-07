#pragma once
#include <iostream>
#include<map>
#include<vector>
#include<mutex>
#include<thread>
#include<list>
#include<memory>
#include<shared_mutex>
//template<typename T>
class ThreadSafeHash
{
private:
    vector<T>
        ThreadSafeHash(const ThreadSafeHash&) = delete;
    ThreadSafeHash& operator=(const ThreadSafeHash&) = delete;
    struct node
    {
        T data;
        unique_ptr<T> next;
    };
public:
    ThreadSafeHash(ThreadSafeHash&& t)
    {
    }

};
