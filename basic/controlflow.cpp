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
    // argument type list
    std::vector<Type *> Doubles(1, Type::getDoubleTy(TheContext));
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    // create function in the module.
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Set names for all arguments.
    llvm::Value* arg = (function->arg_begin());

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry");
    function->getBasicBlockList().push_back(basicBlock);
    builder.SetInsertPoint(basicBlock);

    // Convert condition to a bool by comparing non-equal to 0.0.
    llvm::Value* flag = builder.CreateFCmpULT(arg, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");

    // // Create blocks for the then and else cases.  Insert the 'then' block at the
    // // end of the function.

    BasicBlock *thenBB = BasicBlock::Create(TheContext, "then");
    function->getBasicBlockList().push_back(thenBB);

    BasicBlock *elseBB = BasicBlock::Create(TheContext, "else");
    function->getBasicBlockList().push_back(elseBB);

    builder.CreateCondBr(flag, loop, afterLoopBlock);

    builder.SetInsertPoint(thenBB);

    auto v1 = llvm::ConstantFP::get(TheContext, llvm::APFloat(1.0));
    // builder.CreateBr(mergeBB);
    thenBB = builder.GetInsertBlock();

    auto result = llvm::ConstantFP::get(TheContext, llvm::APFloat(-1.0));
    builder.CreateRet(result);

    // TheFunction->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);
    elseBB = builder.GetInsertBlock();

    auto result2 = llvm::ConstantFP::get(TheContext, llvm::APFloat(1.0));
    builder.CreateRet(result2);

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
    ExecutionEngine *engineBuilder = EngineBuilder(std::move(module))
        .setEngineKind(EngineKind::JIT)
        .setErrorStr(&errStr)
        .create();
    if (!engineBuilder) {
        std::cout << "error: " << errStr << std::endl;
        return 1;
    }

    // Get pointer to a function which is built by EngineBuilder.
    auto f = reinterpret_cast<double(*)(double)>(
            engineBuilder->getFunctionAddress(function->getName().str()));
    if (f == NULL) {
        cout << "error" << endl;
        return 1;
    }

    // Execution
    // a + b
    cout << f(-1.0) << endl;
    cout << f( 1.0) << endl;

    return 0;
}
