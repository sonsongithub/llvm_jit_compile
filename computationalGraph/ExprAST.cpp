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

#include <utility>

#include "ExprAST.hpp"
#include "IRVisitor.hpp"
#include "VariableParserVisitor.hpp"

void VarExprAST::dump(int level) {
    for (int i = 0; i < level; i++) { std::cout << "-"; }
    std::cout << "VarExprAST" << std::endl;
}

llvm::Value* VarExprAST::accept(IRVisitor* visitor) {
    auto p = visitor->name2Value[name];
    return p;
}

BinaryExprAST::BinaryExprAST(char operation, Expr a, Expr b) {
    lhs = std::move(a);
    rhs = std::move(b);
    op = operation;
}

llvm::Value* BinaryExprAST::accept(IRVisitor* visitor) {
    std::cout << "accept - BinaryExprAST" << std::endl;
    llvm::Value* left = visitor->visit(lhs);
    llvm::Value* right = visitor->visit(rhs);
    switch (op) {
    case '+':
        return visitor->builder->CreateFAdd(left, right, "addtmp");
    case '*':
        return visitor->builder->CreateFMul(left, right, "multmp");
    }
    return nullptr;
}

void BinaryExprAST::dump(int level) {
    for (int i = 0; i < level; i++) { std::cout << "-"; }
    std::cout << "BinaryExprAST" << std::endl;
    lhs.value->dump(level + 1);
    rhs.value->dump(level + 1);
}
