#pragma once
#include <iostream>
#include<map>
#include<vector>
#include<mutex>
#include<thread>
#include<list>
#include<memory>
#include<shared_mutex>
class MyClass
{
public:
    MyClass(int i) :_data(i) {}
    friend std::ostream& operator << (std::ostream& os, const MyClass& mc) {
        os << mc._data;
        return os;
    }
private:
    int _data;
};