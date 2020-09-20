// MIT License
//
// Copyright (c) 2020 sonson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT W  ARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// This code is to analyze the behavior of LLVM when variable arguments are handled. 
//

#include <stdarg.h>
#include <iostream>

void inspect_va_list(va_list * p) {
    char *pointer = (char*)p;
    printf("00000x%04x\n", *(pointer + 0));       // 0
    printf("00000x%04x\n", *(pointer + 1));       // 8
    printf("%010p\n", *(int*)(pointer + 4));   // 16
    printf("%010p\n", *(int*)(pointer + 6));   // 24
}

int sum_i(int amount, ...) {
    int i = 0;
    int val = 0;
    int sum = 0;

    va_list vl;


    va_start(vl, amount);

    int p_2 = amount;
    int p_3 = 0;
    int p_4;
    int p_5 = 0;

    va_list *p_6 = &vl;

    // label: 9

    // va_list {
    //     int32 e1;
    //     int32 e2;
    //     char *e3;
    //     char *e4;
    // }

    while (1) {
        int p_10 = p_3;
        int p_11 = p_2;

        std::cout << "----------------------------------" << std::endl;
        inspect_va_list(&vl);
        std::cout << "----------------------------------" << std::endl;

        // std::cout << p_10 << std::endl;
        // std::cout << p_11 << std::endl;

        if (p_10 < p_11) {
            // label: 13

            va_list* p_14 = (va_list*)p_6;

            int *p_15 = (int*)(p_14);
            int p_16 = (int)(*p_15);

            int* p_30 = 0;

            std::cout << "p_16=" << p_16 << std::endl;

            // printf("%p\n", p_14);

            if (p_16 <= 40) {
                // label: 18
                // %19 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 3
                // get the pointer to e4 of the va_list.
                // e4 is a pointer to data.
                char **p_19 = (char**)((char*)(p_14) + 4 + 4 + 8);
                char *p_20 = *p_19;

                // This pointer is shifted by 2 (offsets) from p_12.
                char *p_21 = p_20 + p_16;
                int *p_22 = (int*)p_21;

                int p_23 = p_16 + 8;

                // It writes the value of p23 to the address pointed to by p13.
                *p_15 = p_23;

                // phi node
                p_30 = p_22;

            } else {
                // label: 24
                // e3 のポインタ（中身はポインタ）を取得
                char **p_25 = (char**)((char*)(p_14) + 4 + 4);

                char *p_26 = *p_25;
                int *p_27 = (int*)p_26;

                // そのポインタから，８バイト，ポインタをずらす
                char *p_28 = p_26 + 8;

                *p_25 = p_28;

                // phi node
                p_30 = p_27;
            }

            // label: 29
            int p_31 = *p_30;

            p_4 = p_31;
            int p_32 = p_4;
            int p_33 = p_5;
            
            std::cout << p_32 << std::endl;

            int p_34 = p_32 + p_33;

            p_5 = p_34;

            // std::cout << p_5 << std::endl;

        } else {
            va_list* p_39 = (va_list*)p_6;
            char *p_40 = (char*)p_39;

            va_end(vl);

            break;
        }

        // label: 35

        int p_36 = p_3;

        int p_37 = p_36 + 1;

        p_3 = p_37;
    }
    return p_5;
}

int main() {

    // std::cout << sum_i(1, 13) << std::endl;
    std::cout << sum_i(12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) << std::endl;

    std::cout << sizeof(va_list) << std::endl;

    return 0;
}