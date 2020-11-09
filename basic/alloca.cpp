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

// ; ModuleID = 'originalModule'
// source_filename = \"originalModule\"
//
// define double @originalFunction(double* %p_a) {
// entry:
//   %0 = load double, double* %p_a
//   %a = alloca double
//   store double %0, double* %a
//   %1 = load double, double* %a
//   %tempmul = fmul double %0, %0
//   %tempmul1 = fadd double %tempmul, %1
//   ret double %tempmul1
// }

int main(int argc, char *argv[]) {
    // using declaration for llvm
    using llvm::Type;
    using llvm::Function;
    using llvm::BasicBlock;
    using llvm::FunctionType;

    // Init LLVM
    llvm::InitLLVM X(argc, argv);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    // create context
    auto context = std::make_unique<llvm::LLVMContext>();

    // Create a new module
    std::unique_ptr<llvm::Module> module(new llvm::Module("originalModule", *context));

    // LLVM IR builder
    static llvm::IRBuilder<> builder(*context);

    // define function
    std::vector<std::string> argNames{"p_a"};
    auto functionName = "originalFunction";

    std::vector<Type *> Doubles(1, Type::getDoublePtrTy(*context));
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(*context), Doubles, false);
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(argNames[idx++]);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(*context, "entry", function);
    builder.SetInsertPoint(basicBlock);

    // Create table
    static std::map<std::string, llvm::Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    llvm::Value * v = builder.CreateLoad(name2VariableMap["p_a"]);

    llvm::AllocaInst *Alloca = builder.CreateAlloca(Type::getDoubleTy(*context), 0, "a");

    builder.CreateStore(v, Alloca);

    llvm::Value *v2 = builder.CreateLoad(Alloca);

    auto temp = builder.CreateFMul(v, v, "tempmul");
    auto result = builder.CreateFAdd(temp, v2, "tempmul");

    builder.CreateRet(result);

    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
        return 1;
    }

    module->print(llvm::outs(), nullptr);

    llvm::ExitOnError("Error module!", verifyModule(*module));

    auto thread_safe_module = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));

    // Try to detect the host arch and construct an LLJIT instance.
    auto jit = llvm::orc::LLJITBuilder().create();

    if (auto error = jit->get()->addIRModule(std::move(thread_safe_module))) {
        // error
        return 1;
    }

    auto symbol = jit->get()->lookup("originalFunction");
    auto f = reinterpret_cast<double(*)(double*)>(symbol->getAddress());

    // Execution
    double a = 10;

    std::cout << f(&a) << std::endl;

    return 0;
}