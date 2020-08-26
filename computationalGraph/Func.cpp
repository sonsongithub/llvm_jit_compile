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
#include <string>

#include "Func.hpp"
#include "Var.hpp"

Func::Func() {
    executionEngine = NULL;
}

Func::~Func() {
    delete executionEngine;
}

double Func::operator()(std::vector<double> arg) {
    for (int i = 0; i < arg.size(); i++) {
        argumentsBuffer[i] = arg[i];
    }
    auto f = reinterpret_cast<double(*)()>(executionEngine->getFunctionAddress("caller"));
    return f();
}

void Func::set_arguments(std::vector<Var> arg) {
    argumentPlacefolders.clear();
    argumentPlacefolders = arg;
    argumentsBuffer.clear();
    for (int i = 0; i < argumentPlacefolders.size(); i++) {
        argumentsBuffer.push_back(static_cast<double>(0));
    }
}

void Func::realise() {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::BasicBlock;
    using llvm::Type;

    IRVisitor* visitor = new IRVisitor();
    llvm::Function* callee = visitor->create_callee(argumentPlacefolders, "callee", expr);
    llvm::Function* caller = visitor->create_caller(callee, argumentsBuffer, "caller");
    executionEngine = visitor->create_engine();

    delete visitor;
}