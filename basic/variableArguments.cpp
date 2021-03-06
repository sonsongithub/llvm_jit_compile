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

// base code

// ; This struct is different for every platform. For most platforms,
// ; it is merely an i8*.
// %struct.va_list = type { i8* }

// ; For Unix x86_64 platforms, va_list is the following struct:
// ; %struct.va_list = type { i32, i32, i8*, i8* }

// define i32 @test(i32 %X, ...) {
//   ; Initialize variable argument processing
//   %ap = alloca %struct.va_list
//   %ap2 = bitcast %struct.va_list* %ap to i8*
//   call void @llvm.va_start(i8* %ap2)

//   ; Read a single integer argument
//   %tmp = va_arg i8* %ap2, i32

//   ; Demonstrate usage of llvm.va_copy and llvm.va_end
//   %aq = alloca i8*
//   %aq2 = bitcast i8** %aq to i8*
//   call void @llvm.va_copy(i8* %aq2, i8* %ap2)
//   call void @llvm.va_end(i8* %aq2)

//   ; Stop processing of arguments.
//   call void @llvm.va_end(i8* %ap2)
//   ret i32 %tmp
// }

// declare void @llvm.va_start(i8*)
// declare void @llvm.va_copy(i8*, i8*)
// declare void @llvm.va_end(i8*)

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

    StructType *const struct_va_list = StructType::create(TheContext, "struct.va_list");
    struct_va_list->setBody(members);

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

    FunctionType *ft = FunctionType::get(Type::getVoidTy(TheContext), Type::getInt8PtrTy(TheContext), false);
    module->getOrInsertFunction("llvm.va_start", ft);
    module->getOrInsertFunction("llvm.va_end", ft);

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    llvm::AllocaInst *v_va_list = builder.CreateAlloca(struct_va_list, 0, "va_list");
    llvm::AllocaInst *count = builder.CreateAlloca(llvm::Type::getInt64Ty(TheContext), 0, "count");
    llvm::AllocaInst *summation = builder.CreateAlloca(llvm::Type::getDoubleTy(TheContext), 0, "summation");

    llvm::Value* arg = (function->arg_begin());
    builder.CreateStore(arg, count);

    auto pointer_va_list = builder.CreateBitCast(v_va_list, llvm::Type::getInt8PtrTy(TheContext), "p_va_list");

    Function *func_va_start = module->getFunction("llvm.va_start");

    builder.CreateCall(func_va_start, pointer_va_list);

    Function *func_va_end = module->getFunction("llvm.va_end");

    auto *VAArg1 = new llvm::VAArgInst(pointer_va_list, llvm::Type::getDoubleTy(TheContext), "get_double", basicBlock);
    llvm::Value *vvv1 = dyn_cast_or_null<Value>(VAArg1);

    auto *VAArg2 = new llvm::VAArgInst(pointer_va_list, llvm::Type::getDoubleTy(TheContext), "get_double", basicBlock);
    llvm::Value *vvv2 = dyn_cast_or_null<Value>(VAArg2);

    llvm::Value * result2 = builder.CreateFAdd(vvv1, vvv2, "a");

    builder.CreateCall(func_va_end, pointer_va_list);

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