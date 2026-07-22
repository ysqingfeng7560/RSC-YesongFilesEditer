#include <stdio.h>

int main(void) {
    int a, b;
    
    printf("=== GCC 测试程序 ===\n");
    printf("请输入两个整数: ");
    scanf("%d %d", &a, &b);
    
    printf("和: %d + %d = %d\n", a, b, a + b);
    printf("差: %d - %d = %d\n", a, b, a - b);
    printf("积: %d * %d = %d\n", a, b, a * b);
    
    if (b != 0) {
        printf("商: %d / %d = %.2f\n", a, b, (double)a / b);
    } else {
        printf("除数不能为 0\n");
    }
    
    printf("\n=== 测试通过，环境正常 ===\n");
    return 0;
}