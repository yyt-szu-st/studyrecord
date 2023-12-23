#include<iostream>
#include<vector>
#include<math.h>
using namespace std;

class Solution {
public:
    //双指针思路，左闭右开原则
    vector<int> sortedSquares(vector<int>& nums) {
        int k = nums.size() - 1;
        int i, j;
        int length = nums.size();
        vector<int>res(length,0);
        for (i = 0, j = length - 1; i <= j;) 
        {
            if (nums[i] * nums[i] > nums[j] * nums[j])
                res[k--] = nums[i++];
            else if (nums[i] * nums[i] < nums[j] * nums[j])
                res[k--] = nums[j++];
            else
                res[k--] = nums[i++];
        }
        return res;
    }
};

int main() 
{
    cout << -1 * -1;
}
