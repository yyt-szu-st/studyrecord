#include <iostream>
#include<stack>
#include<mutex>
#include<queue>
#include"All_class.h"
using namespace std;

mutex mtx_cout;
void PrintMyClass(string consumer, shared_ptr<Mydata> data) 
{
    lock_guard<mutex> lk(mtx_cout);
    cout << consumer << " pop data sucess , data is " << (*data)<<endl;
}
void TestThreadSafaStack() 
{
    threadsafe_stack_waitable<Mydata> stack;
    thread producer([&]()
        {
            for (int i = 0; i < 100; i++)
            {
                Mydata mc(i);
                stack.push(move(mc));
            }
        });
    thread consumer1([&]()
        {
            for (;;)
            {
                shared_ptr<Mydata> data = stack.wait_and_pop();
                PrintMyClass("consumer 1", data);
            }
        });
    thread consumer2([&]()
        {
            for (;;)
            {
                shared_ptr<Mydata> data = stack.wait_and_pop();
                PrintMyClass("consumer 1", data);
            }
        });

    consumer1.join();
    consumer2.join();
    producer.join();
   
}
//因为队列是先进先出出的，所以不用考虑出队和入队之间的加锁，可以手动实现一个链表

int main()
{
    //TestThreadSafaStack();
    queue<shared_ptr<int>> q;
    for (int i = 0; i < 5; i++) 
    {
        q.push(make_shared<int>(i));
    }
    cout << "shraed_ptr大小为： " << sizeof(q.front())<<endl;
    for (int i = 0; i < 5; i++) 
    {
        cout << "第" << i << "个指针地址为：" << q.front()<<endl;
        q.pop();
    }
    queue<unique_ptr<int>> q1;
    for (int i = 0; i < 5; i++)
    {
        q1.push(make_unique<int>(i));
    }
    cout << "unique_ptr大小为： " << sizeof(q1.front()) << endl;
    for (int i = 0; i < 5; i++)
    {
        cout << "第" << i << "个指针地址为：" << q1.front() << endl;
        q1.pop();
    }
}
