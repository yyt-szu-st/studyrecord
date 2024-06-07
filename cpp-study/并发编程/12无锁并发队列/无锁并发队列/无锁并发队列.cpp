#include<thread>
#include <iostream>
#include<atomic>
#include<queue>
#include<mutex>
using namespace std;

//用锁实现环形队列
template<typename T,size_t Cap>
class CircularQueLk :private std::allocator<T>
{
private:
    size_t _max_size;
    T* _data;
    std::mutex _mtx;
    size_t _head = 0;
    size_t _tail = 0;
public:
    CircularQueLk():_max_size(Cap + 1),
        _data(std::allocator<T>::allocate(_max_size)), _head(0), _tail(0) {}
    CircularQueLk(const CircularQueLk&) = delete;
    CircularQueLk operator =(const CircularQueLk&) volatile = delete;
    CircularQueLk& operator = (const CircularQueLk&) = delete;

    ~CircularQueLk() 
    {
        //循环销毁
        std::lock_guard<std::mutex> lk(_mtx);//加入锁
        //调用内部析构函数
        while (_head != _tail) 
        {
            std::allocator<T>::destroy(_data + _head);
            _head++;
        }
        //调用回收操作
        std::allocator<T>::deallocate(_data, _max_size);
    }

    //先实现一个可变参数列表版本的插入函数为最基准函数
    template <typename ...Args>
    bool emplace(Args&& ...args) 
    {
        std::lock_guard<std::mutex> lk(_mtx);

        if ((_tail + 1) % _max_size == _head) 
        {
            cout << "circular queue full !" << endl;
            return false;
        }
        std::allocator<T>::construct(_data + _tail, std::forward<Args>(args)...);
        //更新尾部元素位置
        _tail = (_tail + 1) % _max_size;//保证不会超过最大值
        return true;
    }


    //push 实现两个版本，一个接受左值引用，一个接受右值引用
    //接受左值引用版本
    bool push(const T& val) {
        std::cout << "called push const T& version" << std::endl;
        return emplace(val);
    }
    //接受右值引用版本，当然也可以接受左值引用，T&&为万能引用
    // 但是因为我们实现了const T&
    bool push(T&& val) {
        std::cout << "called push T&& version" << std::endl;
        return emplace(std::move(val));
    }

    //出队函数
    bool pop(T& val) {
        std::lock_guard<std::mutex> lock(_mtx);
        //判断头部和尾部指针是否重合，如果重合则队列为空
        if (_head == _tail) {
            std::cout << "circular que empty ! " << std::endl;
            return false;
        }
        //取出头部指针指向的数据
        val = std::move(_data[_head]);
        //更新头部指针
        _head = (_head + 1) % _max_size;
        return true;
    }
};

void TestCircularQue() 
{
    CircularQueLk<int, 100> cq_lk;
    int a =999;
    cq_lk.push(a);
    cq_lk.push(move(a));

    for (int i = 1; i < 100;i++) 
    {
        auto res = cq_lk.push(i);
        if (res == false)
            break;
    }

    for (int i = 1; i < 100; i++) 
    {
        int a;
        auto res = cq_lk.pop(a);
        if (!res) 
        {
            break;
        }
        cout << "pop success:" << a << endl;
    }

}

//无锁实现循环队列
template<typename T, size_t Cap>
class CircularQueSeq :private std::allocator<T> {
private:
    size_t _max_size;
    T* _data;
    std::atomic<bool> _atomic_using;
    size_t _head = 0;
    size_t _tail = 0;
public:
    CircularQueSeq() :_max_size(Cap + 1), _data(std::allocator<T>::allocate(_max_size)), _atomic_using(false), _head(0), _tail(0) {}
    CircularQueSeq(const CircularQueSeq&) = delete;
    CircularQueSeq& operator = (const CircularQueSeq&) volatile = delete;
    CircularQueSeq& operator = (const CircularQueSeq&) = delete;
    ~CircularQueSeq() {
        //循环销毁
        bool use_expected = false;
        bool use_desired = true;
        do
        {
            use_expected = false;
            use_desired = true;

            //循环检测，但会浪费资源，自旋锁
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        //调用内部元素的析构函数
        while (_head != _tail) 
        {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _max_size;
        }
        //调用回收操作
        std::allocator<T>::deallocate(_data, _max_size);
        do
        {
            use_expected = true;
            use_desired = false;
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
    }
    //先实现一个可变参数列表版本的插入函数最为基准函数
    template <typename ...Args>
    bool emplace(Args && ... args) {
        bool use_expected = false;
        bool use_desired = true;
        do
        {
            use_expected = false;
            use_desired = true;
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        //判断队列是否满了
        if ((_tail + 1) % _max_size == _head) {
            std::cout << "circular que full ! " << std::endl;
            do
            {
                use_expected = true;
                use_desired = false;
            } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
            return false;
        }
        //在尾部位置构造一个T类型的对象，构造参数为args...
        std::allocator<T>::construct(_data + _tail, std::forward<Args>(args)...);
        //更新尾部元素位置
        _tail = (_tail + 1) % _max_size;
        do
        {
            use_expected = true;
            use_desired = false;
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        return true;
    }
    //push 实现两个版本，一个接受左值引用，一个接受右值引用
    //接受左值引用版本
    bool push(const T& val) {
        std::cout << "called push const T& version" << std::endl;
        return emplace(val);
    }
    //接受右值引用版本，当然也可以接受左值引用，T&&为万能引用
    // 但是因为我们实现了const T&
    bool push(T&& val) {
        std::cout << "called push T&& version" << std::endl;
        return emplace(std::move(val));
    }
    //出队函数
    bool pop(T& val) {
        bool use_expected = false;//期望值
        bool use_desired = true;//目的值
        do
        {
            use_desired = true;
            use_expected = false;
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        //判断头部和尾部指针是否重合，如果重合则队列为空
        if (_head == _tail) {
            std::cout << "circular que empty ! " << std::endl;
            do
            {
                use_expected = true;
                use_desired = false;
            } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
            return false;
        }
        //取出头部指针指向的数据
        val = std::move(_data[_head]);
        //更新头部指针
        _head = (_head + 1) % _max_size;
        do
        {
            use_expected = true;
            use_desired = false;
        } while (!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        return true;
    }

};

template<typename T, size_t Cap>
class CircularQueLight : private std::allocator<T>
{
public:
    CircularQueLight() :_max_size(Cap + 1),
        _data(std::allocator<T>::allocate(_max_size))
        , _head(0), _tail(0) {}
    CircularQueLight(const CircularQueLight&) = delete;
    CircularQueLight& operator = (const CircularQueLight&) volatile = delete;
    CircularQueLight& operator = (const CircularQueLight&) = delete;
    bool push(const T& val)
    {
        size_t t;
        do
        {
            t = _tail.load(std::memory_order_relaxed);  //5
            //判断队列是否满
            if ((t + 1) % _max_size == _head.load(std::memory_order_acquire))
            {
                std::cout << "circular que full ! " << std::endl;
                return false;
            }
        } while (!_tail.compare_exchange_strong(t,
            (t + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed));  //6
        _data[t] = val;
        size_t tailup;
        do
        {
            tailup = t;
        } while (_tail_update.compare_exchange_strong(tailup,
            (tailup + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed)); //7
        std::cout << "called push data success " << val << std::endl;
        return true;
    }
    bool pop(T& val) {
        size_t h;
        do
        {
            h = _head.load(std::memory_order_relaxed);  //1 处
            //判断头部和尾部指针是否重合，如果重合则队列为空
            if (h == _tail.load(std::memory_order_acquire)) //2处
            {
                std::cout << "circular que empty ! " << std::endl;
                return false;
            }
            //判断如果此时要读取的数据和tail_update是否一致，如果一致说明尾部数据未更新完
            if (h == _tail_update.load(std::memory_order_acquire)) //3处
            {
                return false;
            }
            val = _data[h]; // 2处
        } while (!_head.compare_exchange_strong(h,
            (h + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed)); //4 处
        std::cout << "pop data success, data is " << val << std::endl;
        return true;
    }
private:
    size_t _max_size;
    T* _data;
    std::atomic<size_t>  _head;
    std::atomic<size_t> _tail;
    std::atomic<size_t> _tail_update;
};
int main()
{
    size_t a;
    cout << sizeof(a);
}



/*
优势

无锁高并发. 虽然存在循环重试, 但是这只会在相同操作并发的时候出现. 
push 不会因为与 pop 并发而重试, 反之亦然.

缺陷

这样队列只应该存储标量, 如果存储类对象时，
多个push线程只有一个线程push成功，而拷贝复制的开销很大，
其他线程会循环重试，每次重试都会有开销。
*/