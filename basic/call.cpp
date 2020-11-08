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
#include <typeinfo>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"

#include "llvm/Target/TargetMachine.h"

#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"

// ; ModuleID = 'originalModule'
// source_filename = \"originalModule\"
//
// declare double @hoge(double)
//
// declare double @_Z8hoge_cppd(double)
//
// declare double @sin(double)
//
// define double @originalFunction(double %a, double %b) {
// entry:
//   %tmphoge = call double @_Z8hoge_cppd(double %a)
//   %tmphoge1 = call double @cos(double %tmphoge)
//   %addtmp = fadd double %tmphoge1, %b
//   ret double %addtmp
// }

// declare double @cos(double)

using namespace llvm;
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

extern "C" double hoge(double a) {
    return a * 2;
}

double hoge_cpp(double a) {
    return a * 2;
}

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    // module->getOrInsertFunction("cos", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

    module->getOrInsertFunction("hoge", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

    module->getOrInsertFunction("_Z8hoge_cppd", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

    module->getOrInsertFunction("sin", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

    std::vector<Type *> Double(1, Type::getDoubleTy(TheContext));

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    // define function
    std::vector<std::string> argNames{"a", "b"};
    auto functionName = "originalFunction";

    std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    FunctionType *conFunctionType = FunctionType::get(Type::getDoubleTy(TheContext), Double, false);
    Function::Create(conFunctionType, Function::ExternalLinkage, "cos", module.get());

    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(argNames[idx++]);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    // Create table
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    Function *TheFunction = module->getFunction("_Z8hoge_cppd");

    Function *cosFunction = module->getFunction("cos");

    auto hoge1 = builder.CreateCall(TheFunction, name2VariableMap["a"], "tmphoge");

    auto hoge2 = builder.CreateCall(cosFunction, hoge1, "tmphoge");

    auto result = builder.CreateFAdd(hoge2, name2VariableMap["b"], "addtmp");

    builder.CreateRet(result);

    if (verifyFunction(*function)) {
        cout << ": Error constructing function!\n" << endl;
        return 1;
    }

    module->print(llvm::outs(), nullptr);

    if (verifyModule(*module)) {
        cout << ": Error module!\n" << endl;
        return 1;
    }

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
    auto f = reinterpret_cast<double(*)(double, double)>(engineBuilder->getFunctionAddress(function->getName().str()));
    if (f == NULL) {
        cout << "error" << endl;
        return 1;
    }

    // Execution
    // a + b
    cout << f(1.0, 2.0) << endl;
    cout << f(2.0, 2.0) << endl;

    return 0;
}