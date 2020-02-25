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
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/APFloat.h"

using namespace llvm;

static LLVMContext TheContext;

class ExprAST;

class IRVisitor {
    llvm::IRBuilder<> *builder;
 public:
    IRVisitor();
    ~IRVisitor();
    Value* visit(ExprAST* exp);
};

/// ExprAST - Base class for all expression nodes.
class ExprAST {
 public:
    virtual ~ExprAST() = default;
    virtual void dump() = 0;
    virtual Value* accept(IRVisitor* builder) = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
    double Val;
 public:
    NumberExprAST() : Val(0) {}
    explicit NumberExprAST(double Val) : Val(Val) {}
    void dump() override;
    Value* accept(IRVisitor* builder) override;
};

void NumberExprAST::dump() {
    std::cout << "NumberExprAST" << std::endl;
}

Value* NumberExprAST::accept(IRVisitor* builder) {
    return nullptr;
}

/// VarExprAST - Expression class for referencing a Var, like "a".
class VarExprAST : public ExprAST {
    std::string Name;

 public:
    explicit VarExprAST(const std::string &Name) : Name(Name) {}
    void dump() override;
    Value* accept(IRVisitor* builder) override;
};

void VarExprAST::dump() {
    std::cout << "VarExprAST" << std::endl;
}

Value* VarExprAST::accept(IRVisitor* builder) {
    std::cout << Name << std::endl;
    return nullptr;
}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char Op;
    // std::unique_ptr<ExprAST> LHS, RHS;
    std::unique_ptr<ExprAST> LHS;
    std::unique_ptr<ExprAST> RHS;

 public:
    BinaryExprAST(char _Op, std::unique_ptr<ExprAST> _LHS, std::unique_ptr<ExprAST> _RHS) : Op(_Op), LHS(std::move(_LHS)), RHS(std::move(_RHS))) {}
    void dump() override;
    Value* accept(IRVisitor* visitor) override;
};

void BinaryExprAST::dump() {
    std::cout << "BinaryExprAST" << std::endl;
}

Value* BinaryExprAST::accept(IRVisitor* visitor) {
    return nullptr;
}

class Var {
    // static std::vector<std::string> used_names;
    static int name_count;
    std::string name;
 public:
    Var() {
        name = "var" + std::to_string(name_count++);
    }

    operator std::unique_ptr<ExprAST>() const {
        auto p = std::make_unique<VarExprAST>(name);
        return std::move(p);
    }
    
};

ExprAST* operator+ (VarExprAST a, ExprAST* b) { return (ExprAST*)(new BinaryExprAST('+', &a, b)); }
ExprAST* operator+ (ExprAST* a, VarExprAST b) { return (ExprAST*)(new BinaryExprAST('+', a, &b)); }

ExprAST* operator+ (Var a, ExprAST* b) {
    ExprAST *p = static_cast<ExprAST*>(a);
    return  (ExprAST*)(new BinaryExprAST('+', p, b));
}
ExprAST* operator+ (ExprAST* a, Var b) {
    ExprAST *p = static_cast<ExprAST*>(b);
    return  (ExprAST*)(new BinaryExprAST('+', a, p));
}

ExprAST* operator+ (VarExprAST a, VarExprAST b) { return (ExprAST*)(new BinaryExprAST('+', &a, &b)); }
std::unique_ptr<ExprAST> operator+ (Var a, Var b) {
    std::unique_ptr<ExprAST> p_a = static_cast<std::unique_ptr<ExprAST>>(a);
    std::unique_ptr<ExprAST> p_b = static_cast<std::unique_ptr<ExprAST>>(b);
    return  (ExprAST*)(new BinaryExprAST('+', std::move(p_a), std::move(p_b)));
}

int Var::name_count = 0;

IRVisitor::IRVisitor() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    builder = new llvm::IRBuilder<>(TheContext);
}

Value* IRVisitor::visit(ExprAST* exp) {
    exp->accept(this);
    return nullptr;
}

IRVisitor::~IRVisitor() {
    delete builder;
}

int main() {
    Var a, b;

    IRVisitor* visitor = new IRVisitor();

    ExprAST* c = (a + b) + (a + b);

    // c->dump();
    // visitor->visit(c);

    // ExprAST* d = a;

    // c->dump();
    // d->dump();

    return 0;
}

