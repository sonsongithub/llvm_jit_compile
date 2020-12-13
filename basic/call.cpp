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
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/InitLLVM.h"

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

// C
extern "C" double two_times(double a) {
    return a * 2;
}

// C++
double three_times(double a) {
    return a * 3;
}

int main(int argc, char *argv[]) {
    // using declaration for llvm
    using llvm::Type;
    using llvm::Function;
    using llvm::BasicBlock;
    using llvm::FunctionType;
    using llvm::Value;
    using llvm::LLVMContext;
    using llvm::Module;

    // Init LLVM
    llvm::InitLLVM X(argc, argv);
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // create context
    auto context = std::make_unique<LLVMContext>();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", *context));

    module->getOrInsertFunction("two_times", Type::getDoubleTy(*context), Type::getDoubleTy(*context));

    module->getOrInsertFunction("_Z11three_timesd", Type::getDoubleTy(*context), Type::getDoubleTy(*context));

    module->getOrInsertFunction("sin", Type::getDoubleTy(*context), Type::getDoubleTy(*context));

    std::vector<Type *> Double(1, Type::getDoubleTy(*context));

    // LLVM IR builder
    static llvm::IRBuilder<> builder(*context);

    // define function
    std::vector<std::string> argNames{"a", "b"};
    auto functionName = "originalFunction";

    std::vector<Type *> Doubles(2, Type::getDoubleTy(*context));
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(*context), Doubles, false);
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    FunctionType *conFunctionType = FunctionType::get(Type::getDoubleTy(*context), Double, false);
    Function::Create(conFunctionType, Function::ExternalLinkage, "sin", module.get());

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
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    Function *TheFunction = module->getFunction("_Z11three_timesd");

    Function *cosFunction = module->getFunction("sin");

    auto hoge1 = builder.CreateCall(TheFunction, name2VariableMap["a"], "tmphoge");

    auto hoge2 = builder.CreateCall(cosFunction, hoge1, "tmphoge");

    auto result = builder.CreateFAdd(hoge2, name2VariableMap["b"], "addtmp");

    builder.CreateRet(result);

    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
        return 1;
    }

    module->print(llvm::outs(), nullptr);

    llvm::ExitOnError("Error module!", verifyModule(*module));

    // Try to detect the host arch and construct an LLJIT instance.
    auto jit = llvm::orc::LLJITBuilder().create();

    if (jit) {
        auto thread_safe_module = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
        auto error = jit->get()->addIRModule(std::move(thread_safe_module));
        llvm::orc::JITDylib &dylib = jit->get()->getMainJITDylib();
        const llvm::DataLayout &dataLayout = jit->get()->getDataLayout();
        dylib.addGenerator(
            llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dataLayout.getGlobalPrefix())));
        assert(!error && "LLJIT can not add handle module.");
        auto symbol = jit->get()->lookup("originalFunction");
        if (!symbol) {
            assert(0 && "Can not find a symbol of the main function.");
        }
        auto f = reinterpret_cast<double(*)(double, double)>(symbol->getAddress());
        assert(f && "Can not get function pointer.");

        std::cout << "Evaluated to " << f(10, 11) << std::endl;
    } else {
        std::cout << "Error - LLJIT can not be initialized." << std::endl;
    }
    return 0;
}