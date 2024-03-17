#include<iostream>
#include<vector>
#include<queue>
using namespace std;
class Solution {
public:
    int minStoneSum(vector<int>& piles, int k) {
        //���ȶ���+̰�ķ�
        priority_queue<int> pq(piles.begin(), piles.end());
        for (int i = 0; i < k; i++) {
            int pile = pq.top();
            pq.pop();
            pile -= pile / 2;
            pq.push(pile);
        }
        int sum = 0;
        while (!pq.empty()) {
            sum += pq.top();
            pq.pop();
        }
        return sum;
    }
};

int main() 
{
}
