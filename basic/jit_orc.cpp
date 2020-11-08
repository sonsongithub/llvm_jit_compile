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
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace std;

// https://llvm.org/docs/ORCv2.html
// https://stackoverflow.com/questions/62271258/llvm-orc-v2-symbol-to-global-variable

int main(int argc, char *argv[]) {

    auto context = std::make_unique<LLVMContext>();

    // Initialize LLVM.
    llvm::InitLLVM X(argc, argv);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    // Try to detect the host arch and construct an LLJIT instance.
    auto jit = llvm::orc::LLJITBuilder().create();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", *context));

    // LLVM IR builder
    static IRBuilder<> builder(*context);

    // define function
    // argument name list
    auto functionName = "originalFunction";
    std::vector<std::string> argNames{"a", "b"};
    // argument type list
    std::vector<Type *> Doubles(2, Type::getDoubleTy(*context));
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(*context), Doubles, false);
    // create function in the module.
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(argNames[idx++]);
    }

    // Create argument table for LLVM::Value type.
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(*context, "entry", function);
    builder.SetInsertPoint(basicBlock);

    // calculate "add"
    auto result = builder.CreateFAdd(name2VariableMap["a"], name2VariableMap["b"], "addtmp");

    // set return
    builder.CreateRet(result);

    llvm::ExitOnError("Error constructing function!", verifyFunction(*function));

    llvm::ExitOnError("Error module!", verifyModule(*module));

    // confirm LLVM IR, text mode.
    module->print(llvm::outs(), nullptr);

    auto thread_safe_module = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));

    if (auto error = jit->get()->addIRModule(std::move(thread_safe_module))) {
        // error
        return 1;
    }

    auto symbol = jit->get()->lookup("originalFunction");
    auto f = reinterpret_cast<double(*)(double, double)>(symbol->getAddress());
    
    printf("%f\n", f(2, 3));

    return 0;
}
