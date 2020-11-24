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

class AdaptiveCallee {
 public:
    std::unique_ptr<llvm::LLVMContext> context;
    const int count;
    std::vector<double> argumentsBuffer;
    llvm::Function *caller;

    llvm::orc::ThreadSafeModule threadSafeModule;

    explicit AdaptiveCallee(int c) : count(c) {
        context = std::make_unique<llvm::LLVMContext>();
        prepare();
    }

    template <typename... Args>
    double operator() (Args&&... args) {
        std::vector<double> collected_args{std::forward<Args>(args)...};
        assert(count == collected_args.size() && "The size of arguments buffer do not match one of arguments .");
        for (int i = 0; i < count; i++) {
            argumentsBuffer[i] = collected_args[i];
        }
        return execute();
    }

 private:
    double execute();
    void prepare();
    llvm::Function *createCallee(llvm::Module* module, int count);
    llvm::Function *createCaller(llvm::Function *callee, const std::vector<double> &arguments, llvm::Module* module);
};

double AdaptiveCallee::execute() {
    auto jit = llvm::orc::LLJITBuilder().create();
    auto error = jit->get()->addIRModule(std::move(threadSafeModule));
    assert(!error && "LLJIT can not add handle module.");
    auto symbol = jit->get()->lookup(caller->getName().str());
    if (symbol) {
        auto f = reinterpret_cast<double(*)()>(symbol->getAddress());
        if (f)
            return f();
        else
            assert(0 && "Can not get the address of built target function.");
    } else {
            assert(0 && "Can not find the symbol of built target function.");
    }
}

void AdaptiveCallee::prepare() {
    std::unique_ptr<llvm::Module> module(new llvm::Module("module", *context));

    argumentsBuffer.clear();
    for (int i = 0; i < count; i++) {
        argumentsBuffer.push_back(static_cast<double>(i + 1));
    }

    try {
        llvm::Function *callee = createCallee(module.get(), count);
        caller = createCaller(callee, argumentsBuffer, module.get());

        // confirm LLVM IR
        module->print(llvm::outs(), nullptr);

        // All code are saved into thread safe module.
        threadSafeModule = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
    } catch (...) {
        assert(0);
    }
}

llvm::Function *AdaptiveCallee::createCallee(llvm::Module* module, int count) {
    using llvm::Type;
    using llvm::FunctionType;
    using llvm::Function;
    using llvm::Value;
    using llvm::BasicBlock;

    // define function
    // argument name list
    auto functionName = "originalFunction";
    // argument type list
    std::vector<Type *> Doubles(count, Type::getDoubleTy(*context));
    // create function type
    FunctionType *functionType = FunctionType::get(llvm::Type::getDoubleTy(*context), Doubles, false);
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
    BasicBlock *basicBlock = BasicBlock::Create(*context, "entry", function);

    // LLVM IR builder
    static llvm::IRBuilder<> builder(*context);

    builder.SetInsertPoint(basicBlock);

    Value *result = name2VariableMap["var0"];

    idx = 1;
    for (int i = 1; i < function->arg_size(); i++) {
        auto name = "var" + std::to_string(idx++);
        result = builder.CreateFAdd(result, name2VariableMap[name], "addtmp");
    }

    builder.CreateRet(result);

    llvm::ExitOnError("Error function!", verifyFunction(*function));
    llvm::ExitOnError("Error module!", verifyModule(*module));

    return function;
}

llvm::Function *AdaptiveCallee::createCaller(llvm::Function *callee, const std::vector<double> &arguments, llvm::Module* module) {
    using llvm::Type;
    using llvm::FunctionType;
    using llvm::Function;
    using llvm::Value;
    using llvm::BasicBlock;

    // define function
    // argument name list
    auto functionName = "caller";
    // argument type list
    std::vector<Type *> vacant = {};
    // create function type
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(*context), vacant, false);
    // create function in the module.
    Function *caller = Function::Create(functionType, Function::ExternalLinkage, functionName, module);

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(*context, "entry2", caller);

    static llvm::IRBuilder<> builder(*context);

    builder.SetInsertPoint(basicBlock);

    // LLVM IR builder
    std::vector<Value *> arguments_values = {};

    for (int i = 0; i < arguments.size(); i++) {
        u_int64_t p = (u_int64_t)&arguments[i];
        llvm::Constant *constant = llvm::ConstantPointerNull::getIntegerValue(llvm::PointerType::getDoublePtrTy(*context), llvm::APInt(64, 1, &p));
        auto temp = builder.CreateLoad(constant, "buf");
        arguments_values.push_back(temp);
    }

    llvm::ArrayRef<Value*> a(arguments_values);

    auto result = builder.CreateCall(callee, a, "a");

    builder.CreateRet(result);

    llvm::ExitOnError("Error function!", verifyFunction(*caller));
    llvm::ExitOnError("Error module!", verifyModule(*module));

    return caller;
}

int main(int argc, char *argv[]) {
    llvm::InitLLVM X(argc, argv);
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    AdaptiveCallee obj(2);
    AdaptiveCallee obj2(3);

    std::cout << obj2(101.0, 10.0, 20.0) << std::endl;
    std::cout << obj(10.0, 20.0) << std::endl;
}
