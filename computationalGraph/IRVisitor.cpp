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
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/APFloat.h"

#include "Var.hpp"
#include "Func.hpp"

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

llvm::Value* IRVisitor::createValue(double value) {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(value));
}

llvm::Function* IRVisitor::create_callee(const std::vector<Var> &argumentPlacefolders, std::string name, Expr expr) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::BasicBlock;
    using llvm::Type;

    // arguments double to ptr
    std::vector<llvm::Type *> Doubles(argumentPlacefolders.size(), Type::getDoubleTy(TheContext));
    FunctionType *funcType = llvm::FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    llvm::Function *callee = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto &arg : callee->args()) {
        arg.setName(argumentPlacefolders[idx++].name);
    }

    // Record the function arguments in the NamedValues map.
    name2Value.clear();
    for (auto &arg : callee->args()) {
        name2Value[std::string(arg.getName())] = &arg;
    }

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *basicBlock = llvm::BasicBlock::Create(TheContext, "entry", callee);
    builder->SetInsertPoint(basicBlock);

    llvm::Value *RetVal = this->visit(expr);

    builder->CreateRet(RetVal);

    // varify LLVM IR
    if (verifyFunction(*callee, &llvm::errs())) {
        throw 1;
    }

    // confirm LLVM IR
    module->print(llvm::outs(), nullptr);

    return callee;
}

llvm::Function *IRVisitor::create_caller(llvm::Function *callee, const std::vector<double> &arguments, std::string name) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::BasicBlock;
    using llvm::Type;

    // define function
    // argument name list
    auto functionName = "caller";
    // argument type list
    std::vector<Type *> vacant = {Type::getDoubleTy(TheContext)};
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), vacant, false);
    // create function in the module.
    Function *caller = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", caller);

    static llvm::IRBuilder<> builder(TheContext);

    builder.SetInsertPoint(basicBlock);

    // LLVM IR builder
    std::vector<llvm::Value *> arguments_values = {};

    for (int i = 0; i < arguments.size(); i++) {
        u_int64_t p = (u_int64_t)&arguments[i];
        llvm::Constant *constant = llvm::ConstantPointerNull::getIntegerValue(llvm::PointerType::getDoublePtrTy(TheContext), llvm::APInt(64, 1, &p));
        auto temp = builder.CreateLoad(constant, "buf");
        arguments_values.push_back(temp);
    }

    llvm::ArrayRef<llvm::Value*> a(arguments_values);

    auto result = builder.CreateCall(callee, a, "a");

    builder.CreateRet(result);

    // varify LLVM IR
    if (verifyFunction(*caller, &llvm::errs())) {
        throw 1;
    }

    // confirm LLVM IR
    module->print(llvm::outs(), nullptr);

    // confirm the current module status
    if (verifyModule(*module, &llvm::errs())) {
        throw 1;
    }

    return caller;
}

llvm::ExecutionEngine *IRVisitor::create_engine() {
    llvm::EngineBuilder builder(std::move(module));
    return builder.create();
}
