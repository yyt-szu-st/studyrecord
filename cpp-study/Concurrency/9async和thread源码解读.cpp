#include <iostream>
#include<thread>
#include<future>
using namespace std;
void work() 
{
    this_thread::sleep_for(chrono::seconds(3));
    return;
}
void test_error() 
{
    thread t1(work);
    thread t2(work);

    t1 = move(t2);
    t1.join();
    t2.join();
    //这里会造成错误
    //源码
    /*
    thread& operator=(thread && _Other) noexcept {
        if (joinable()) {
            _STD terminate();
        }

        _Thr = _STD exchange(_Other._Thr, {});
        return *this;
    }*/

    //当检查t2是否是joinable的时候，会抛出错误
}

void ChangeValue() 
{
    int m = 10;
    thread t1{ [](int& a)
        {
            a++;
        }, 
        ref(m) };
    t1.join();
    cout << m;

}

void test_async() 
{
    //我们使用async时，其实其内部调用了thread,pacakged_task,future等机制。
    //async会返回一个future这个future如果会在被析构时等待其绑定的线程任务是否执行完成
    std::cout << "begin block async" << std::endl;
    {
        //当任务执行完，future会执行析构函数
        //引用计数为1
        future<void> fut=std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "std::async called " << std::endl;
            });
    }
    std::cout << "end block async" << std::endl;
}

void source_async()
{/*
 async源码

    _Associated_state<typename _P_arg_type<_Ret>::type>* _Get_associated_state(launch _Psync, _Fty && _Fnarg) {
        // construct associated asynchronous state object for the launch type
        switch (_Psync) { // select launch type
        case launch::deferred:
            return new _Deferred_async_state<_Ret>(_STD forward<_Fty>(_Fnarg));
        case launch::async: // TRANSITION, fixed in vMajorNext, should create a new thread here
        default:
            return new _Task_async_state<_Ret>(_STD forward<_Fty>(_Fnarg));
        }
    }

 ！！当调用async的时候会选择状态，当选择的是async时会调用_Task-async_state类！！

_Task-async_state类源码

    template <class _Rx>
class _Task_async_state : public _Packaged_state<_Rx()> {
    // class for managing associated synchronous state for asynchronous execution from async
public:
    using _Mybase     = _Packaged_state<_Rx()>;
    using _State_type = typename _Mybase::_State_type;

    ~_Task_async_state() noexcept override {
        _Wait();
    }

    void _Wait() override { // wait for completion
        _Task.wait();
    }

private:
    ::Concurrency::task<void> _Task;
};

！！该类在析构的时候会调用_Task.wait()等待任务完成！！

先西沟子类再析构父类，当析构future的时候，会析构其父类_Associated_state析构，会调用关联状态，关联状态的析构会delete this，
因为其实现了虚析构实现了多态的效果
接着调用具体格式，即async的析构，会调用 _Task_async_state，就会等待任务的完成（阻塞等待）
*/

}


void Deadlock() 
{
    mutex mtx;
    cout << "Deadlock is running"<<endl;
    unique_lock<mutex> unlock(mtx); 
    {
        future<void> futures = async(launch::async, [&mtx]()
            {
                cout << "async called" << endl;
                unique_lock<mutex> unlock(mtx);
                cout << "async working" << endl;

            });
    }

    cout << "Deadlock end" << endl;
}

int asyncFunc() {
    this_thread::sleep_for(chrono::seconds(3));
    std::cout << "this is asyncFunc" << endl;
    return 0;
}
void func1(future<int>& fut) 
{
    cout <<"this is func1" << endl;
    fut = async(launch::async, asyncFunc);
}

void func2(future<int>& fut) 
{
    cout << "this is func2" << endl;
    auto fut_ = fut.get();//获取数据
    if (fut_ == 0) 
    {
        cout << "get asyncFunc value success" << endl;
    }
    else 
    {
        cout << "fail to get asyncFunc value" << endl;
    }
}

void test_asyncfunc() 
{
    future<int> fut;

    //必须保证func1和func2用的是引用
    func1(fut);//func1并没有阻塞的等待
    func2(fut);
}

template<typename Func, typename... Args  >
auto  ParallenExe(Func&& func, Args && ... args) -> std::future<decltype(func(args...))> {
    typedef    decltype(func(args...))  RetType;
    std::function<RetType()>  bind_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    //bind_func返回的类型时RetType，参数为(),后面将参数绑定
    std::packaged_task<RetType()> task(bind_func);
    auto rt_future = task.get_future();
    std::thread t(std::move(task));

    t.detach();//后台执行，用join会阻塞地等待
    return rt_future;
}

void TestParallen1() {
    int i = 0;
    std::cout << "Begin TestParallen1 ..." << std::endl;
    {
        //并行的执行
        ParallenExe([](int i) {
            while (i < 3) {
                i++;
                std::cout << "ParllenExe thread func " << i << " times" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            }, i);
    }
    std::cout << "End TestParallen1 ..." << std::endl;
}

void TestParallen2() {
    int i = 0;
    std::cout << "Begin TestParallen2 ..." << std::endl;
    auto rt_future = ParallenExe([](int i) {
        while (i < 3) {
            i++;
            std::cout << "ParllenExe thread func " << i << " times" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }, i);
    std::cout << "End TestParallen2 ..." << std::endl;
    rt_future.wait();
}
int main()
{
    //test_error();
    //ChangeValue();
    //test_async();
    //Deadlock();
    //test_asyncfunc();
    //TestParallen1();
    //如果没有汇合，则会导致子线程没执行完就关闭
    TestParallen2();
    //this_thread::sleep_for(chrono::seconds(5));
}
