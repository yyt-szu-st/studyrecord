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
    //�����ʵ�ֶ��̵߳Ľṹ����Ҫ�ѿ����͸�ֵȫ��ɾ������Ȼ�����ڹ���
    lock_free_stack(const lock_free_stack&) = delete;
    lock_free_stack& operator = (const lock_free_stack&) = delete;

    std::atomic<node*> head;//ԭ�ӱ�����Ϊͷ�ڵ�
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
        //�Ƚ�head��new_node.next�ģ����һ������head����Ϊnew_node��ͬʱ����true
        //compare_exchange_weak�Ŀ������Сһ��
    }
    //��ͬ˼·ʵ��pop
    void pop(T& value) 
    {
        node* old_head;
        do 
        {
            old_head = head.load();
        } while (!head.compare_exchange_strong(old_head,old_head->next));
        value = old_head->data;
        //������������쳣�������ƻ���ջ�Ľṹ�����������ݶ�ʧ
        //����ʹ��shared_ptrʵ��
    }

    //������������������
    //1.δ�ж��Ƿ�Ϊ��ջ
    //2.������ֵ���ܻ�����쳣
    //3.δ�ͷŵ����ڵ���ڴ�
    //����ָ��汾
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
        //�Ƚϲ�ǰ��
        
        //ʵ�ּ򵥵Ļ����߼�
        shared_ptr<T> res;
        res.swap(old_node->data);
        delete old_node;//������Դ
        //�����ǻ�����Դ�������߳�2���ʹ����delete���պ��߳�1����ʹ����Դ
        //�ͻᵼ���߳�һ���ִ���
        return res;//data�������һ������ָ��
    }

    //pop�����汾���ӳ�ɾ���ڵ�
    shared_ptr<T> pop_best() 
    {
        //�ȼ�������������ִ����������
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
        //�Ƚϲ�ǰ��

        //ʵ�ּ򵥵Ļ����߼�
        shared_ptr<T> res;
        if (old_node) 
        {
            res.swap(old_node->data);
        }
        //�����ͷ�
        try_reclaim(old_node);
        return res;//data�������һ������ָ��
    }

    void try_reclaim(node* old_head)
    {
        //ԭ�ӱ����жϽ���һ���߳̽���
        if (threads_in_pop == 1)
        {
            //��ǰ�̰߳Ѵ�ɾ�б�ȥ��
            node* nodes_to_delete = to_be_deleted.exchange(nullptr);
            //����ԭ�ӱ�����ȡ׼ȷ״̬
            if (!--threads_in_pop) //�ٽ���һ���ж�
            {
                //���Ψһ�����򽫴�ɾ�б�ɾ��
                delete_nodes(nodes_to_delete);
            }
            else if (nodes_to_delete)
            {
                //5������������̵߳��ã��Ҵ�ɾ�б�Ϊ��
                //�򽫴�ɾ�б��׽ڵ���¸�to_be_delete
                chain_pending_node(nodes_to_delete);
            }
            delete old_head;
        }
        else
        {
            //����߳̾���head�ڵ㣬��ʱ����ɾ��old_head
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
    void chain_pending_node(node* n)//�����ɾ���� 
    {
        node* last = n;
        //����nextָ��ǰ��������ĩβ
        while ( node* const next=last->next)
        {
            last = next;
        }
        chian_pending_nodes(n,last);
    }
    void chian_pending_nodes(node* first, node* last) 
    {
        //�Ƚ�last�õ�next�Ľڵ����Ϊ��ɾ�б���׽ڵ�
        last->next = to_be_deleted;
        //��ѭ����֤last->ָ����ȷ
        //����ɾ�б���׽�����Ϊfirst�ڵ�
        while (!to_be_deleted.compare_exchange_weak(last->next, first));

    }


};