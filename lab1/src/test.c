#include <stdio.h>
#include <stdlib.h>

int sum(int a, int b) {
    int res;
    res = a + b;
    return res;
}

int main() {
    int n = 3;
    int *arr = (int*)malloc(n * sizeof(int));
    int a = 5;
    int b = 1;
    int c = 6;

    if (rand() > 0) {
        a += b;
    } else {
        a -= c;
    }
    printf("a = %d, b = %d, sum: %d\n", a, b, sum(a,b));
    arr[0] = sum(a,b);
    int z = arr[0];
    printf("z = %d\n", z);
    free(arr);
    return 0;
}
