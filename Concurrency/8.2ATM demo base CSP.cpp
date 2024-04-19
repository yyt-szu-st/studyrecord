#include <iostream>
#include<vector>
#include<thread>
#include<condition_variable>
using namespace std;
class Usr 
{
private:
	int balance;
	string name;
	string passward;
public:
	Usr(int b, string n, string p) 
	{
		balance = b;
		name = n;
		passward = p;
	}
	int get_balance() 
	{
		return balance;
	}
	string get_name() 
	{
		return name;
	}
	string get_pas() 
	{
		return passward;
	}
	void add(int b) 
	{
		balance += b;
	}
	void cut(int b) 
	{
		balance -= b;
	}
};
class atm 
{
private:
	vector<Usr> user;
	bool stop_listen;
	condition_variable lis;
	mutex mtx_;
	int balance;
public:
	atm() 
	{
		user.push_back(Usr(1314, "yyt", "NnNn45672960"));
		user.push_back(Usr(7778, "ymm", "1310189481"));
		stop_listen = false;
	}
	void run() 
	{
		cout << "插卡请按N" << endl;
		thread Listen(Lis);
		Listen.join();

		thread get_money(Get);
		get_money.join();
	}

	void Lis() 
	{
		string a;
		while (cin >> a) 
		{
			if (a == "N") 
			{
				break;
			}
		}
		cout << "请输入账户" << endl;
		string name;
		cin >> name;
		int flag = 0;
		Usr* temp;
		for (auto u : user)
		{
			if (name == u.get_name()) 
			{
				flag = 1;
				temp = &u;
				break;
			}
		}

		string p;
		string n;

		if (flag == 1) 
		{
			cout << "请输入密码" << endl;
			cin >> p;
			if (p == temp->get_pas()) 
			{
				cout << "您的帐户目前余额为:" << temp->get_balance() << endl;
				cout << "请选择你的操作:" << endl << "存款请按1" << endl << "取款请按2" << endl;
				cin >> n;
			}
			else 
			{
				cout << "密码错误，正在退卡" << endl;
				return;
			}
		}


		if (n == "1") 
		{
			cout << "请输入要存入的数量" << endl;
			int t;
			cin >> t;
			temp->add(t);
			cout << "存款成功，您目前的余额为：" << temp->get_balance() << endl;

		}
		else if (n == "2") 
		{
			cout << "请输入要取的数量" << endl;
			int t;
			cin >> t;
			stop_listen = true;
			lis.notify_one();
			temp->cut(t);
			cout << "取款成功，您目前的余额为： " << temp->get_balance() << endl;
			return;
		}
		else 
		{
			cout << "输入未知指令" << endl;
			return;
		}
	}

	void Get(int t) 
	{
		for (;;) 
		{
			std::unique_lock<std::mutex> lock(mtx_);
			lis.wait(lock);
			balance -= t;
		}
	}
};

class Channel 
{

};
int main()
{
}