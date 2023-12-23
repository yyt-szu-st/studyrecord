#include<iostream>
#include<string>
using namespace std;

class MyString 
{
private:
	string mainstr;
	int size;

	void GetNext(string p, int next[]);
	int KMPfind(string p, int pos, int next[]);
public:
	MyString();
	~MyString();
	void setVal(string sp);
	int KMPfindSubstr(string p, int pos);
};
MyString::MyString(){};
MyString::~MyString() 
{
	size = 0;
	mainstr = "";
}
void MyString::setVal(string sp) 
{
	mainstr = "";
	mainstr.assign(sp);//重载mainstr
	size = mainstr.length();
}

int MyString::KMPfindSubstr(string p, int pos) 
{
	int i;
	int L = p.length();//子串的长度
	int* next = new int[L];
	GetNext(p, next);//获取next数组的数据
	for (i = 0; i < L; i++)
		cout << next[i] << " ";
	cout << endl;//将匹配的位置打印出来

	int v = -1;
	v = KMPfind(p, pos, next);
	delete []next;
	return v;
}

void MyString::GetNext(string p,int next[])
{
	int j, k;
	j = 0; k = -1;
	next[0] = -1;//第一个字符没有匹配
	while(j < p.length() - 1)//不能越界
	{
		if (k == -1 || p[j] == p[k]) 
		{
			j++; k++;
			next[j] = k;
		}
		else 
		{
			k = next[k];
		}
	}
}
int MyString::KMPfind(string p, int pos, int next[]) {
	int i = pos; // 主字符串的当前位置
	int j = -1;   // 子串的当前位置

	while (i < mainstr.length()) 
	{
		if (j == -1 || mainstr[i] == p[j]) {
			i++;
			j++;
		}

		if (j == p.length()) //字串已经找到底部
		{
			return (i - j); // 返回匹配的起始位置
		}
		else if (i < mainstr.length() && mainstr[i] != p[j]) {
			j = next[j];
		}
	}

	return -1; // 未找到匹配
}


int main() 
{
	int T; cin >> T;
	while (T--) 
	{
		string mainstr, p;
		MyString s;
		cin >> mainstr >> p;
		s.setVal(mainstr);
		int pos = s.KMPfindSubstr(p, 0);
		cout << pos+1 << endl;
	}
}
