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

#ifndef IRVISITOR_HPP_
#define IRVISITOR_HPP_

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <utility>

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"

class Execution;
struct Expr;
class Var;
class Func;

class IRVisitor {
 public:
    llvm::IRBuilder<> *builder;
    std::map<std::string, llvm::Value*> name2Value;
    std::unique_ptr<llvm::Module> module;
 public:
    IRVisitor();
    ~IRVisitor();
    llvm::Value* visit(Expr expr);
    llvm::ExecutionEngine *create_engine();
    llvm::Value* createValue(double value);
    llvm::LLVMContext* context();
    llvm::Function* create_callee(const std::vector<Var> &argumentPlacefolders, std::string name, Expr expr);
    llvm::Function* create_caller(llvm::Function *callee, const std::vector<double> &arguments, std::string name);
};

#endif  // IRVISITOR_HPP_
