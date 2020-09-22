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

//
// This code is an example of constructing a function with variable arguments in LLVM IR.
// This code builds a function which calculates the sum of the second and subsequent arguments,
// and the first one means a number of arguments.
//

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

extern "C" void printd(double a) {
    printf("%f\n", a);
}

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    std::vector<Type*> members;
    members.push_back(llvm::Type::getInt32Ty(TheContext));
    members.push_back(llvm::Type::getInt32Ty(TheContext));
    members.push_back(llvm::Type::getInt8PtrTy(TheContext));
    members.push_back(llvm::Type::getInt8PtrTy(TheContext));

    StructType *const struct_va_list = StructType::create(TheContext, "struct.va_list");
    struct_va_list->setBody(members);

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    std::vector<Type *> Double(1, Type::getDoubleTy(TheContext));

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    // define function
    auto functionName = "originalFunction";

    std::vector<Type *> args(1, Type::getInt64Ty(TheContext));

    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), args, true);

    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());

    module->getOrInsertFunction("printd", Type::getVoidTy(TheContext), Type::getDoubleTy(TheContext));

    FunctionType *ft = FunctionType::get(Type::getVoidTy(TheContext), Type::getInt8PtrTy(TheContext), false);
    module->getOrInsertFunction("llvm.va_start", ft);
    module->getOrInsertFunction("llvm.va_end", ft);

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    BasicBlock *beforeLoopBlock = BasicBlock::Create(TheContext, "beforeLoop", function);
    BasicBlock *loopBlock = BasicBlock::Create(TheContext, "loop", function);
    BasicBlock *afterLoopBlock = BasicBlock::Create(TheContext, "afterLoop", function);

    // entry block
    llvm::AllocaInst *v_va_list = builder.CreateAlloca(struct_va_list, 0, "va_list");
    llvm::AllocaInst *count = builder.CreateAlloca(llvm::Type::getInt64Ty(TheContext), 0, "count");
    llvm::AllocaInst *i = builder.CreateAlloca(llvm::Type::getInt64Ty(TheContext), 0, "i");
    llvm::AllocaInst *summation = builder.CreateAlloca(llvm::Type::getDoubleTy(TheContext), 0, "summation");

    llvm::Value* arg = (function->arg_begin());
    builder.CreateStore(arg, count);

    auto pointer_va_list = builder.CreateBitCast(v_va_list, llvm::Type::getInt8PtrTy(TheContext), "p_va_list");

    Function *func_va_start = module->getFunction("llvm.va_start");

    builder.CreateStore(arg, count);

    builder.CreateCall(func_va_start, pointer_va_list);

    auto zero_d = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    builder.CreateStore(zero_d, summation);

    llvm::Value *constant = llvm::ConstantInt::get(builder.getInt64Ty(), 0x0);
    builder.CreateStore(constant, i);

    builder.CreateBr(beforeLoopBlock);

    {
        builder.SetInsertPoint(beforeLoopBlock);
        llvm::Value* c1 = builder.CreateLoad(i);
        llvm::Value* c2 = builder.CreateLoad(count);
        llvm::Value* for_check_flag = builder.CreateICmpSLT(c1, c2, "ifcond");
        builder.CreateCondBr(for_check_flag, loopBlock, afterLoopBlock);
    }

    {
        builder.SetInsertPoint(loopBlock);
        llvm::Value* c1 = builder.CreateLoad(i);
        llvm::Value* c2 = builder.CreateAdd(c1, llvm::ConstantInt::get(builder.getInt64Ty(), 0x1), "added");
        builder.CreateStore(c2, i);

        llvm::Value* d1 = builder.CreateLoad(summation);
        auto *VAArg1 = new llvm::VAArgInst(pointer_va_list, llvm::Type::getDoubleTy(TheContext), "get_double", loopBlock);
        llvm::Value *v1 = dyn_cast_or_null<Value>(VAArg1);

        Function *printd = module->getFunction("printd");
        builder.CreateCall(printd, v1);

        llvm::Value * result = builder.CreateFAdd(d1, v1, "a");

        builder.CreateStore(result, summation);
        builder.CreateBr(beforeLoopBlock);
    }

    {
        builder.SetInsertPoint(afterLoopBlock);
        Function *func_va_end = module->getFunction("llvm.va_end");
        builder.CreateCall(func_va_end, pointer_va_list);
        llvm::Value* d1 = builder.CreateLoad(summation);
        builder.CreateRet(d1);
    }

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

    std::cout << f(5, 20.0, 30.1, 10.0, 12.0, 10.1) << std::endl;

    return 0;
}