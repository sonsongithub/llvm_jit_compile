
#include <stdarg.h>
#include <iostream>

int sum1(int amount, int k, ...) {
    int i = 0;
    int val = 0;
    int sum = 0;
    va_list vl;
    va_start(vl, k);
    for (i = 0; i < amount; i++) {
        val = va_arg(vl, int);
        sum += val;
    }
    va_end(vl);
    return sum; 
}

int sum2(int amount,...) {
    int i = 0;
    int val = 0;
    int sum = 0;
    va_list vl;
    va_start(vl, amount);
    for (i = 0; i < amount; i++) {
        val = va_arg(vl, int);
        sum += val;
    }
    va_end(vl);
    return sum; 
}

int main() {

    std::cout << sum1(2, 2, 1, 13) << std::endl;
    std::cout << sum2(3, 2, 1, 13) << std::endl;

    return 0;
}