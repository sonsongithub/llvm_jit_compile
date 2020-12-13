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

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/InitLLVM.h"

int main(int argc, char *argv[]) {
    // using declaration for llvm
    using llvm::Type;
    using llvm::Function;
    using llvm::BasicBlock;
    using llvm::FunctionType;
    using llvm::Value;

    using llvm::orc::IRCompileLayer;
    using llvm::orc::ExecutionSession;
    using llvm::orc::RTDyldObjectLinkingLayer;
    using llvm::orc::ConcurrentIRCompiler;

    ExecutionSession executionSession;

    llvm::InitLLVM X(argc, argv);
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto jitTargetMachineBuilder = llvm::orc::JITTargetMachineBuilder::detectHost();
    assert(jitTargetMachineBuilder && "Error: JITTargetMachineBuilder");

    auto dataLayout = jitTargetMachineBuilder->getDefaultDataLayoutForTarget();
    assert(dataLayout && "Error: getfaultDataLayoutForTarget");

    RTDyldObjectLinkingLayer::GetMemoryManagerFunction f = []() { return std::make_unique<llvm::SectionMemoryManager>(); };
    RTDyldObjectLinkingLayer objectLinkingLayer(executionSession, f);

    IRCompileLayer compileLayer(executionSession, objectLinkingLayer, std::make_unique<ConcurrentIRCompiler>(std::move(*jitTargetMachineBuilder)));

    llvm::orc::ThreadSafeContext orc_context(std::make_unique<llvm::LLVMContext>());

    llvm::orc::JITDylib &mainJITDylib = executionSession.createJITDylib("<main>");

    mainJITDylib.addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dataLayout->getGlobalPrefix())));

    llvm::orc::MangleAndInterner mangle(executionSession, *dataLayout);
    llvm::LLVMContext *context = orc_context.getContext();

    // Open a new module.
    std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>("my cool jit", *context);

    module->setDataLayout(*dataLayout);

    // Create a new builder for the module.
    std::unique_ptr<llvm::IRBuilder<>> builder = std::make_unique<llvm::IRBuilder<>>(*context);

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
    builder->SetInsertPoint(basicBlock);

    // calculate "add"
    auto result = builder->CreateFAdd(name2VariableMap["a"], name2VariableMap["b"], "addtmp");

    // set return
    builder->CreateRet(result);

    llvm::ExitOnError("Error constructing function!", verifyFunction(*function));

    // confirm LLVM IR, text mode.
    module->print(llvm::outs(), nullptr);

    auto error = compileLayer.add(mainJITDylib, llvm::orc::ThreadSafeModule(std::move(module), orc_context));
    assert(!error && "Error: add module to IRCompileLayer.");

    auto symbol = executionSession.lookup({&mainJITDylib}, mangle("originalFunction"));

    if (symbol) {
        auto *func = (double (*)(double, double))(intptr_t)symbol->getAddress();
        assert(func && "Failed to codegen function");
        std::cout << "Evaluated to " << func(10, 11) << std::endl;
    }

    return 0;
}
