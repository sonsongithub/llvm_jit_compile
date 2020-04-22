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

#include <map>
#include <memory>
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
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

// ; ModuleID = 'originalModule'
// source_filename = \"originalModule\"
//
// define double @originalFunction(double %var0, double %var1, double %var2, double %var3, double %var4, double %var5, double %var6, double %var7, double %var8, double %var9) {
// entry:
//   %addtmp = fadd double %var0, %var1
//   %addtmp1 = fadd double %addtmp, %var2
//   %addtmp2 = fadd double %addtmp1, %var3
//   %addtmp3 = fadd double %addtmp2, %var4
//   %addtmp4 = fadd double %addtmp3, %var5
//   %addtmp5 = fadd double %addtmp4, %var6
//   %addtmp6 = fadd double %addtmp5, %var7
//   %addtmp7 = fadd double %addtmp6, %var8
//   %addtmp8 = fadd double %addtmp7, %var9
//   ret double %addtmp8
// }
//
// define double @caller(double) {
// entry2:
//   %buf = load double, double* inttoptr (i64 4323303344 to double*)
//   %buf1 = load double, double* inttoptr (i64 4323303352 to double*)
//   %buf2 = load double, double* inttoptr (i64 4323303360 to double*)
//   %buf3 = load double, double* inttoptr (i64 4323303368 to double*)
//   %buf4 = load double, double* inttoptr (i64 4323303376 to double*)
//   %buf5 = load double, double* inttoptr (i64 4323303384 to double*)
//   %buf6 = load double, double* inttoptr (i64 4323303392 to double*)
//   %buf7 = load double, double* inttoptr (i64 4323303400 to double*)
//   %buf8 = load double, double* inttoptr (i64 4323303408 to double*)
//   %buf9 = load double, double* inttoptr (i64 4323303416 to double*)
//   %a = call double @originalFunction(double %buf, double %buf1, double %buf2, double %buf3, double %buf4, double %buf5, double %buf6, double %buf7, double %buf8, double %buf9)
//   ret double %a
// }

Function *createCallee(Module* module, int count) {
    // define function
    // argument name list
    auto functionName = "originalFunction";
    // argument type list
    std::vector<Type *> Doubles(count, Type::getDoubleTy(TheContext));
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    // create function in the module.
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module);

    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        auto name = "var" + std::to_string(idx++);
        arg.setName(name);
    }

    // Create argument table for LLVM::Value type.
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    builder.SetInsertPoint(basicBlock);

    llvm::Value *result = name2VariableMap["var0"];

    idx = 1;
    for (int i = 1; i < function->arg_size(); i++) {
        auto name = "var" + std::to_string(idx++);
        result = builder.CreateFAdd(result, name2VariableMap[name], "addtmp");
    }

    builder.CreateRet(result);

    // varify LLVM IR
    if (verifyFunction(*function, &llvm::errs())) {
        cout << ": Error constructing function!\n" << endl;
        throw 1;
    }

    // confirm LLVM IR
    module->print(llvm::outs(), nullptr);

    // confirm the current module status
    if (verifyModule(*module, &llvm::errs())) {
        throw 1;
    }

    return function;
}

Function *createCaller(const Function *callee, const std::vector<double> &arguments, Module* module) {
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

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    int count = 10;

    std::vector<double> array = {};

    for (int i = 0; i < count; i++) {
        array.push_back(static_cast<double>(i + 1));
    }

    try {
        Function *callee = createCallee(module.get(), count);
        Function *caller = createCaller(callee, array, module.get());

        // Builder JIT
        std::string errStr;
        ExecutionEngine *engineBuilder = EngineBuilder(std::move(module))
            .setEngineKind(EngineKind::JIT)
            .setErrorStr(&errStr)
            .create();
        if (!engineBuilder) {
            std::cout << "error: " << errStr << std::endl;
            return 1;
        }

        // Get pointer to a function which is built by EngineBuilder.
        auto f = reinterpret_cast<double(*)()>(engineBuilder->getFunctionAddress(caller->getName().str()));
        if (f == NULL) {
            throw 1;
        }
        cout << f() << endl;
    } catch(...) {
        cout << "Error" << endl;
    }
    return 0;
}
