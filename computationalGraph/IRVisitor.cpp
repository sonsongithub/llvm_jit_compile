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

#include "IRVisitor.hpp"
#include "ExprAST.hpp"

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
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/APFloat.h"

#include "Var.hpp"
#include "Execution.hpp"

static llvm::LLVMContext TheContext;

IRVisitor::IRVisitor() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    builder = new llvm::IRBuilder<>(TheContext);
    module = llvm::make_unique<llvm::Module>("abc", TheContext);
}

llvm::Value* IRVisitor::visit(Expr exp) {
    std::cout << "Visit" << std::endl;
    return exp.accept(this);
}

IRVisitor::~IRVisitor() {
    delete builder;
}

void IRVisitor::set_arguments(std::vector<Var> collected_args) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::BasicBlock;
    using llvm::Type;

    // arguments double to ptr
    std::vector<llvm::Type *> DoublePointors(1, Type::getDoublePtrTy(TheContext));
    FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), DoublePointors, false);

    std::string name = "hoge";
    function = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

    for (auto &arg : function->args())
        arg.setName("pointer");

    // Record the function arguments in the NamedValues map.
    name2Value.clear();
    for (auto &arg : function->args())
        name2Value[std::string(arg.getName())] = &arg;

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder->SetInsertPoint(basicBlock);

    llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                    function->getEntryBlock().begin());
    llvm::AllocaInst *Alloca = TmpB.CreateAlloca(Type::getDoublePtrTy(TheContext), 0, "a");

    auto a = name2Value["pointer"];

    builder->CreateStore(a, Alloca);

    llvm::Value * v = builder->CreateLoad(a);

    auto arg1 = llvm::ConstantFP::get(TheContext, llvm::APFloat(3.14/3.0));
    builder->CreateRet(v);

    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
        return;
    }

    if (verifyModule(*module)) {
        std::cout << ": Error module!\n" << std::endl;
        return;
    }

    module->print(llvm::outs(), nullptr);

    execution = new Execution(std::move(module), "hoge");

    auto p = execution->getFunctionAddress();
    auto functionPointer = reinterpret_cast<double(*)(double*)>(p);
    double aaaa = 10;
    std::cout << (*functionPointer)(&aaaa) << std::endl;
    aaaa = 14;
    std::cout << (*functionPointer)(&aaaa) << std::endl;
}

void IRVisitor::realise(Expr expr) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::Type;

    // llvm::Value *RetVal = visit(expr);

    // std::cout << RetVal << std::endl;

    // // arguments double to ptr
    // std::vector<llvm::Type *> DoublePointors(2, Type::getDoublePtrTy(TheContext));

    // FunctionType *FT =
    //     FunctionType::get(Type::getDoubleTy(TheContext), DoublePointors, false);

    // // Set names for all arguments.
    // unsigned Idx = 0;
    // for (auto &Arg : F->args())
    //     Arg.setName("x");

    // auto arg1 = llvm::ConstantFP::get(TheContext, llvm::APFloat(3.14/3.0));

    // std::vector<llvm::Value *> ArgsV;

    // ArgsV.push_back(arg1);

    // Function *TheFunction = module->getFunction("cos");

    // auto hoge = builder->CreateCall(TheFunction, ArgsV, "tmphoge");

    // builder->CreateRet(hoge);

    // if (verifyFunction(*function)) {
    //     std::cout << ": Error constructing function!\n" << std::endl;
    //     return;
    // }

    // if (verifyModule(*module)) {
    //     std::cout << ": Error module!\n" << std::endl;
    //     return;
    // }

    // module->print(llvm::outs(), nullptr);

    // execution = new Execution(std::move(module), "hoge");
    
    // auto p = execution->getFunctionAddress();
    // auto functionPointer = reinterpret_cast<double*>(p);
    // double a = 10;
    // std::cout << (*functionPointer)(&a) << std::endl;
}


// template <typename... Args>
// void IRVisitor::set_arguments(Args&&... args) {
//     using llvm::Function;
//     using llvm::FunctionType;
//     using llvm::BasicBlock;
//     using llvm::Type;

//     std::vector<Var> collected_args{std::forward<Args>(args)...};

//     std::vector<Type *> Doubles(collected_args.size(), Type::getDoubleTy(TheContext));
//     FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

//     std::string name = "hoge";
//     function = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

//     // Set names for all arguments.
//     unsigned i = 0;
//     for (auto &arg : function->args()) {
//         Var a = collected_args[i++];
//         std::cout << a.name << std::endl;
//         arg.setName(a.name);
//     }

//     // Create a new basic block to start insertion into.
//     BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
//     builder->SetInsertPoint(basicBlock);

//     // Record the function arguments in the NamedValues map.
//     name2Value.clear();
//     for (auto &arg : function->args())
//         name2Value[std::string(arg.getName())] = &arg;
// }

// template <typename T>
// T* find_name(std::vector<T*> v, std::string name)
// template Item* find_name<Item>(std::vector<Item*> v, std::string name);
// void IRVisitor::set_arguments<Var&, Var&>(Var&, Var&);

// void IRVisitor::realise(Expr expr) {
//     using llvm::Function;
//     using llvm::FunctionType;
//     using llvm::Type;

//     llvm::Value *RetVal = visit(expr);

//     std::cout << RetVal << std::endl;

//     Type::getDoublePtrTy(TheContext);

//     // Make the function type:  double(double,double) etc.
//     std::vector<llvm::Type *> Doubles(1, Type::getDoubleTy(TheContext));
//     FunctionType *FT =
//         FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
//     Function *F =
//         Function::Create(FT, Function::ExternalLinkage, "cos", module.get());

//     // Set names for all arguments.
//     unsigned Idx = 0;
//     for (auto &Arg : F->args())
//         Arg.setName("x");

//     auto arg1 = llvm::ConstantFP::get(TheContext, llvm::APFloat(3.14/3.0));

//     std::vector<llvm::Value *> ArgsV;

//     ArgsV.push_back(arg1);

//     Function *TheFunction = module->getFunction("cos");

//     auto hoge = builder->CreateCall(TheFunction, ArgsV, "tmphoge");

//     builder->CreateRet(hoge);

//     if (verifyFunction(*function)) {
//         std::cout << ": Error constructing function!\n" << std::endl;
//         return;
//     }

//     if (verifyModule(*module)) {
//         std::cout << ": Error module!\n" << std::endl;
//         return;
//     }

//     module->print(llvm::outs(), nullptr);


//     // GenericValue GV = execution->engineBuilder->runFunction(function, Args);

//     // std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
//     // FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

//     // llvm::FunctionCallee c = module->getOrInsertFunction("hoge", funcType);

//     // regist cos


//     execution = new Execution(std::move(module), "hoge");
//     // c.getCallee()
//     // module->getOrInsertFunction("atan2", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

//     // Type::getDoubleTy

//     // auto f = getfun

//     // builder->CreateCall(c, Args, "tmphoge");

//     // using Func =  int(Ts...);
//     auto p = execution->getFunctionAddress();
//     auto functionPointer = reinterpret_cast<double(*)(double, double)>(p);
//     std::cout << (*functionPointer)(10, 10) << std::endl;
// }

