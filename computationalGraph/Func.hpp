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

#ifndef FUNC_HPP_
#define FUNC_HPP_

#include <utility>
#include <memory>
#include <vector>

#include "ExprAST.hpp"
#include "Expr.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

class Var;
class Execution;

class Func {
 public:
    std::vector<double> argumentsBuffer;
    llvm::ExecutionEngine *engineBuilder;

 public:

    void prepare(int count);
    // explicit Func(std::unique_ptr<llvm::Module>);
    // Expr& operator()(std::vector<Var>) {
    //     return expr;
    // }

    double operator()(std::vector<double>);

    template <typename... Args>
    double operator() (Args&&... args) {
        std::vector<double> collected_args{std::forward<Args>(args)...};
        return this->operator()(collected_args);
    }

    // void set_arguments(std::vector<double>);

    // template <typename... Args>void set_arguments(Args&&... args) {
    //     std::vector<double> collected_args{std::forward<Args>(args)...};
    //     this->set_arguments(collected_args);
    // }
};

#endif  // FUNC_HPP_
