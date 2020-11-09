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

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/InitLLVM.h"

using namespace llvm;
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

// ; ModuleID = 'originalModule'
// source_filename = "originalModule"
//
// define double @originalFunction(double %0) {
// entry:
//   %ifcond = fcmp ult double %0, 0.000000e+00
//   br i1 %ifcond, label %then, label %else
//
// then:                                             ; preds = %entry
//   ret double -1.000000e+00
//
// else:                                             ; preds = %entry
//   ret double 1.000000e+00
// }

int main(int argc, char *argv[]) {
    // Init LLVM
    llvm::InitLLVM X(argc, argv);

    // create context
    auto context = std::make_unique<LLVMContext>();

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", *context));

    // LLVM IR builder
    static IRBuilder<> builder(*context);

    // define function
    // argument name list
    auto functionName = "originalFunction";
    // argument type list
    std::vector<Type *> Doubles(1, Type::getDoubleTy(*context));
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(*context), Doubles, false);
    // create function in the module.
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Set names for all arguments.
    llvm::Value* arg = (function->arg_begin());

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(*context, "entry", function);
    builder.SetInsertPoint(basicBlock);

    llvm::Value* flag = builder.CreateFCmpULT(arg, ConstantFP::get(*context, APFloat(0.0)), "ifcond");

    BasicBlock *thenBB = BasicBlock::Create(*context, "then", function);
    BasicBlock *elseBB = BasicBlock::Create(*context, "else", function);

    // if -------------------------------
    builder.CreateCondBr(flag, thenBB, elseBB);

    // then -----------------------------
    builder.SetInsertPoint(thenBB);
    thenBB = builder.GetInsertBlock();

    auto result_minus = llvm::ConstantFP::get(*context, llvm::APFloat(-1.0));
    builder.CreateRet(result_minus);

    // else -----------------------------
    builder.SetInsertPoint(elseBB);
    elseBB = builder.GetInsertBlock();

    auto result_one = llvm::ConstantFP::get(*context, llvm::APFloat(1.0));
    builder.CreateRet(result_one);

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

    auto thread_safe_module = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));

    // Try to detect the host arch and construct an LLJIT instance.
    auto jit = llvm::orc::LLJITBuilder().create();

    if (auto error = jit->get()->addIRModule(std::move(thread_safe_module))) {
        // error
        return 1;
    }

    auto symbol = jit->get()->lookup("originalFunction");
    auto f = reinterpret_cast<double(*)(double)>(symbol->getAddress());
    if (f == NULL) {
        cout << "error" << endl;
        return 1;
    }

    // Execution
    // a + b
    cout << f(-1.0) << endl;
    cout << f(1.0) << endl;

    return 0;
}
