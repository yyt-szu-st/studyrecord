#include <iostream>
#include<queue>
#include<mutex>
#include<thread>
#include<condition_variable>
using namespace std;
//actor
//通过消息传递的方式与外界通信。消息传递是异步的。每个actor都有一个邮箱，该邮箱接收并缓存其他actor发过来的消息，actor一次只能同步处理一个消息，处理消息过程中，除了可以接收消息，不能做任何其他操作。
//每一个类独立在一个线程里称作Actor，
//Actor之间通过队列通信，比如Actor1 发消息给Actor2， Actor2 发消息给Actor1都是投递到对方的队列中。
//消除了共享，在网络通信中使用较为频繁，对于逻辑层的处理就采用了将要处理的逻辑消息封装成包投递给逻辑队列
//逻辑类从队列中消费的思想，Erlang是天然支持Actor的语言


//CSP
//CSP是 Communicating Sequential Process 的简称，中文可以叫做通信顺序进程，
//是一种并发编程模型，是一个很强大的并发数据模型，
//相对于Actor模型，CSP中channel是第一类对象，它不关注发送消息的实体，而关注与发送消息时使用的channel

//class Channal {
//public:
//	queue<std::thread::id>message;
//public:
//	queue<std::thread::id> get_q()
//	{
//		return message;
//	}
//	void print()
//	{
//		while (!message.empty())
//		{
//			cout << message.front() << endl;
//			message.pop();
//		}
//	}
//};
//void w(Channal* c)
//{
//	auto a = this_thread::get_id();
//	c->message.push(a);
//}
//class user
//{
//public:
//	void write(Channal& c)
//	{
//		thread t1(w,c);
//		t1.join();
//	}
//
//	
//};
// 
//by myself

template <typename T>
class Channel {
private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    size_t capacity_;
    bool closed_ = false;
public:
    Channel(size_t capacity = 0) : capacity_(capacity) {}
    bool send(T value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_producer_.wait(lock, [this]() {
            // 对于无缓冲的channel，我们应该等待直到有消费者准备好
            return (capacity_ == 0 && queue_.empty()) || queue_.size() < capacity_ || closed_;
            });
        if (closed_) {
            return false;
        }
        queue_.push(value);
        cv_consumer_.notify_one();
        return true;
    }
    bool receive(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_consumer_.wait(lock, [this]() { return !queue_.empty() || closed_; });
        if (closed_ && queue_.empty()) {
            return false;
        }
        value = queue_.front();
        queue_.pop();
        cv_producer_.notify_one();
        return true;
    }
    void close() {
        std::unique_lock<std::mutex> lock(mtx_);
        closed_ = true;
        cv_producer_.notify_all();
        cv_consumer_.notify_all();
    }
};
// 示例使用
int main() {
    Channel<int> ch(10);  // 10缓冲的channel
    std::thread producer([&]() {
        for (int i = 0; i < 5; ++i) {
            ch.send(i);
            std::cout << "Sent: " << i << std::endl;
        }
        ch.close();
        });
    std::thread consumer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 故意延迟消费者开始消费
        int val;
        while (ch.receive(val)) {
            std::cout << "Received: " << val << std::endl;
        }
        });
    producer.join();
    consumer.join();
    return 0;
}

