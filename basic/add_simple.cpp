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
// source_filename = "originalModule"
//
// define double @originalFunction(double %a, double %b) {
// entry:
//   %addtmp = fadd double %a, %b
//   ret double %addtmp
// }

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    // define function
    // argument name list
    auto functionName = "originalFunction";
    std::vector<std::string> argNames{"a", "b"};
    // argument type list
    std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    // create function in the module.
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    std::vector<llvm::Value*> arg_list;

    // Create argument table for LLVM::Value type.
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        arg_list.push_back(&arg);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    // calculate "add"
    auto result = builder.CreateFAdd(arg_list[0], arg_list[1], "addtmp");

    // set return
    builder.CreateRet(result);

    // varify LLVM IR
    if (verifyFunction(*function)) {
        cout << ": Error constructing function!\n" << endl;
        return 1;
    }

    // confirm LLVM IR
    module->print(llvm::outs(), nullptr);

    // confirm the current module status
    if (verifyModule(*module)) {
        cout << ": Error module!\n" << endl;
        return 1;
    }

    // Builder JIT
    std::string errStr;
    ExecutionEngine *executionEngine = EngineBuilder(std::move(module))
        .setEngineKind(EngineKind::JIT)
        .setErrorStr(&errStr)
        .create();
    if (!executionEngine) {
        std::cout << "error: " << errStr << std::endl;
        return 1;
    }

    // Get pointer to a function which is built by EngineBuilder.
    auto f = reinterpret_cast<double(*)(double, double)>(
            executionEngine->getFunctionAddress(function->getName().str()));
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
