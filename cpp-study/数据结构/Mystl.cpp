#include <iostream>
using namespace std;
#define max 999
class node 
{
public:
	int data;
	node* next;
	node() :data(0), next(NULL) {};
};
class My_Vector 
{
public:
	My_Vector();
	node *root=new node;
	bool Add(int x);
	bool Delete(int x);
	void Display();
	int Size();
	int Search(int x);
	bool Change(int x, int y);
};

My_Vector::My_Vector() 
{
	root->data = -1;
}
void My_Vector::Display() 
{
	node* p = root;
	while (p!= nullptr) 
	{
		cout << p->data << "->";
		p = p->next;
	}
	cout << endl;
}
bool My_Vector::Add(int x) //返回值为添加成功
{
	if (root->next == NULL) 
	{
		node* p = new node;
		p->data = x;
		root->next = p;
	}
	else 
	{
		node* p = root;
		while (p->next != NULL)
			p = p->next;

		p->next = new node;
		p->next->data = x;
	}
	return true;
};
bool My_Vector::Delete(int x) 
{
	node* p = root;
	if (x == -1)
	{
		cout << "error" << endl;
		return false;//根节点不能删
	}

	while (p->next !=nullptr) 
	{
		if (p->next->data == x) 
		{
			p->next = p->next->next;
			return true;
		}
		p = p->next;
	}
	cout << "error"<<endl;
	return false;

}
int My_Vector::Size() 
{
	//包括根节点本来就占用一个位置
	node* p = root;
	int sum = 0;
	while (p != nullptr)
	{
		p = p->next;
		sum++;
	}
	return sum;
}
int My_Vector::Search(int x) 
{
	//返回最早出现的元素
	node* p = root;
	int pos=0;
	int flag = -1;
	if (root->data == x)
		return pos;

	while (p->next != NULL)
	{
		p = p->next;
		pos++;
		if (p->data == x)
			return pos;
	}
	cout <<"error"<<endl;
	return flag;//如果不存在则返回-1
}
bool My_Vector::Change(int x, int y)
{
	node* p = root;
	while (p != nullptr) 
	{
		if (p->data == x)
		{
			p->data = y;
			return true;
		}
		p = p->next;
	}
	cout << "Can't find the elements" << endl;
}
int main()
{
	My_Vector v;
	v.Add(3);
	v.Add(2);
	v.Add(9);
	v.Add(1);
	v.Change(9,2);
	v.Display();
	v.Delete(8);
	v.Display();
	cout <<endl<< v.Size();
}
