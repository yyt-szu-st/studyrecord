#include <iostream>
#include<thread>
#include<mutex>
std::mutex mtx_num;
int num = 1;

std::condition_variable cvA;
std::condition_variable cvB;

void PoorImpleman() {//不良的实现方式，这样会造成无谓的时间消耗
    //当线程A发现是2的时候会sleep
    std::thread t1([]() {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 1) {
                    std::cout << "thread A print 1....." << std::endl;
                    num++;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        });
    std::thread t2([]() {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 2) {
                    std::cout << "thread B print 2....." << std::endl;
                    num--;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        });
    t1.join();
    t2.join();
}

//条件变量是处理线程间通信的方法
//对比上面效率会高很多
void ResonableImplemention() {
    std::thread t1([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx_num);

            //写法一
            while (num != 1) {
                cvA.wait(lock);
            }
            //写法二
            cvA.wait(lock, []() {
                return num == 1;
                //这个返回值是给wait的，如果num==1返回true，接着往下走
                //如果是false，则会在这里挂起等待
                });

            //if(num!=1){
            // cvA.wait(lock)
            // }
            //危险用法：会存在虚假唤醒，如果是操作系统唤醒，发现这个线程沉睡太久，进来判断还是false
            num++;
            std::cout << "thread A print 1....." << std::endl;
            //监听线程通知
            cvB.notify_one();//通知一个线程
            cvB.notify_all();//通知所有线程（条件变量）

        }
        });
    std::thread t2([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx_num);
            cvB.wait(lock, []() {
                return num == 2;
                });
            num--;
            std::cout << "thread B print 2....." << std::endl;
            cvA.notify_one();
        }
        });
    t1.join();
    t2.join();
}


//利用条件变量实现队列安全

#include<queue>
#include<memory>
#include<condition_variable>
template<typename T>
class threadsafe_queue 
{
private:
    mutable std::mutex mut;//代表其在const成员函数中也可修改
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue() {};
    threadsafe_queue(threadsafe_queue const& other) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue = other.data_queue;
    };
    //用const引用的好处
       //在不实现移动构造的时候，我们的拷贝构造可以被移动构造实现
    T push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.pust();
        data_cond.notify_one();//通知其他的线程我加入了数据
    }
    void wait_and_pop(T& value)//利用应用修改防止返回的数据过大导致性能消耗
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        value = data_queue.front();//一次拷贝赋值
        data_queue.pop();
    }//在函数内是零拷贝消耗
    std::shared_ptr<T>wait_and_pop() 
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]，{ return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
        //不要返回局部变量的地址
    }//引用基数减一

    bool try_pop(T& value) 
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() 
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty)
            return false;

        std::shared_ptr<T> res(std::make_shared<T>(daya_queue.front()));
        data_queue.pop();
        return res;
    }

    bool empty() const 
    {
        std::lock_guard(std::mutex) lk(mut);
        return data_queue.empty();
    }
};

void test_safe_que() {
    threadsafe_queue<int>  safe_que;
    std::mutex  mtx_print;
    std::thread producer(
        [&]() {
            for (int i = 0; ; i++) {
                safe_que.push(i);
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "producer push data is " << i << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    );
    std::thread consumer1(
        [&]() {
            for (;;) {
                auto data = safe_que.wait_and_pop();
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "consumer1 wait and pop data is " << *data << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    );
    std::thread consumer2(
        [&]() {
            for (;;) {
                auto data = safe_que.try_pop();
                if (data != nullptr) {
                    {
                        std::lock_guard<std::mutex> printlk(mtx_print);
                        std::cout << "consumer2 try_pop data is " << *data << std::endl;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    );
    producer.join();
    consumer1.join();
    consumer2.join();
}
int main()
{
    //PoorImpleman()
   // ResonableImplemention();
    test_safe_que();
}
