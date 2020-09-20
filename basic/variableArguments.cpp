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
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <typeinfo>

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
#include "llvm/IR/Mangler.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/APFloat.h"

// http://llvm.org/docs/LangRef.html#int-varargs
using namespace llvm;
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    std::vector<Type*> members;
    members.push_back(llvm::Type::getInt32Ty(TheContext));
    members.push_back(llvm::Type::getInt32Ty(TheContext));
    members.push_back(llvm::Type::getInt8PtrTy(TheContext));
    members.push_back(llvm::Type::getInt8PtrTy(TheContext));

    StructType *const llvm_S = StructType::create(TheContext, "S");
    llvm_S->setBody(members);

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    std::vector<Type *> Double(1, Type::getDoubleTy(TheContext));

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    // define function
    std::vector<std::string> argNames{"a", "b"};
    auto functionName = "originalFunction";

    std::vector<Type *> args(1, Type::getInt64Ty(TheContext));

    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), args, true);

    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        std::cout << idx << std::endl;
        arg.setName(argNames[idx++]);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    FunctionType *ft = FunctionType::get(Type::getVoidTy(TheContext), Type::getInt8PtrTy(TheContext), false);

    module->getOrInsertFunction("llvm.va_start", ft);
    module->getOrInsertFunction("llvm.va_end", ft);
    llvm::AllocaInst *Alloca = builder.CreateAlloca(llvm_S, 0, "a");

    auto p = builder.CreateBitCast(Alloca, llvm::Type::getInt8PtrTy(TheContext), "hoge");

    Function *func_va_start = module->getFunction("llvm.va_start");
    Function *func_va_end = module->getFunction("llvm.va_end");

    std::cout << func_va_start << std::endl;
    std::cout << func_va_end << std::endl;

    builder.CreateCall(func_va_start, p);

    auto *VAArg1 = new llvm::VAArgInst(p, llvm::Type::getDoubleTy(TheContext), "get_double", basicBlock);
    llvm::Value *vvv1 = dyn_cast_or_null<Value>(VAArg1);
    auto *VAArg2 = new llvm::VAArgInst(p, llvm::Type::getDoubleTy(TheContext), "get_double", basicBlock);
    llvm::Value *vvv2 = dyn_cast_or_null<Value>(VAArg2);

    llvm::Value * result2 = builder.CreateFAdd(vvv1, vvv2, "a");

    builder.CreateCall(func_va_end, p);

    auto result = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.1));

    builder.CreateRet(result2);

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
    auto f = reinterpret_cast<double(*)(int, ...)>(engineBuilder->getFunctionAddress(function->getName().str()));
    if (f == NULL) {
        cout << "error" << endl;
        return 1;
    }

    std::cout << f(1, 20.0, 30.1) << std::endl;

    return 0;
}