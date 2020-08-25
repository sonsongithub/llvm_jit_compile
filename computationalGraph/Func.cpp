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

#include <algorithm>

#include "Func.hpp"
#include "Var.hpp"
#include "Execution.hpp"


Func::~Func() {
    delete executionEngine;
}

void Func::prepare(int count) {
    argumentsBuffer.clear();
    for (int i = 0; i < count; i++) {
        argumentsBuffer.push_back(static_cast<double>(i + 1));
    }
}

double Func::operator()(std::vector<double> arg) {
    for (int i = 0; i < arg.size(); i++) {
        argumentsBuffer[i] = arg[i];
    }
    auto f = reinterpret_cast<double(*)()>(executionEngine->getFunctionAddress("caller"));
    return f();
}

// Func::Func(std::unique_ptr<llvm::Module> module) {
//     // Builder JIT
//     execution = new Execution(std::move(module), "hoge");
// }

// double Func::set_arguments(std::vector<Var> vars) {
//     std::cout << "set_arguments" << std::endl;
//     std::copy(vars.begin(), vars.end(), std::back_inserter(arguments));
// }
