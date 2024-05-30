#include <stdio.h>

int main()
{
    int n = 4;
    int a = 0;
    int b = 1;

    for (int i = 2; i <= n; i++)
    {
        int temp = a + b;
        a = b;
        b = temp;
    }

    printf("The 4th Fibonacci number is: %d\n", b);

    return 0;
}