#include <stdio.h>
#include <stdlib.h>

struct MyStruct {
    int values[3];
    float coeff;
};

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

    // Структура и её использование
    struct MyStruct ms;
    ms.values[0] = sum(a, b);          // доступ к полю-массиву через точку
    int z1 = ms.values[0];             // считывание через точку

    struct MyStruct *pms = &ms;
    pms->values[1] = sum(a, b);        // доступ через указатель
    int z2 = pms->values[1];           // считывание через указатель

    printf("a = %d, b = %d, z1 = %d, z2 = %d\n", a, b, z1, z2);

    arr[0] = sum(a, b);
    int z = arr[0];
    printf("z = %d\n", z);
    free(arr);
    return 0;
}
