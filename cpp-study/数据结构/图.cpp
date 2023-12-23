#include <iostream>
#include <queue>
#include<algorithm>
using namespace std;
#define max 20
#define INF 9999999
struct tree 
{
    string start, end;
    int weight;
    bool operator<(const tree& t2) 
    {
        return weight < t2.weight;
    }
};
class map
{
public:
    string start;//书生成开始的顶点
    int *visit;//已经访问过的数组
    int mat[max][max];//储存邻接矩阵
    int dist[max];
    int vexnum;//顶点的数量
    string* vertix;//储存顶点的数量
    int res;//记录最小路径
    int knum;//记录所有的边数
    int p[max];
    tree t1[max];
    tree t2[max];
    void Setmap();
    void DFS(int v);
    void BFS(int v);
    void prim();
    int Out_Degree(int v);
    int In_Degree(int v);
    int search(string a);
    int kruskal();
    int find(int x);
    void display();
};
void map::Setmap()
{
    int i, j;
    cin >> vexnum;
    vertix = new string[vexnum];
    for (i = 0; i < vexnum; i++)
        cin >> vertix[i];
    
    for (i = 0; i < max; i++)
    {
        for (j = 0; j < max; j++)
        {
            mat[i][j] = INF;
        }
        dist[i] = INF;
    }
    string ch1, ch2;
    int k,weight;
    cin >> k;
    knum = k;
    for(int i=0;i<knum;i++)
    {
        cin >> ch1 >> ch2>>weight;
        t1[i].start = ch1; t1[i].end = ch2; t1[i].weight = weight;
        mat[search(ch1)][search(ch2)]=mat[search(ch2)][search(ch1)] = weight;
    }
}
int map::search(string a) 
{
    int i;
    for (i = 0; i < vexnum; i++)
        if (a == vertix[i])
            return i;
}
int map::find(int x)
{
    if (x != p[x])
        p[x] = find(p[x]);
    return p[x];
}

void map::DFS(int v)
{
    visit[v] = true;
    cout << v << " ";//将已经访问到的给标记为true
    int* v1 = new int[vexnum];
    for (int i = 0; i < vexnum; i++)
        v1[i] = -1;
    //v1用来记录与v相连但是没有被访问到的节点
    int k = 0;
    for (int j = 0; j < vexnum; j++)//保持列不变，往下面去找和v联通的
    {
        if (mat[v][j] == 1)
        {
            v1[k] = j;//记录与v相连的顶点
            k++;
        }
    }//找到k的值代表和他相连有几个边
    int i = 0;
    while (k--)//将k递减至直到0，即找完所有的边
    {
        int w = v1[i];
        if (!visit[w])
            DFS(w);
        i++;
    }
    delete[] v1;
}
void map::BFS(int v)
{
    int k, w, u, * v1 = new int[vexnum];
    queue<int>Q;
    for (v = 0; v < vexnum; v++)
        visit[v] = false;

    for (v = 0; v < vexnum; v++)
    {
        if (!visit[v])
        {
            visit[v] = true;
            cout << v << " ";
            Q.push(v);

            while (!Q.empty())
            {
                u = Q.front();
                Q.pop();

                k = 0;
                for (int i = 0; i < vexnum; i++)
                {
                    if (mat[u][i] == 1)
                    {
                        v1[k] = i;
                        k++;
                    }
                }

                int i = 0;
                while (i < k)
                {
                    w = v1[i];
                    if (!visit[w])
                    {
                        visit[w] = true;
                        cout << w << " ";
                        Q.push(w);
                    }
                    i++;
                }
            }
        }
    }
    cout << endl;
    delete[] v1;
}
int map::Out_Degree(int v) 
{
    int count = 0;
    int i;
    for (i = 0; i < vexnum; i++)
        if (mat[v][i] == 1)
            count++;
    //保持列不变，找行是出度；
    return count;
}
int map::In_Degree(int v) 
{
    int count = 0;
    int i;
    for (i = 0; i < vexnum; i++)
        if (mat[i][v] == 1)
            count++;
    return count;
}
void map::prim() 
{

    int min_index;
    int min_dis;
    int weight;
    int* adj = new int[vexnum];
    bool* book = new bool[vexnum];//记录已经访问过的点
    int edge_index = 0;
    tree* result = new tree[vexnum-1];
    min_index = search(start);
    
    for (int i = 0; i < vexnum; i++)
    {
        book[i] = false;
        dist[i] = INF;
    }

    for (int t = 0; t < vexnum-1; t++) 
    {
        book[min_index] = true;//将第一个访问的点标记
        
        //更新dist数组
        for (int i = 0; i < vexnum; i++) 
        {
            if (!book[i] && mat[min_index][i]< dist[i]) //此处为找到了
            {
                dist[i] = mat[min_index][i];//dist，在mat同一列中寻找
                adj[i] = min_index;
            }
        }

        min_dis = INF;//初始化原始距离
        //开始找点
        for (int i = 0; i < vexnum; i++) 
        {
            if (!book[i] && dist[i] < min_dis) 
            {
                min_dis = dist[i];//找到距离最小且未被访问点，并记录dis
                min_index = i;//记录下当前最小的点，并且作为下一个点的起点
            }
        }

        res += dist[min_index];
        result[edge_index].start = vertix[adj[min_index]];
        result[edge_index].end = vertix[min_index];
        result[edge_index].weight = dist[min_index];
        edge_index++;
    }
    
    //输出
    cout << res << endl;
    for (int i = 0; i < vexnum-1; i++)
        cout << result[i].start << " " << result[i].end << " " << result[i].weight << endl;

}

int map::kruskal()
{
    sort(t1, t1 + knum);
    int bshu = 0;
    int i;
    int weight = 0;
    int k = 0;
    for (i = 0; i < vexnum; i++)
        p[i] = i;
    for (i = 0; i < knum; i++) 
    {
        int a = search(t1->start);
        int b = search(t1->end);
        a = find(a);
        b = find(b);
        if (a != b)
        {
            t2[k++] = t1[i];
            if (search(t1[i].start) > search(t1[i].end))
                swap(t2[k - 1].start, t2[k - 1].end);
            p[a] = b;
            bshu++;
            weight += t1[i].weight;
        }
    }
    return weight;
}
void map::display() 
{
    cout << kruskal() << endl;
    for (int i = 0; i < vexnum - 1; i++) 
    {
        cout << t1[i].start << " " << t1[i].end <<" "<< t1[i].weight << endl;
    }
}
int main()
{
    map M;
    M.Setmap();
    M.display();
    return 0;
}