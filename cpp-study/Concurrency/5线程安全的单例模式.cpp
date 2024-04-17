// 线程安全的单例模式.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


//当一个函数中定义一个静态局部变量
#include <iostream>
#include<thread>
#include<mutex>
using namespace std;



class Single2 
{
private:
	Single2() {};
	Single2(const Single2&) = delete;
	Single2& operator=(const Single2&) = delete;
public://无参构造和拷贝构造对外部关闭
	static Single2& GetInst()
	{
		static Single2 single;
		return single;//局部静态变量会放到静态区
	}
};
//c++11以前该方式存在风险，在多个线程初始化时开辟多个实例情况
//c++11以后大部分单例模式都回归到这个模式了
//要求多个线程只要调用这个接口，则生成的局部静态变量都是统一的
void test_single2() 
{
	//多线程会存在问题
	cout << "s1 addr is" << &Single2::GetInst() << endl;
	cout << "s2 addr is" << &Single2::GetInst() << endl;
}


//在C++11 推出以前
// 局部静态变量的方式实现单例存在线程安全问题
// 所以部分人推出了一种方案，就是在主线程启动后
// 其他线程没有启动前，由主线程先初始化单例资源
// 这样其他线程获取的资源就不涉及重复初始化的情况了

//!!饿汉式初始化！！

//饿汉式
class Single2Hungry
{
private:
    Single2Hungry(){}
    Single2Hungry(const Single2Hungry&) = delete;
    Single2Hungry& operator=(const Single2Hungry&) = delete;
public:
    static Single2Hungry* GetInst()
    {
        if (single == nullptr)
        {
            single = new Single2Hungry();
        }
        return single;
    }
private:
    static Single2Hungry* single;
};

//饿汉式初始化
Single2Hungry* Single2Hungry::single = Single2Hungry::GetInst();
void thread_func_s2(int i)
{
    std::cout << "this is thread " << i << std::endl;
    std::cout << "inst is " << Single2Hungry::GetInst() << std::endl;
}
void test_single2hungry()
{
    std::cout << "s1 addr is " << Single2Hungry::GetInst() << std::endl;
    std::cout << "s2 addr is " << Single2Hungry::GetInst() << std::endl;
    for (int i = 0; i < 3; i++)
    {
        std::thread tid(thread_func_s2, i);
        tid.join();
    }
}
//饿汉式是从使用角度规避多线程的安全问题.所以这种方式不是很推荐。

//不应该加以限制，所以就有了懒汉式方式初始化资源，
// 在用到时如果没有初始化单例则初始化，
// 如果初始化了则直接使用.
//所以这种方式我们要加锁，防止资源被重复初始化(在多线程模式下被多个线程同时调用)

//!!懒汉式初始化!!
class SinglePointer 
{
private:
    SinglePointer()
    {
    }
    SinglePointer(const SinglePointer&) = delete;
    SinglePointer& operator=(const SinglePointer&) = delete;
public:
    static SinglePointer* GetInst()
    {
        if (single != nullptr)//判断是不是空指针
        {
            return single;
        }//先不加锁判断以免影响性能
        //有可能A,B同时走到这里
        s_mutex.lock();//加锁，只有一个线程进来

        if (single != nullptr)//再次判断，以免A先加锁然后解锁，然后B再加锁，覆盖掉A的工作
        {
            s_mutex.unlock();
            return single;
        }
        single = new SinglePointer();
        s_mutex.unlock();
        return single;
    }
private:
    static SinglePointer* single;
    static std::mutex s_mutex;
};

SinglePointer* SinglePointer::single = nullptr;
std::mutex SinglePointer::s_mutex;
void thread_func_lazy(int i)
{
    std::cout << "this is lazy thread " << i << std::endl;
    std::cout << "inst is " << SinglePointer::GetInst() << std::endl;
}
void test_singlelazy()
{
    for (int i = 0; i < 3; i++)
    {
        std::thread tid(thread_func_lazy, i);
        tid.join();
    }
    //何时释放new的对象？造成内存泄漏
    //不知道哪个线程开辟的资源，所以无法回收
    //如果每个线程都释放，就造成了多次析构的问题
}

//智能指针来完成自动回收
class SingleAuto 
{
private:
    SingleAuto() {};
    SingleAuto(const SingleAuto&) = delete;
    SingleAuto& operator=(const SingleAuto&) = delete;
public:
    ~SingleAuto() 
    {
        cout << "Single auto delete success " << endl;
    }

    static std::shared_ptr<SingleAuto> GetInst()
    {
        if (single != nullptr)
        {
            return single;
        }
        s_mutex.lock();
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        single = std::shared_ptr<SingleAuto>(new SingleAuto);
        s_mutex.unlock();
        return single;
    }

private:
    static std::shared_ptr<SingleAuto> single;
    static std::mutex s_mutex;
}
;



//利用辅助类帮助智能指针释放资源
//将智能指针的析构设置为私有
std::shared_ptr<SingleAuto> SingleAuto::single = nullptr;
std::mutex SingleAuto::s_mutex;
void test_singleauto()
{
    auto sp1 = SingleAuto::GetInst();
    auto sp2 = SingleAuto::GetInst();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    //此时存在隐患，可以手动删除裸指针，造成崩溃
    // delete sp1.get();
}

//防止手贱
//为了规避用户手动释放内存，可以提供一个辅助类帮忙回收内存
//并将单例类的析构函数写为私有
class SingleAutoSafe;
class SafeDeletor//辅助删除器
{
public:
    void operator()(SingleAutoSafe* sf)//仿函数功能
    {
        std::cout << "this is safe deleter operator()" << std::endl;
        delete sf;
    }
};
class SingleAutoSafe
{
private:
    SingleAutoSafe() {}
    ~SingleAutoSafe()
    {
        std::cout << "this is single auto safe deletor" << std::endl;
    }
    SingleAutoSafe(const SingleAutoSafe&) = delete;
    SingleAutoSafe& operator=(const SingleAutoSafe&) = delete;
    //定义友元类，通过友元类调用该类析构函数
    friend class SafeDeletor;
public:
    static std::shared_ptr<SingleAutoSafe> GetInst()
    {
        //1处
        if (single != nullptr)
        {
            return single;
        }
        s_mutex.lock();
        //2处
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        //额外指定删除器  
        //3 处
        single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDeletor());
        //第二个参数表示将SafeDeletor指定为shared_ptr的析构函数
        //也可以指定删除函数
        // single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDelFunc);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleAutoSafe> single;
    static std::mutex s_mutex;
};
//但是上面的代码存在危险，比如懒汉式的使用方式，当多个线程调用单例时，有一个线程加锁进入3处的逻辑。
//其他的线程有的在1处，判断指针非空则跳过初始化直接使用单例的内存会存在问题。
//主要原因在于SingleAutoSafe* temp = new SingleAutoSafe() 这个操作是由三部分组成的
//1 调用allocate开辟内存
//2 调用construct执行SingleAutoSafe的构造函数
//3 调用赋值操作将地址赋值给temp
//    new并不是原子操作
//而现实中2和3的步骤可能颠倒，所以有可能在一些编译器中通过优化是1，3，2的调用顺序，
//其他线程取到的指针就是非空，还没来的及调用构造函数就交给外部使用造成不可预知错误。
//为解决这个问题，C++11 推出了std::call_once函数保证多个线程只执行一次

class SingletonOnce {
private:
    SingletonOnce() = default;
    SingletonOnce(const SingletonOnce&) = delete;
    SingletonOnce& operator = (const SingletonOnce& st) = delete;
    static std::shared_ptr<SingletonOnce> _instance;
public:
    static std::shared_ptr<SingletonOnce> GetInstance() {
        static std::once_flag s_flag;//原来为false，当有线程调用之后变成true
        std::call_once(s_flag, [&]() {//内部会有一把锁，来检测有没有线程初始化过
            _instance = std::shared_ptr<SingletonOnce>(new SingletonOnce);
            });
        return _instance;
    }
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }
    ~SingletonOnce() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};
std::shared_ptr<SingletonOnce> SingletonOnce::_instance = nullptr;
void TestSingle__callonce() {
    std::thread t1([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SingletonOnce::GetInstance()->PrintAddress();
        });
    std::thread t2([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SingletonOnce::GetInstance()->PrintAddress();
        });
    t1.join();
    t2.join();
}

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>& st) = delete;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
            });
        return _instance;
    }
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }
    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
//通过继承实现单例模式，通过模板的方式
class LogicSystem :public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem() {}
private:
    LogicSystem() {}
};

int main()
{
    test_single2hungry();
}

