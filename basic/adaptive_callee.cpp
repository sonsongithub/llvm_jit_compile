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

#include "llvm/ADT/APFloat.h"
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

// #include "llvm/ExecutionEngine/Orc/LLJIT.h"

#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"

using namespace llvm;
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

class AdaptiveCallee {
 public:
    const int count;
    ExecutionEngine *engineBuilder;
    std::vector<double> argumentsBuffer;
    Function *caller;

    explicit AdaptiveCallee(int c) : count(c) {
        prepare();
    }

    template <typename... Args>
    double operator() (Args&&... args) {
        std::vector<double> collected_args{std::forward<Args>(args)...};
        for (auto it = collected_args.begin(); it != collected_args.end(); it++) {
            std::cout << *it << std::endl;
        }
        return execute();
    }
 private:
    double execute();
    void prepare();
    Function *createCallee(Module* module, int count);
    Function *createCaller(Function *callee, const std::vector<double> &arguments, Module* module);
};

double AdaptiveCallee::execute() {
    auto f = reinterpret_cast<double(*)()>(engineBuilder->getFunctionAddress(caller->getName().str()));
    if (f == NULL) {
        throw 1;
    }
    return f();
}

void AdaptiveCallee::prepare() {
    std::unique_ptr<Module> module(new llvm::Module("module", TheContext));

    argumentsBuffer.clear();
    for (int i = 0; i < count; i++) {
        argumentsBuffer.push_back(static_cast<double>(i + 1));
    }

    try {
        Function *callee = createCallee(module.get(), count);
        caller = createCaller(callee, argumentsBuffer, module.get());

        // confirm LLVM IR
        module->print(llvm::outs(), nullptr);

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
    } catch (...) {
        assert(0);
    }
}

Function *AdaptiveCallee::createCallee(Module* module, int count) {
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

    // confirm the current module status
    if (verifyModule(*module, &llvm::errs())) {
        throw 1;
    }

    return function;
}

Function *AdaptiveCallee::createCaller(Function *callee, const std::vector<double> &arguments, Module* module) {
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

    // confirm the current module status
    if (verifyModule(*module, &llvm::errs())) {
        throw 1;
    }

    return caller;
}

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    AdaptiveCallee obj(5);

    std::cout << obj(10.0, 20.0) << std::endl;
}
