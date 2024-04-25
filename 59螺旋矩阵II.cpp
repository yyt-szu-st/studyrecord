#include<iostream>
#include<vector>
using namespace std;
class Solution {
public:
    vector<vector<int>> generateMatrix(int n) {
        vector<vector<int>> vec(n,vector<int>(n,0));
        int r=0, l=0;
        int up=0, down=n-1, left=0, right=n-1;
        for (int i = 1; i <= n*n; i++)
        {
            vec[r][l] = i;
            if (r == up && l<right) 
            {
                l++;
                if (l == right) 
                {
                    up++;
                }
            }
            else if(l==right&&r<down)
            {
                r++;
                if(r==down)
                {
                    right--;
                }
                
            }
            else if (r == down && l>left) 
            {
                l--;
                if (l == left) 
                {
                    down--;
                }    
            }
            else if ( l == left&&r>up) 
            {
                r--;
                if (r == up) 
                {
                    left++;
                }
          
            }
        }
        return vec;
    }
};

int main()
{
    Solution s;
    s.generateMatrix(3);
}