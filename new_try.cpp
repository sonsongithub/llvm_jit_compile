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

static llvm::LLVMContext TheContext;

class ExprAST;
class IRVisitor;
class Var;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
 public:
    ExprAST() = default;
    virtual ~ExprAST() = default;
    virtual void dump(int level = 0) = 0;
    virtual llvm::Value* accept(IRVisitor* builder) = 0;
};

/// VarExprAST - Expression class for referencing a Var, like "a".
class VarExprAST : public ExprAST {
    std::string name;

 public:
    explicit VarExprAST(std::string name) : name(name) {}
    // ~VarExprAST() { std::cout << "VarExprAST is deleted." << std::endl; }
    void dump(int level = 0) override;
    llvm::Value* accept(IRVisitor* visitor) override;
};

void VarExprAST::dump(int level) {
    for (int i = 0; i < level; i++) { std::cout << "-"; }
    std::cout << "VarExprAST" << std::endl;
}

struct Expr {
    std::shared_ptr<ExprAST> value;
 public:
    Expr() = default;
    explicit Expr(std::shared_ptr<ExprAST> ast) : value(ast) {}
    llvm::Value* accept(IRVisitor* builder);
};

llvm::Value* Expr::accept(IRVisitor* builder) {
    return value->accept(builder);
}

class Execution;

class IRVisitor {
 public:
    Execution *execution;
    llvm::IRBuilder<> *builder;
    std::map<std::string, llvm::Value *> name2Value;
 private:
    std::unique_ptr<llvm::Module> module;
    llvm::Function *function;
 public:
    IRVisitor();
    ~IRVisitor();
    llvm::Value* visit(Expr expr);
    template <typename... Args> void set_arguments(Args&&... args);
    void realise(Expr expr);
};

llvm::Value* VarExprAST::accept(IRVisitor* visitor) {
    auto p = visitor->name2Value[name];
    return p;
}

IRVisitor::IRVisitor() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    builder = new llvm::IRBuilder<>(TheContext);
    module = std::make_unique<llvm::Module>("abc", TheContext);
}

llvm::Value* IRVisitor::visit(Expr exp) {
    std::cout << "Visit" << std::endl;
    return exp.accept(this);
}

IRVisitor::~IRVisitor() {
    delete builder;
}

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char op;
    Expr lhs;
    Expr rhs;

 public:
    BinaryExprAST(char operation, Expr a, Expr b);
    // ~BinaryExprAST() { std::cout << "BinaryExprAST is deleted." << std::endl; }
    void dump(int level = 0) override;
    llvm::Value* accept(IRVisitor* builder) override;
};

BinaryExprAST::BinaryExprAST(char operation, Expr a, Expr b) {
    lhs = std::move(a);
    rhs = std::move(b);
    op = operation;
}

llvm::Value* BinaryExprAST::accept(IRVisitor* visiter) {
    std::cout << "accept - BinaryExprAST" << std::endl;
    llvm::Value* left = visiter->visit(lhs);
    llvm::Value* right = visiter->visit(rhs);
    switch (op) {
    case '+':
        return visiter->builder->CreateFAdd(left, right, "addtmp");
    case '*':
        return visiter->builder->CreateFMul(left, right, "multmp");
    }
    return nullptr;
}

Expr operator+ (Expr lhs, Expr rhs) {
    std::shared_ptr<ExprAST> p(new BinaryExprAST('+', lhs, rhs));
    return Expr(p);
}

Expr operator* (Expr lhs, Expr rhs) {
    std::shared_ptr<ExprAST> p(new BinaryExprAST('*', lhs, rhs));
    return Expr(p);
}

void BinaryExprAST::dump(int level) {
    for (int i = 0; i < level; i++) { std::cout << "-"; }
    std::cout << "BinaryExprAST" << std::endl;
    lhs.value->dump(level + 1);
    rhs.value->dump(level + 1);
}

class Var {
 public:
    // static std::vector<std::string> used_names;
    static int name_count;
    std::string name;
 public:
    Var() {
        name = "var" + std::to_string(name_count++);
    }
    operator Expr() const {
        std::shared_ptr<ExprAST> p(new VarExprAST(name));
        return Expr(p);
    }
    // ~Var() { std::cout << "Var is deleted." << std::endl; }
};

int Var::name_count = 0;

template <typename... Args>
void IRVisitor::set_arguments(Args&&... args) {

    using llvm::Function;
    using llvm::FunctionType;
    using llvm::BasicBlock;
    using llvm::Type;

    std::vector<Var> collected_args{std::forward<Args>(args)...};

    std::vector<Type *> Doubles(collected_args.size(), Type::getDoubleTy(TheContext));
    FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    std::string name = "hoge";
    function = Function::Create(funcType, Function::ExternalLinkage, name, module.get());

    // Set names for all arguments.
    unsigned i = 0;
    for (auto &arg : function->args()) {
        Var a = collected_args[i++];
        std::cout << a.name << std::endl;
        arg.setName(a.name);
    }

    // Create a new basic block to start insertion into.
    BasicBlock *basicBlock = BasicBlock::Create(TheContext, "entry", function);
    builder->SetInsertPoint(basicBlock);

    // Record the function arguments in the NamedValues map.
    name2Value.clear();
    for (auto &arg : function->args())
        name2Value[std::string(arg.getName())] = &arg;
}

class Execution {
 public:
    llvm::ExecutionEngine *engineBuilder;
    std::string functionName;
 public:
    Execution(std::unique_ptr<llvm::Module>, std::string);
    uint64_t getFunctionAddress();
};

Execution::Execution(std::unique_ptr<llvm::Module> module, std::string name) {
    // Builder JIT
    functionName = name;
    std::string errStr;
    engineBuilder = llvm::EngineBuilder(std::move(module))
        .setEngineKind(llvm::EngineKind::JIT)
        .setErrorStr(&errStr)
        .create();
    if (!engineBuilder) {
        std::cout << "error: " << errStr << std::endl;
    }
}

uint64_t Execution::getFunctionAddress() {
    return engineBuilder->getFunctionAddress(functionName);
}

double hoge(double a, double b) {
    return a * b;
}

void IRVisitor::realise(Expr expr) {
    using llvm::Function;
    using llvm::FunctionType;
    using llvm::Type;

    llvm::Value *RetVal = visit(expr);

    std::cout << RetVal << std::endl;


    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> Doubles(1, Type::getDoubleTy(TheContext));
    FunctionType *FT =
        FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *F =
        Function::Create(FT, Function::ExternalLinkage, "cos", module.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName("x");

    auto arg1 = llvm::ConstantFP::get(TheContext, llvm::APFloat(3.14/3.0));

    std::vector<llvm::Value *> ArgsV;

    ArgsV.push_back(arg1);

    Function *TheFunction = module->getFunction("cos");

    auto hoge = builder->CreateCall(TheFunction, ArgsV, "tmphoge");

    builder->CreateRet(hoge);

    if (verifyFunction(*function)) {
        std::cout << ": Error constructing function!\n" << std::endl;
        return;
    }

    if (verifyModule(*module)) {
        std::cout << ": Error module!\n" << std::endl;
        return;
    }

    module->print(llvm::outs(), nullptr);


    // GenericValue GV = execution->engineBuilder->runFunction(function, Args);

    // std::vector<Type *> Doubles(2, Type::getDoubleTy(TheContext));
    // FunctionType *funcType = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    // llvm::FunctionCallee c = module->getOrInsertFunction("hoge", funcType);

    // regist cos


    execution = new Execution(std::move(module), "hoge");
    // c.getCallee()
    // module->getOrInsertFunction("atan2", Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext), Type::getDoubleTy(TheContext));

    // Type::getDoubleTy

    // auto f = getfun

    // builder->CreateCall(c, Args, "tmphoge");

    // using Func =  int(Ts...);
    auto p = execution->getFunctionAddress();
    auto functionPointer = reinterpret_cast<double(*)(double, double)>(p);
    std::cout << (*functionPointer)(10, 10) << std::endl;
}

int main() {
    Var a, b;

    IRVisitor* visitor = new IRVisitor();

    visitor->set_arguments(a, b);

    Expr d = a + b;

    d.value->dump();
    visitor->realise(d);

    delete visitor;

    return 0;
}

