
#include <memory>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

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
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/APFloat.h"

using namespace llvm;
using namespace std;

// Context for LLVM
static LLVMContext TheContext;

int main() {

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create a new module
    std::unique_ptr<Module> module(new llvm::Module("originalModule", TheContext));

    // LLVM IR builder
    static IRBuilder<> builder(TheContext);

    // function
    std::vector<std::string> argNames{"a", "b"};
    auto functionName = "originalFunction";

    std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
    FunctionType *functionType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *function = Function::Create(functionType, Function::ExternalLinkage, functionName, module.get());
    
    // Set names for all arguments.
    // I'd like to use "zip" function, here.....
    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(argNames[idx++]);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder.SetInsertPoint(basicBlock);

    // Create table
    static std::map<std::string, Value*> name2VariableMap;
    for (auto &arg : function->args()) {
        name2VariableMap[arg.getName()] = &arg;
    }

    auto body2 = builder.CreateFAdd(name2VariableMap["a"], name2VariableMap["b"], "addtmp");
    auto body = builder.CreateFAdd(body2, name2VariableMap["b"], "addtmp2");

    builder.CreateRet(body);

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
    auto f = reinterpret_cast<double(*)(double, double)>(engineBuilder->getFunctionAddress(function->getName().str()));
    if (f == NULL) {
        cout << "error" << endl;
        return 1;
    }

    // Execution
    cout << f(1.0, 2.0) << endl;

    return 0;
};