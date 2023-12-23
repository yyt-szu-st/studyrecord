#include<iostream>
#include<algorithm>
#include<queue>
using namespace std;

class TreeNode
{
public:
	char data;
	TreeNode* left;
	TreeNode* right;
	int leaf;
	TreeNode();
	~TreeNode();
};
static int i = 0;
TreeNode::TreeNode() { leaf = 0; };
TreeNode::~TreeNode() { delete left; delete right; }
static queue<TreeNode*> q;
void pre(TreeNode* tree)
{
	if (tree)
	{
		pre(tree->left);
		pre(tree->right);
	}
}
void in(TreeNode* tree)
{
	if (tree)
	{
		in(tree->left);
		in(tree->right);

	}
}
void post(TreeNode* tree)
{
	if (tree)
	{
		post(tree->left);
		post(tree->right);
		cout << tree->data;
	}
}
void levelOrder(TreeNode* p)
{
	q.push(p);
	while (!q.empty())
	{
		TreeNode* node = q.front();
		q.pop();
		cout << node->data;
		if (node->left != nullptr) q.push(node->left);
		if (node->right != nullptr) q.push(node->right);
	}
}
int tree_height(TreeNode* p)
{
	if (p == NULL) return 0;
	int m, n;

	m = tree_height(p->left);
	n = tree_height(p->right);
	if (m > n) return m + 1;
	else return n + 1;
}
TreeNode* create_tree(string str)
{
	TreeNode* p = new TreeNode;
	if (str[i] == '0') p = NULL;
	else
	{
		p->data = str[i];
		i++;
		p->left = create_tree(str);
		i++;
		p->right = create_tree(str);
	}
	return p;
};
TreeNode* judge_tree(TreeNode* p)
{
	if (p != NULL && p->left == NULL && p->right == NULL)
	{
		p->leaf = 1;
		cout << p->data << " ";
	}//表示该节点为叶子
	else if (p != NULL) {
		p->left = judge_tree(p->left);
		p->right = judge_tree(p->right);
	}
	return p;
}
void array_pre(int* p, int z, int n)
{
	if (z < n && p[z] != 0)
	{
		cout << p[z] << " ";
		array_pre(p, 2 * z + 1, n);
		array_pre(p, 2 * z + 2, n);
	}
}
int main()
{
	int T;
	cin >> T;
	while (T--)
	{
		int n;
		cin >> n;
		int* arr = new int[n];
		for (int j = 0; j < n; j++)
		{
			int num;
			cin >> num;
			arr[j] = num;
		}
		int z = 0;
		array_pre(arr, z, n);
		cout << endl;
		delete[] arr;
	}
	return 0;
}