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
            int middle = left - (left - right) / 2;//��ֹԽ��
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

//��������

//ȷ��while()����left<right����left<=right
//����ұ�д��,[left,right]