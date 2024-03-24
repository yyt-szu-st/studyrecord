﻿#include <iostream>
#include<thread>
#include<mutex>
#include <shared_mutex>
#include <map>
std::mutex mtx;
//!!一个互斥量要对应一把锁
int shared_data = 0;
//unique_lock和lock_guard区别
//lock_guard:通过构造函数和析构函数控制锁的作用范围，创造对象的时候加锁，离开作用域的时候解锁
//unique_lock：可以通过构造函数和析构函数控制锁的作用范围，
//也可以在构造函数中延时加锁，在需要的时候手动加锁和解锁。
//unique_lock相对而言更加灵活,对于资源的消耗明显要大一些，因为它要维持mutex的状态；
//lock_guard虽然笨重一些, 但是资源消耗相对要小一点


//unique_lock 基本用法
void use_unique() {
    //lock可自动解锁，也可手动解锁
    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "lock success" << std::endl;
    shared_data++;
    lock.unlock();
}

void owns_unique_lock ()
{
    //unique_lock可以自动解锁也可以手动解锁
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    if (lock.owns_lock())//判断是否加锁 
    {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }

    lock.unlock(); 
    if (lock.owns_lock())//判断是否加锁 
    {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
}

//可以延迟加锁
void defer_lock() {
    //延迟加锁
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
    //可以加锁
    lock.lock();
    //可以自动析构解锁，也可以手动解锁
    lock.unlock();
}

//同时使用owns和defer
void use_own_defer() {
    std::unique_lock<std::mutex>  lock(mtx);
    // 判断是否拥有锁
    if (lock.owns_lock())
    {
        std::cout << "Main thread has the lock." << std::endl;
    }
    else
    {
        std::cout << "Main thread does not have the lock." << std::endl;
    }
    std::thread t([]() {
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);//这里选择延迟加锁
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 加锁
        lock.lock();
        //这里加锁会阻塞
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 解锁
        lock.unlock();
        });
    t.join();
}

//同样支持领养操作
void use_own_adopt() {
    mtx.lock();
    std::unique_lock<std::mutex> lock(mtx, std::adopt_lock);
    //只负责析构，依旧后续操作，不负责出生
    if (lock.owns_lock()) {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "does not have the lock" << std::endl;
    }
    lock.unlock();
}

int a = 10;
int b = 99;
std::mutex mtx1;
std::mutex mtx2;
void safe_swap1() 
{
    std::lock(mtx1, mtx2);
    std::unique_lock<std::mutex> lock1(mtx1,std::adopt_lock);
    std::unique_lock<std::mutex> lock2(mtx2,std::adopt_lock);
    std::swap(a, b);

    //错误用法
    //mtx1.unlock();
    //mtx2.unlock();
    //mtx已经交给lock去接管,所以解锁应该用lock

    //正确用法
    //lock1.unlock();
    //lock2.unlock();
}


void safe_swap2() {
    std::unique_lock<std::mutex> lock1(mtx1, std::defer_lock);
    std::unique_lock<std::mutex> lock2(mtx2, std::defer_lock);
    //需用lock1,lock2加锁
    std::lock(lock1, lock2);
    //错误用法
    //std::lock(mtx1, mtx2);
    //原因同上
    std::swap(a, b);
}

//转移互斥量的所有权
//互斥量本身不支持move操作，但是unique_lock支持
std::unique_lock<std::mutex> get_lock() 
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    return lock;
}
//编译器找不到拷贝构造和移动构造，就会执行移动构造（返回值优化）

void use_return() 
{
    std::unique_lock<std::mutex> lock(get_lock());//get_lock返回的是一个右值
    shared_data++;
}


//锁的粒度表示加锁的精细程度
//一个锁的粒度要足够大，保证可以锁住要访问的共享数据。
//同时一个锁的粒度要足够小，保证非共享数据不被锁住影响性能。
void precision_lock() 
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    lock.unlock();
    //不设计共享数据的耗时操作不要放在锁内执行
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

//c++17 标准shared_mutex
//c++14 提供了shared_time_mutex
//c++11 无上述互斥，想使用可以利用boost、

//在Linux中使用需要-std=c++17
class DNService {
public:
    DNService() {}
    //读操作采用共享锁
    std::string QueryDNS(std::string dnsname) {
        std::shared_lock<std::shared_mutex> shared_locks(_shared_mtx);
        auto iter = _dns_info.find(dnsname);
        if (iter != _dns_info.end()) {
            return iter->second;
        }
        return "";
    }
    //写操作采用独占锁
    void AddDNSInfo(std::string dnsname, std::string dnsentry) {
        std::lock_guard<std::shared_mutex>  guard_locks(_shared_mtx);//让一个线程独占一个锁
        _dns_info.insert(std::make_pair(dnsname, dnsentry));
    }
private:
    std::map<std::string, std::string> _dns_info;
    mutable std::shared_mutex  _shared_mtx;
};



class RecursiveDemo {
public:
    RecursiveDemo() {}
    bool QueryStudent(std::string name) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            return false;
        }
        return true;
    }
    void AddScore(std::string name, int score) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        if (!QueryStudent(name)) {
            _students_info.insert(std::make_pair(name, score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
    }
    //不推荐采用递归锁，使用递归锁说明设计思路并不理想，需优化设计
    //推荐拆分逻辑，将共有逻辑拆分为统一接口
    void AddScoreAtomic(std::string name, int score) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            _students_info.insert(std::make_pair(name, score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
        return;
    }
private:
    std::map<std::string, int> _students_info;
    std::recursive_mutex   _recursive_mtx;
};

int main()
{
    std::cout << a << std::endl << b;
}
