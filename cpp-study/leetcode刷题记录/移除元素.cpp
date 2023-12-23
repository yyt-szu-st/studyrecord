#include<iostream>
#include<vector>
using namespace std;

class Solution {
public:
    //˫ָ��˼·��fast_ptr and slow_ptr
    //ǰ��̽·��������
    int removeElement(vector<int>& nums, int val) {
        int length = nums.size();
        int fast=0, slow=0;

        for (int fast = 0; fast < length; fast++) 
            if (nums[fast] != val) 
                nums[slow++] = nums[fast];           
        return slow;
    }
};
