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

#ifndef EXPRAST_HPP_
#define EXPRAST_HPP_

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

#include "Expr.hpp"

class IRVisitor;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
 public:
    ExprAST() = default;
    virtual ~ExprAST() = default;
    virtual void dump(int level = 0) = 0;
    virtual llvm::Value* accept(IRVisitor* builder) = 0;
};

/// VarExprAST - Expression class for referencing a Var, like "a".
class VarExprAST : public ExprAST {
    std::string name;

 public:
    VarExprAST(std::string name) : name(name) {}
    // ~VarExprAST() { std::cout << "VarExprAST is deleted." << std::endl; }
    void dump(int level = 0) override;
    llvm::Value* accept(IRVisitor* visitor) override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char op;
    Expr lhs;
    Expr rhs;

 public:
    BinaryExprAST(char operation, Expr a, Expr b);
    // ~BinaryExprAST() { std::cout << "BinaryExprAST is deleted." << std::endl; }
    void dump(int level = 0) override;
    llvm::Value* accept(IRVisitor* builder) override;
};

#endif  // EXPRAST_HPP_
