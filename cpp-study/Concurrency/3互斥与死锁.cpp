#include <iostream>
#include<thread>
#include<mutex>
#include<stack>
std::mutex mtx1;
//mutex对共享数据进行加锁，防止多线程访问共享区造成数据不一致的问题
int share_data = 100;

void use_lock() {
    while (true) 
    {
        mtx1.lock();//加锁保证其他线程进不来
        share_data++;
        std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
        std::cout << "Share data is " << share_data << std::endl;
        mtx1.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));//需要进行短时间的睡眠防止死循环长时间的占用
    }
}

void test_lock() {
    std::thread t1(use_lock);

    std::thread t2([]() {//通过lambda表达式创建线程
        while (true) {
            {
                std::lock_guard<std::mutex> lk_guard(mtx1);
                share_data--;
                std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
                std::cout << "sharad data is " << share_data << std::endl; 
            }//执行到这里会调用lock_guard的析构函数
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        });
    t1.join();
    t2.join();
}

//利用RAII思想自动加锁,在析构函数处解锁
//std::lock_guard<std::mutex> lock(mtx1)

template<typename T>
class threadsafe_stack1{//线程不安全示例
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack1(){};
    threadsafe_stack1(const threadsafe_stack1& other) {
        std::lock_guard<std::mutex> lock(other.m);//防止在拷贝别人的数据的时候被修改
        data = other.data;
    }

    threadsafe_stack1& operator=(const threadsafe_stack1&) = delete;//允许该类进行拷贝构造，不允许传参构造
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }

    //问题代码
    T pop()
    {
        std::lock_guard<std::mutex> lock(m);
        auto element = data.top();
        data.pop();
        return element;
    }//有可能返回值过大，导致没有空间继续拷贝
    //但此时栈已经弹出，栈变成了空，就会导致数据丢失

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }//布尔变量被外边使用，有可能在这段时间内bool变量改变
};

void test_threadsafe_stack1() {//危险的使用方式造成异常
    threadsafe_stack1<int> safe_stack;
    safe_stack.push(1);

    std::thread t1([&safe_stack]() {
        if (!safe_stack.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));//模拟程序执行
            safe_stack.pop();}
        });

    std::thread t2([&safe_stack]() {
        if (!safe_stack.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
        });
    //当两个函数同时pop的时候就会出现问题
    t1.join();
    t2.join();
}
//为了解决这个问题，可以使用抛出异常的方式
struct empty_stack : std::exception
{
    const char* what() const throw();
};
//线程安全的栈
template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack() {}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        //①在构造函数的函数体（constructor body）内进行复制操作
        data = other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }

    //方式一
    std::shared_ptr<T> pop()//智能指针
    {
        std::lock_guard<std::mutex> lock(m);
        //②试图弹出前检查是否为空栈
        if (data.empty())
        {
            throw empty_stack();

            //return nullptr;可以直接返回空指针
            //没必要抛出异常
        }
        //③改动栈容器前设置返回值
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));//获取栈顶部元素，拷贝成一个T类型的智能指针
        data.pop();
        return res;
    }
    //方式二
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) throw empty_stack();
        value = data.top();
        data.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

//死锁是怎么造成的：调用顺序不一致
//当线程1先加锁A，再加锁B，而线程2先加锁B，再加锁A。那么在某一时刻就可能造成死锁
std::mutex  t_lock1;
std::mutex  t_lock2;
int m_1 = 0;
int m_2 = 1;
void dead_lock1() {
    while (true) {
        std::cout << "dead_lock1 begin " << std::endl;
        t_lock1.lock();
        m_1 = 1024;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        t_lock2.lock();
        m_2 = 2048;
        t_lock2.unlock();
        t_lock1.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "dead_lock2 end " << std::endl;
    }
}
void dead_lock2() {
    while (true) {
        std::cout << "dead_lock2 begin " << std::endl;
        t_lock2.lock();
        m_2 = 2048;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        t_lock1.lock();
        m_1 = 1024;
        t_lock1.unlock();
        t_lock2.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "dead_lock2 end " << std::endl;
    }
}

void test_dead_lock()
{
    std::thread t1(dead_lock1);
    std::thread t2(dead_lock2);

    t1.join();
    t2.join();
}
//实际工作中避免死锁的一个方式就是加锁和解锁功能封装为独立的函数
//这样能保证在独立的函数里执行完操作之后就解锁，不会导致一个函数里使用多个锁




//加锁和解锁作为原子操作解耦合，各自只管理自己的功能
//每次使用完锁会立刻解锁，就不会出现持有锁一同时使用锁二
void atomic_lock1() {
    std::cout << "lock1 begin lock" << std::endl;
    t_lock1.lock();
    m_1 = 1024;
    t_lock1.unlock();
    std::cout << "lock1 end lock" << std::endl;
}
void atomic_lock2() {
    std::cout << "lock2 begin lock" << std::endl;
    t_lock2.lock();
    m_2 = 2048;
    t_lock2.unlock();
    std::cout << "lock2 end lock" << std::endl;
}
void safe_lock1() {
    while (true) {
        atomic_lock1();
        atomic_lock2();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
void safe_lock2() {
    while (true) {
        atomic_lock2();
        atomic_lock1();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
void test_safe_lock() {
    std::thread t1(safe_lock1);
    std::thread t2(safe_lock2);
    t1.join();
    t2.join();
}

//对于要使用两个互斥量，可以同时加锁
//如果不同时加锁，可能会死锁

//假设是一个很复杂的数据结构，我们不建议执行拷贝构造

class som_big_object 
{
private:
    int _data;
public:
    som_big_object(int data) :_data(data) {};
    //拷贝构造
    som_big_object(const som_big_object& b2):_data(b2._data) {};
    //移动构造.完美转发，右值引用，使他们指向一个地址
    som_big_object(som_big_object&& b2) :_data(std::move(b2._data)) {
    };

    friend std::ostream& operator <<(std::ostream& os, const som_big_object& big_obj) 
    {
        os << big_obj._data;
        return os;
    }

    //重载赋值运算符
    //系统默认不会给你拷贝赋值，拷贝构造
    //拷贝赋值
    som_big_object& operator=(const som_big_object& b2) 
    {
        if (this == &b2) 
        {
            return *this;
        }

        _data = b2._data;
        return *this;
    }
    //移动赋值
    som_big_object& operator=(const som_big_object&& b2)
    {
        _data = std::move(b2._data);
        return *this;
    }

    //交换数据
    friend void swap(som_big_object& b1, som_big_object& b2) {
        som_big_object temp = std::move(b1);
        b1 = std::move(b2);
        b2 = std::move(temp);
    }


};
void test_move()
{
    som_big_object bigobj1(100);
    som_big_object bigobj2(200);
    bigobj2 = bigobj1;
    bigobj2 = std::move(bigobj1);
    //move会返回一个右值引用
}
//假设这是一个结构，包含了复杂的成员对象和互斥量
class big_object_mgr {
public:
    big_object_mgr(int data = 0) :_obj(data) {}
    void printinfo() {
        std::cout << "current obj data is " << _obj << std::endl;
    }
    friend void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2);
    friend void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2);
    friend void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2);
private:
    std::mutex _mtx;
    som_big_object _obj;
};

void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::lock_guard <std::mutex> gurad1(objm1._mtx);
    //此处为了故意制造死锁，我们让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

//同时加锁是最安全的形式，让一个线程持有两个锁
void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::lock(objm1._mtx, objm2._mtx);
    //领养锁管理互斥量解锁
    //加入后面的参数，不负责锁的定义，只负责解锁
    std::lock_guard<std::mutex> guard11(objm1._mtx, std::adopt_lock);
    //此处为了故意制造死锁，我们让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard12(objm2._mtx, std::adopt_lock);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

//c++17提供的新特性，不用领养的方式,直接使用scope
void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }

    std::scoped_lock guard0(objm1._mtx, objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}


void  test_danger_swap() {
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(danger_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(danger_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}
void  test_safe_swap() {
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}
void test_safe_swap_scope() 
{
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap_scope, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap_scope, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

//有一些项目不支持我们使用同时加两个锁（对性能的要求）
//解决方案：层级锁
//设置不同层级，在启动的时候就会抛出异常

class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned long value) :_hierarchy_value(value),
        _previous_hierarchy_value(0) {}//显式调用
    hierarchical_mutex(const hierarchical_mutex&) = delete;
    hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;
    //没有定义移动构造，在使用std::move的时候会执行拷贝构造
    void lock() {
        check_for_hierarchy_violation();
        _internal_mutex.lock();//加锁
        update_hierarchy_value();//更新值
    }
    void unlock() {
        if (_this_thread_hierarchy_value != _hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        _this_thread_hierarchy_value = _previous_hierarchy_value;
        _internal_mutex.unlock();
    }

    //非阻塞式
    bool try_lock() {
        check_for_hierarchy_violation();
        if (!_internal_mutex.try_lock()) {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
private:
    std::mutex  _internal_mutex;
    //当前层级值
    //每把锁都会有一个当前的层级值
    unsigned long const _hierarchy_value;
    //上一次层级值
    unsigned long _previous_hierarchy_value;
    //本线程记录的层级值
    static thread_local  unsigned long  _this_thread_hierarchy_value;
    void check_for_hierarchy_violation() {
        if (_this_thread_hierarchy_value <= _hierarchy_value) {
            throw  std::logic_error("mutex  hierarchy violated");
        }
    }
    void  update_hierarchy_value() {
        _previous_hierarchy_value = _this_thread_hierarchy_value;
        _this_thread_hierarchy_value = _hierarchy_value;
    }
};

thread_local unsigned long hierarchical_mutex::_this_thread_hierarchy_value(ULONG_MAX);
void test_hierarchy_lock() {

    //数字大的必须要先加
    hierarchical_mutex  hmtx1(1000);
    hierarchical_mutex  hmtx2(500);
    std::thread t1([&hmtx1, &hmtx2]() {
        hmtx1.lock();
        hmtx2.lock();
        hmtx2.unlock();
        hmtx1.unlock();
        });
    std::thread t2([&hmtx1, &hmtx2]() {
        hmtx2.lock();
        hmtx1.lock();
        hmtx1.unlock();
        hmtx2.unlock();
        });
    t1.join();
    t2.join();
}
int main()
{

    test_hierarchy_lock();
}