#include<iostream>
#include<vector>
using namespace std;
class Solution {
public:
    int minSubArrayLen(int target, vector<int>& nums) {
        int left, right;
        left = right = 0;
        int min = 100001;
        int sum = 0;
        while(right!=nums.size()){
            sum += nums[right];
            while (sum >= target) {
                
                sum -= nums[left];
                if (min > right - left) {
                    min = right - left;
                }
                left++;
            }
            right++;
        }
        if (min == 100001)
            return 0;
        else return min+1;
    }
};   