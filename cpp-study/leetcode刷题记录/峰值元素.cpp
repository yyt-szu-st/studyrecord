#include<iostream>
#include<vector>
using namespace std;
class Solution1 {
public:
    int findPeakElement(vector<int>& nums) {
        //����ѭ����,ֱ���ҵ����ֵ
        int length = nums.size();
        int max = -2147483648;
        int flag;
        for (int i = 0; i < length; i++)
            if (nums[i] > max)
            {
                max = nums[i];
                flag = i;
            }

        return flag;
        return 0;
    }
};

class Solution {
public:
    int findPeakElement(vector<int>& nums) {
        int n = nums.size();

        // ���������������±� i������һ����Ԫ�� (0/1, nums[i])
        // ���㴦�� nums[-1] �Լ� nums[n] �ı߽����
        auto get = [&](int i) -> pair<int, int> {
            if (i == -1 || i == n) {
                return { 0, 0 };
            }
            return { 1, nums[i] };
            };

        int left = 0, right = n - 1, ans = -1;
        while (left <= right) {
            int mid = (left + right) / 2;
            if (get(mid - 1) < get(mid) && get(mid) > get(mid + 1)) {
                ans = mid;
                break;
            }
            if (get(mid) < get(mid + 1)) {
                left = mid + 1;
            }
            else {
                right = mid - 1;
            }
        }
        return ans;
    }
};
