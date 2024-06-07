#include <iostream>
#include<thread>
void test(int n)
{
    std::cout << n<<std::endl;
}
class background_task 
{
public:
    void operator()(){ //通过重载括号运算符实现仿函数
        std::cout << "background_task" << std::endl;
    }
};

struct func 
{
    int& ii;//此为一个引用，当外部变量去销毁时会导致引用出错
    func(int& i) :ii(i) {};
    void operator()() {//实现仿函数功能
        for (int i = 0; i < 3; i++) {
            ii = i;
            std::cout << "ii is" <<ii<< std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
void oops() 
{
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread funthread(myfunc);
    //隐患，访问局部变量，局部变量可能会随着}结束而回收或随着主线程的退出而回收
    funthread.detach();
}//}会调用析构函数,时机不是由程序员决定，而是由编译器决定

void use_join() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread(myfunc);
    functhread.join();
}
//use_join();

void catch_exception()//防止主线程在运行的时候出现问题导致直接回收所有资源 
{
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread(myfunc);
    try{
        //假设主线程出现了异常，被捕获到
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (std::exception& e) {
        functhread.join();//当主线程出现异常时，会等待子线程执行完毕
        throw;
    }

    functhread.join();
}

class thread_guard {//守护线程
private:
    std::thread& _t;
public:
    explicit thread_guard(std::thread& t) :_t(t) {}
    ~thread_guard() {//在该类析构之前将函数运行
        //join只能调用一次
        if (_t.joinable()) {
            _t.join();
        }
    }

    //禁用该类的赋值构造函数和赋值运算符
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};
void auto_guard(){
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);//等待子线程推出
    //本线程做一些事情
    std::cout << "auto guard finished " << std::endl;
}

void print_str(int i, std::string const& s) 
{
    std::cout << "i is " << i << " str is " << s << std::endl;
}
void danger_oops(int som_param) {   //c++中会有隐式转化将char*转换为std::string.
    //在线程调用上会引发崩溃
    char buffer[1024];
    sprintf_s(buffer, "%i", som_param);
    std::thread t(print_str, som_param, buffer);
    t.detach();
    std::cout << "danger oops finished " << std::endl;
}
void safe_oops(int som_param){
    char buffer[1024];
    sprintf_s(buffer, "%i", som_param);
    std::thread t(print_str, som_param,std::string(buffer));
    //解决方法：将隐式转换变为显式转换
    t.detach();
    std::cout << "safe oops finished " << std::endl;
}

void change_param(int& param) {
    param++;
}
void ref_oops(int some_param) {
    std::cout << "before change , param is " << some_param << std::endl;
    //需使用引用显示转换
    std::thread t2(change_param, std::ref(some_param));//使用thread构造函数的时候，都会把参数
    //变成一个右值储存起来
    t2.join();
    std::cout << "after change , param is " << some_param << std::endl;
}

//绑定类成员函数
class X {
public:
    void do_lengthy_work() {
        std::cout << "do lengthy work " << std::endl;
    }
};
void bind_class_oops(){
    X my_x;
    std::thread t(&X::do_lengthy_work, &my_x);
    t.join();
}

void deal_unique(std::unique_ptr<int> p) 
{
    std::cout << "unique ptr data is " << *p << std::endl;
    (*p)++;

    std::cout << "after unique ptr data is " << *p << std::endl;
}
int main()
{
    int n = 3;
    std::thread t1(test,n);//第一个参数为回调函数地址，第二个为参数
    t1.join();

    //std::thread t21(background_task());//编译器会把t2当成一个函数对象
    //t2.join();//报错

    //解决方式，将最外层括号转为花括号
    std::thread t22{ background_task() };
    t22.join();

    //线程detach
    //线程允许采用分离的方式在后台独自运行，称其为守护线程
    //当某个线程还在运行的时候，若它的局部变量被释放，容易出现崩溃
    //oops();

    //解决方法一
    //防止主线程退出过快，需要停顿一下，让子线程跑完detach
    std::this_thread::sleep_for(std::chrono::seconds(1));

    //解决方法二
    //使用try/catch保证等待执行线程完毕再结束主线程
    try 
    {
        oops();
    }
    catch(...)
    {
    }

    //解决方法三
    //通过智能指针传递参数，因为引用计数会随着赋值增加，
    // 可保证局部变量在使用期间不被释放，这也就是我们之前提到的伪闭包策略。

    //解决方法四
    //将局部变量的值作为参数传递，这么做需要局部变量有拷贝和复制的功能
    //而且拷贝消耗空间和效率

    //解决方法五
    //将线程运行的方式修改为join(代码line37)，这样能保证局部变量被释放前线程已经运行结束。
    // 但是这么做可能会影响运行逻辑

    //设置自动守卫线程，保证其运行完
    //auto_guard();

    danger_oops(3);//尽量使用显式转换而不是隐式转换
    safe_oops(3);

    ref_oops(3);//显式转换类型避免右值引用引发错误

    bind_class_oops();
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
