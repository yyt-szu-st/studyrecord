#pragma once
#include<queue>
#include<mutex>
#include<stack>
#include<iostream>
using namespace std;

class Mydata
{
public:
	Mydata(int data) :_data(data) {};
	Mydata(const Mydata& m) :_data(m._data) {};
	Mydata(Mydata&& m) :_data(m._data) {};
	friend std::ostream& operator <<(std::ostream& os, const Mydata& mc)
	{
		os << mc._data;
		return os;
	}
private:
	int _data;
};


template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();    //⇽-- - ①
    }
    void wait_and_pop(T& value)    //⇽-- - ②
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        value = std::move(data_queue.front());
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop()   // ⇽-- - ③
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });   // ⇽-- - ④
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();    //⇽-- - ⑤
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};


struct empty_stack :std::exception //继承exception
{
    const char* what() const throw();
};//定义异常
template<typename T>
class ThreadSafe_Stack
{
private:
    stack<T> data;
    mutable mutex m;
public:
    ThreadSafe_Stack() {}
    ThreadSafe_Stack(const ThreadSafe_Stack& other)
    {
        std::lock_guard<mutex> lk(other.m);//将其他的类锁住
        data = other.data;
    }
    ThreadSafe_Stack& operator=(const ThreadSafe_Stack&) = delete;
    void push(T new_val)
    {
        lock_guard<mutex> lk(m);
        data.push(move(new_val));
    }

    shared_ptr<T> pop()//返回智能指针
    {
        if (data.empty()) throw empty_stack();
        shared_ptr<T> const res(make_shared<T>(move(data.top())));//万一内存不不够很不安全
        data.pop();        return res;
    }

    void pop(T& value)//返回普通版本 
    {
        lock_guard<mutex> lk(m);
        if (data.empty()) throw empty_stack();
        value = data.top();
    }

    bool empty() const
    {
        lock_guard<mutex> lock(m);
        return data.empty();
    }
};

template<typename  T>
class threadsafe_stack_waitable
{
private:
    std::stack<T> data;
    mutable std::mutex m;
    std::condition_variable cv;
public:
    threadsafe_stack_waitable() {}
    threadsafe_stack_waitable(const threadsafe_stack_waitable& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }
    threadsafe_stack_waitable& operator=(const threadsafe_stack_waitable&) = delete;
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));    // ⇽-- - 1
        cv.notify_one();
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this]()
            {
                if (data.empty())
                {
                    return false;
                }
                return true;
            }); //  ⇽-- - 2
        std::shared_ptr<T> const res(
            std::make_shared<T>(std::move(data.top())));   // ⇽-- - 3
        data.pop();   // ⇽-- - 4
        return res;
    }
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this]()
            {
                if (data.empty())
                {
                    return false;
                }
                return true;
            });
        value = std::move(data.top());   // ⇽-- - 5
        data.pop();   // ⇽-- - 6
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty())
        {
            return false;
        }
        value = std::move(data.top());
        data.pop();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty())
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};
//用智能指针实现线程安全的队列
template<typename T>
class threadsafe_queue_ptr
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T>> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue_ptr()
    {}
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });//阻塞等待回答
        value = std::move(*data_queue.front());    //⇽-- - 1
        data_queue.pop();
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(*data_queue.front());   // ⇽-- - 2
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        std::shared_ptr<T> res = data_queue.front();   // ⇽-- - 3
        //一般智能指针的拷贝不会因为内存耗尽出现问题
        //开销在栈上，如果使用move的话开辟空间在堆上，有可能导致malloc错误
        data_queue.pop();
        return res;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res = data_queue.front();   // ⇽-- - 4
        data_queue.pop();
        return res;
    }
    void push(T new_value)
    {
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value)));   // ⇽-- - 5
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

//因为队列是先进先出出的，所以不用考虑出队和入队之间的加锁，可以手动实现一个链表
template<typename T>
class threadsafe_queue_ht
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }
    std::unique_lock<std::mutex> wait_for_data()
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&] {return head.get() != get_tail(); }); //5
        return std::move(head_lock);
    }
    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }
    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*head->data);
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }
public:
    threadsafe_queue_ht() :  // ⇽-- - 1
        head(new node), tail(head.get())
    {}
    threadsafe_queue_ht(const threadsafe_queue_ht& other) = delete;
    threadsafe_queue_ht& operator=(const threadsafe_queue_ht& other) = delete;
    std::shared_ptr<T> wait_and_pop() //  <------3
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }
    void wait_and_pop(T& value)  //  <------4
    {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }
    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head;
    }
    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }
    void push(T new_value)  //<------2
    {
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node* const new_tail = p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
        data_cond.notify_one();
    }
};