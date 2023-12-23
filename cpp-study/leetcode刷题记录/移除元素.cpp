#include<iostream>
#include<vector>
using namespace std;

class Solution {
public:
    //双指针思路，fast_ptr and slow_ptr
    //前方探路，后方整顿
    int removeElement(vector<int>& nums, int val) {
        int length = nums.size();
        int fast=0, slow=0;

        for (int fast = 0; fast < length; fast++) 
            if (nums[fast] != val) 
                nums[slow++] = nums[fast];           
        return slow;
    }
};
