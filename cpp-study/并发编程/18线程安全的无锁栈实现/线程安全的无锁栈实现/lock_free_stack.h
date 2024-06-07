#pragma once
#include<iostream>
#include<atomic>
using namespace std;
template<typename T>
class lock_free_stack
{
private:
    struct node
    {
        shared_ptr<T> data;
        node* next;
        node(const T & data_):data(make_shared<T>(data_)) {};
    };
    //如果想实现多线程的结构，都要把拷贝和赋值全都删掉，不然不利于管理
    lock_free_stack(const lock_free_stack&) = delete;
    lock_free_stack& operator = (const lock_free_stack&) = delete;

    std::atomic<node*> head;//原子变量作为头节点
    std::atomic<node*> to_be_deleted;
    std::atomic<int> threads_in_pop;

public:
    lock_free_stack() {};
    void push(const T& value) 
    {
        auto new_node = new node(value);
        do 
        {
            new_node->next = head.load();
        } while (!head.compare_exchange_strong(new_node->next, new_node));
        //比较head和new_node.next的，如果一样，将head设置为new_node，同时返回true
        //compare_exchange_weak的开销会更小一点
    }
    //相同思路实现pop
    void pop(T& value) 
    {
        node* old_head;
        do 
        {
            old_head = head.load();
        } while (!head.compare_exchange_strong(old_head,old_head->next));
        value = old_head->data;
        //如果拷贝发生异常，不仅破坏了栈的结构，还导致数据丢失
        //所以使用shared_ptr实现
    }

    //上面代码存在三个问题
    //1.未判断是否为空栈
    //2.拷贝赋值可能会存在异常
    //3.未释放弹出节点的内存
    //智能指针版本
    shared_ptr<T> pop() 
    {
       
        node* old_node = nullptr;
        do 
        {
            old_node = head.load();
            if (old_node == nullptr)
            {
                return nullptr;
            }
        } while (!head.compare_exchange_strong(old_node,old_node->next));
        //比较并前进
        
        //实现简单的回收逻辑
        shared_ptr<T> res;
        res.swap(old_node->data);
        delete old_node;//回收资源
        //本意是回收资源，但在线程2如果使用了delete，刚好线程1还在使用资源
        //就会导致线程一出现错误
        return res;//data本身就是一个智能指针
    }

    //pop升级版本，延迟删除节点
    shared_ptr<T> pop_best() 
    {
        //先计数器自增，再执行其他操作
        ++threads_in_pop;
        node* old_node = nullptr;
        do
        {
            old_node = head.load();
            if (old_node == nullptr)
            {
                --threads_in_pop;
                return nullptr;
            }
        } while (!head.compare_exchange_strong(old_node, old_node->next));
        //比较并前进

        //实现简单的回收逻辑
        shared_ptr<T> res;
        if (old_node) 
        {
            res.swap(old_node->data);
        }
        //尝试释放
        try_reclaim(old_node);
        return res;//data本身就是一个智能指针
    }

    void try_reclaim(node* old_head)
    {
        //原子变量判断仅有一个线程进入
        if (threads_in_pop == 1)
        {
            //当前线程把待删列表去除
            node* nodes_to_delete = to_be_deleted.exchange(nullptr);
            //更新原子变量获取准确状态
            if (!--threads_in_pop) //再进行一次判断
            {
                //如果唯一调用则将待删列表删除
                delete_nodes(nodes_to_delete);
            }
            else if (nodes_to_delete)
            {
                //5如果还有其他线程调用，且待删列表不为空
                //则将待删列表首节点更新给to_be_delete
                chain_pending_node(nodes_to_delete);
            }
            delete old_head;
        }
        else
        {
            //多个线程竞争head节点，此时不能删除old_head
            chain_pending_node(old_head);
            threads_in_pop--;
        }
    }
       
    void delete_nodes(node* nodes) 
    {
        while (nodes) 
        {
            node* next = nodes->next;
            delete nodes;
            nodes = next;
        }
    }
    void chain_pending_node(node* n)//加入待删函数 
    {
        node* last = n;
        //沿着next指针前进到链表末尾
        while ( node* const next=last->next)
        {
            last = next;
        }
        chian_pending_nodes(n,last);
    }
    void chian_pending_nodes(node* first, node* last) 
    {
        //先将last得到next的节点更新为待删列表的首节点
        last->next = to_be_deleted;
        //借循环保证last->指向正确
        //将待删列表的首届点更新为first节点
        while (!to_be_deleted.compare_exchange_weak(last->next, first));

    }


};