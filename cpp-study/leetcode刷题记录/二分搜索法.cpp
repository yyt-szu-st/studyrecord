#include<iostream>
#include<vector>
using namespace std;

class Solution {
public:

    int search(vector<int>& nums, int target) {
        int left = 0;
        int right = nums.size();
        while (left<right) 
        {
            int middle = left - (left - right) / 2;//防止越界
            if (nums[middle] > target) 
            {
                right = middle;
            }
            else if (nums[middle] < target) 
            {
                left = middle+1;
            }
            if (nums[middle] = target)
                return middle;
        }

        return -1;
    }
};

//二分搜索

//确定while()内是left<right还是left<=right
//左闭右闭写法,[left,right]