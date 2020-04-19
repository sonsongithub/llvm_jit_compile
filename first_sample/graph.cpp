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

// Context for LLVM

static LLVMContext TheContext;

class Func;
class Variable;
class Execution;

class IRVisitor {
    llvm::IRBuilder<> *builder;
    std::vector<Variable*> arguments;
 public:
    IRVisitor();
    ~IRVisitor();
    void visit(Variable *variable);
    void visit(Func *func);
};

class Execution {
    llvm::ExecutionEngine *engineBuilder;
    std::string functionName;
 public:
    Execution(std::unique_ptr<llvm::Module>, std::string);
    uint64_t getFunctionAddress();
};

class Variable {
 public:
    Func operator + (Variable obj);
    Func operator - (Variable obj);
};

class Func {
    Variable lhs;
    Variable rhs;
    Execution *execution;
 public:
    char op;
    Func(char Op, Variable lhs, Variable rhs)
            : op(Op), lhs(lhs), rhs(rhs) {}
    uint64_t realize();
    void accept(IRVisitor* visitor);
    void realize(std::unique_ptr<llvm::Module> module, std::string name);
    double operator()(double a, double b, ...);
};

IRVisitor::IRVisitor() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    builder = new llvm::IRBuilder<>(TheContext);
}

IRVisitor::~IRVisitor() {
    delete builder;
}

void IRVisitor::visit(Variable *variable) {
}

void IRVisitor::visit(Func *func) {
    // Create a new module
    std::unique_ptr<Module> module(new Module("originalModule", TheContext));
    // function
    std::vector<std::string> argNames{"a", "b"};
    auto functionName = "originalFunction";

    std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
    FunctionType *functionType =
        FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *function =
        Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());
    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(argNames[idx++]);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder->SetInsertPoint(basicBlock);

    // Create table
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    switch (func->op) {
    case '+':
        {
            auto body = builder->CreateFAdd(
                name2VariableMap["a"],
                name2VariableMap["b"],
                "addtmp");
            builder->CreateRet(body);
        }
        break;
    case '-':
        {
            auto body = builder->CreateFSub(
                name2VariableMap["a"],
                name2VariableMap["b"],
                "subtmp");
            builder->CreateRet(body);
        }
        break;
    default:
        return;
    }

    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
        return;
    }

    if (verifyModule(*module)) {
        std::cout << ": Error module!\n" << std::endl;
        return;
    }

    module->print(llvm::outs(), nullptr);

    func->realize(std::move(module), function->getName());
}

Execution::Execution(std::unique_ptr<llvm::Module> module, std::string name) {
    // Builder JIT
    functionName = name;
    std::string errStr;
    engineBuilder = llvm::EngineBuilder(std::move(module))
        .setEngineKind(llvm::EngineKind::JIT)
        .setErrorStr(&errStr)
        .create();
    if (!engineBuilder) {
        std::cout << "error: " << errStr << std::endl;
    }
}

uint64_t Execution::getFunctionAddress() {
    return engineBuilder->getFunctionAddress(functionName);
}

Func Variable::operator + (Variable obj) {
    return Func('+', *(this), obj);
}

Func Variable::operator - (Variable obj) {
    return Func('-', *(this), obj);
}

void Func::realize(std::unique_ptr<llvm::Module> module, std::string name) {
    execution = new Execution(std::move(module), name);
}

void Func::accept(IRVisitor* visitor) {
}

double Func::operator()(double a, double b, ...) {
    if (execution) {
        auto p = execution->getFunctionAddress();
        auto f = reinterpret_cast<double(*)(double, double)>(p);
        return f(a, b);
    }
    return 0;
}

int main() {
    auto visitor = IRVisitor();

    Variable x, y;
    Func my_func = x - y;
    Func my_func2 = x + y;

    visitor.visit(&my_func);
    visitor.visit(&my_func2);

    std::cout << my_func(1, 2) << std::endl;
    std::cout << my_func2(20    , 2) << std::endl;

    return 0;
}
