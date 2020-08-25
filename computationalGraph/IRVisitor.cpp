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
#include "Execution.hpp"
#include "Func.hpp"

using namespace llvm;
using namespace std;

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

    args = collected_args;

    // std::cout << collected_args.size() << std::endl;

    // // arguments double to ptr
    // std::vector<llvm::Type *> Doubles(collected_args.size(), Type::getDoubleTy(TheContext));
    // FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    // std::string name = "hoge";
    // function = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

    // // Set names for all arguments.
    // unsigned idx = 0;
    // for (auto &arg : function->args()) {
    //     arg.setName(collected_args[idx++].name);
    // }

    // // Record the function arguments in the NamedValues map.
    // name2Value.clear();
    // for (auto &arg : function->args()) {
    //     name2Value[std::string(arg.getName())] = &arg;
    // }
}

Function *IRVisitor::createCaller(Function *callee, const std::vector<double> &arguments, Module* module) {
    // define function
    // argument name list
    auto functionName = "caller";
    // argument type list
    std::vector<Type *> vacant = {Type::getDoubleTy(TheContext)};
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), vacant, false);
    // create function in the module.
    Function *caller = Function::Create(functionType, Function::ExternalLinkage, functionName, module);

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry2", caller);

    static IRBuilder<> builder(TheContext);

    builder.SetInsertPoint(basicBlock);

    // LLVM IR builder
    std::vector<llvm::Value *> arguments_values = {};

    for (int i = 0; i < arguments.size(); i++) {
        u_int64_t p = (u_int64_t)&arguments[i];
        llvm::Constant *constant = llvm::ConstantPointerNull::getIntegerValue(llvm::PointerType::getDoublePtrTy(TheContext), APInt(64, 1, &p));
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

Func* IRVisitor::realise(Expr expr) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::Type;

    Func *func = new Func();

    std::cout << args.size() << std::endl;

    func->prepare(args.size());

    // arguments double to ptr
    std::vector<llvm::Type *> Doubles(args.size(), Type::getDoubleTy(TheContext));
    FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    std::string name = "callee";
    function = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(args[idx++].name);
    }

    // Record the function arguments in the NamedValues map.
    name2Value.clear();
    for (auto &arg : function->args()) {
        name2Value[std::string(arg.getName())] = &arg;
    }

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *basicBlock = llvm::BasicBlock::Create(TheContext, "entry", function);
    builder->SetInsertPoint(basicBlock);

    llvm::Value *RetVal = visit(expr);

    builder->CreateRet(RetVal);

    // varify LLVM IR
    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
    }

    Function *caller = createCaller(function, func->argumentsBuffer, module.get());

    module->print(llvm::outs(), nullptr);

    // confirm the current module status
    if (verifyModule(*module)) {
        std::cout << ": Error module!\n" << std::endl;
    }

    // Builder JIT
    std::string errStr;
    engineBuilder = EngineBuilder(std::move(module))
        .setEngineKind(EngineKind::JIT)
        .setErrorStr(&errStr)
        .create();
    if (!engineBuilder) {
        std::cout << "error: " << errStr << std::endl;
        assert(0);
    }
    func->engineBuilder = engineBuilder;

    return func;
}

uint64_t IRVisitor::getFunctionAddress() {
    return 0;
    // return execution->getFunctionAddress();
}
