#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Instruction.h>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace llvm;

enum Token {
    token_error = 0x4000,
    token_eof = 0x2000,
    token_ident = 0x1000,
    token_number = 0x0800,
    token_if = 0x0400,
    token_for = 0x0200,
    token_return = 0x0100,
    token_lparen = 0x0080,
    token_rparen = 0x0040,
    token_lfig = 0x0020,
    token_rfig = 0x0010,
    token_comma = 0x0008,
    token_eq = 0x0004,
    token_plus = 0x0002,
    token_minus = 0x0001,
    token_else = 0x0000
};

static std::string input_seq;
static int current_position = 0;
static std::string StrVal;
static int NumVal;

static char next_char() {
    if (current_position >= input_seq.length())
        return '$';
    return input_seq[current_position++];
}

static Token get_token() {
    static char LastChar = ' ';
    while (isspace(LastChar)) LastChar = next_char();
    if (isalpha(LastChar)) {
        StrVal = LastChar;
        while (isalnum((LastChar = next_char())))
            StrVal += LastChar;
        if (StrVal == "if")    return token_if;
        if (StrVal == "for")   return token_for;
        if (StrVal == "return")return token_return;
        if (StrVal == "else")  return token_else;
        return token_ident;
    }
    if (isdigit(LastChar)) {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = next_char();
        } while (isdigit(LastChar));
        NumVal = strtol(NumStr.c_str(), nullptr, 10);
        return token_number;
    }
    Token t = token_error;
    switch (LastChar) {
        case '$': t = token_eof; break;
        case '(': t = token_lparen; break;
        case ')': t = token_rparen; break;
        case '{': t = token_lfig; break;
        case '}': t = token_rfig; break;
        case ',': t = token_comma; break;
        case '=': t = token_eq;    break;
        case '+': t = token_plus;  break;
        case '-': t = token_minus; break;
    }
    LastChar = next_char();
    return t;
}

static Token current_token = token_eof;
static Token getNextToken() { return current_token = get_token(); }

void init_scanner(const char* filepath) {
    current_position = 0;
    std::ifstream t(filepath);
    input_seq = std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

static std::unique_ptr<LLVMContext> context;
static std::unique_ptr<Module> mod;
static std::unique_ptr<IRBuilder<>> builder;
static std::map<std::string, AllocaInst*> named_values;

static void initialize_module() {
    context = std::make_unique<LLVMContext>();
    mod = std::make_unique<Module>("lab3", *context);
    builder = std::make_unique<IRBuilder<>>(*context);
}

class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    int Val;
public:
    NumberExprAST(int Val) : Val(Val) {}
    Value *codegen() override;
};

class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    const std::string &getName() const { return Name; }
    Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    Value *codegen() override;
};

class IfExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> Cond;
    std::vector<std::unique_ptr<ExprAST>> Then, Else;
    IfExprAST(std::unique_ptr<ExprAST> Cond,
              std::vector<std::unique_ptr<ExprAST>> Then,
              std::vector<std::unique_ptr<ExprAST>> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    Value *codegen() override;
};

class ForExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Start, End, Step;
    std::vector<std::unique_ptr<ExprAST>> Body;
public:
    ForExprAST(std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End,
               std::unique_ptr<ExprAST> Step,
               std::vector<std::unique_ptr<ExprAST>> Body)
        : Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)), Body(std::move(Body)) {}
    Value *codegen() override;
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}
    Function *codegen();
    const std::string &getName() const { return Name; }
};

class FunctionAST {
public:
    std::unique_ptr<PrototypeAST> proto;
    std::vector<std::unique_ptr<ExprAST>> body;
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::vector<std::unique_ptr<ExprAST>> body)
        : proto(std::move(proto)), body(std::move(body)) {}
    Function *codegen();
};

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getInt32Ty(*context), nullptr, VarName);
}

Value *NumberExprAST::codegen() {
    return ConstantInt::get(*context, APInt(32, Val, false));
}

Value *VariableExprAST::codegen() {
    AllocaInst *A = named_values[Name];
    if (!A)
        A = CreateEntryBlockAlloca(builder->GetInsertBlock()->getParent(), Name);
    named_values[Name] = A;
    return builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

Value *BinaryExprAST::codegen() {
    if (Op == '=') {
        auto *LHSE = static_cast<VariableExprAST*>(LHS.get());
        Value *Val = RHS->codegen();
        AllocaInst *Variable = named_values[LHSE->getName()];
        if (!Variable) {
            Variable = CreateEntryBlockAlloca(builder->GetInsertBlock()->getParent(), LHSE->getName());
            named_values[LHSE->getName()] = Variable;
        }
        builder->CreateStore(Val, Variable);
        return Val;
    }
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    switch (Op) {
        case '+': return builder->CreateAdd(L, R, "addtmp");
        case '-': return builder->CreateSub(L, R, "subtmp");
        default:  return nullptr;
    }
}

Value *IfExprAST::codegen() {
    Value *CondV = Cond->codegen();
    CondV = builder->CreateICmpNE(CondV,
        ConstantInt::get(*context, APInt(32, 0, false)), "ifcond");
    Function *TheFunction = builder->GetInsertBlock()->getParent();

    BasicBlock *ThenBB  = BasicBlock::Create(*context, "then",  TheFunction);
    BasicBlock *ElseBB  = BasicBlock::Create(*context, "else",  TheFunction);
    BasicBlock *MergeBB = BasicBlock::Create(*context, "ifcont", TheFunction);

    builder->CreateCondBr(CondV, ThenBB, ElseBB);

    builder->SetInsertPoint(ThenBB);
    for (auto &expr : Then)
        expr->codegen();
    builder->CreateBr(MergeBB);

    builder->SetInsertPoint(ElseBB);
    for (auto &expr : Else)
        expr->codegen();
    builder->CreateBr(MergeBB);

    builder->SetInsertPoint(MergeBB);
    return nullptr;
}

Value *ForExprAST::codegen() {
    Function *TheFunction = builder->GetInsertBlock()->getParent();
    Value *StartVal = Start->codegen();
    BasicBlock *LoopBB = BasicBlock::Create(*context, "loop", TheFunction);
    builder->CreateBr(LoopBB);

    builder->SetInsertPoint(LoopBB);
    for (auto &expr : Body)
        expr->codegen();

    Value *StepVal = Step ? Step->codegen()
                           : ConstantInt::get(*context, APInt(32, 1, false));
    Value *EndCond = End->codegen();
    EndCond = builder->CreateICmpNE(EndCond,
        ConstantInt::get(*context, APInt(32, 0, false)), "loopcond");

    BasicBlock *AfterBB = BasicBlock::Create(*context, "afterloop", TheFunction);
    builder->CreateCondBr(EndCond, LoopBB, AfterBB);
    builder->SetInsertPoint(AfterBB);
    return Constant::getNullValue(Type::getInt32Ty(*context));
}

Function *PrototypeAST::codegen() {
    std::vector<Type*> Ints(Args.size(), Type::getInt32Ty(*context));
    FunctionType *FT = FunctionType::get(Type::getInt32Ty(*context), Ints, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, mod.get());
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);
    return F;
}

Function *FunctionAST::codegen() {
    Function *TheFunction = proto->codegen();
    BasicBlock *BB = BasicBlock::Create(*context, "entry", TheFunction);
    builder->SetInsertPoint(BB);

    named_values.clear();
    for (auto &Arg : TheFunction->args()) {
        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());
        builder->CreateStore(&Arg, Alloca);
        named_values[std::string(Arg.getName())] = Alloca;
    }

    Value *RetVal = nullptr;
    for (auto &expr : body)
        RetVal = expr->codegen();
    builder->CreateRet(RetVal);
    verifyFunction(*TheFunction);
    return TheFunction;
}

void assertion(Token t) {
    if (current_token != t) throw std::runtime_error("Parsing error");
}

std::vector<std::string> ParseVars();
std::vector<std::unique_ptr<ExprAST>> ParseBody();
std::unique_ptr<ExprAST> ParseExpr();
std::unique_ptr<ExprAST> ParseBinOp();
std::unique_ptr<ExprAST> ParseEq();

FunctionAST* Parse() {
    getNextToken();
    assertion(token_ident);
    std::string name = StrVal;
    getNextToken();
    auto args = ParseVars();
    auto proto = std::make_unique<PrototypeAST>(name, args);
    auto body = ParseBody();
    assertion(token_eof);
    return new FunctionAST(std::move(proto), std::move(body));
}

std::vector<std::string> ParseVars() {
    std::vector<std::string> args;
    assertion(token_lparen);
    getNextToken();
    while (current_token != token_rparen && current_token != token_eof) {
        assertion(token_ident);
        args.push_back(StrVal);
        getNextToken();
        if (current_token == token_comma) getNextToken();
    }
    getNextToken();
    return args;
}

std::vector<std::unique_ptr<ExprAST>> ParseBody() {
    assertion(token_lfig);
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> exprs;
    while (current_token==token_ident || current_token==token_for || current_token==token_if)
        exprs.push_back(ParseExpr());
    assertion(token_return);
    getNextToken();
    exprs.push_back(ParseBinOp());
    assertion(token_rfig);
    getNextToken();
    return exprs;
}

std::unique_ptr<ExprAST> ParseEq() {
    assertion(token_ident);
    auto LHS = std::make_unique<VariableExprAST>(StrVal);
    getNextToken(); assertion(token_eq); getNextToken();
    auto RHS = ParseBinOp();
    return std::make_unique<BinaryExprAST>('=', std::move(LHS), std::move(RHS));
}

std::unique_ptr<ExprAST> ParseExpr() {
    if (current_token==token_ident) return ParseEq();
    if (current_token==token_for) {
        getNextToken(); assertion(token_lparen); getNextToken();
        auto start = ParseEq(); assertion(token_comma); getNextToken();
        auto cond  = ParseBinOp(); assertion(token_comma); getNextToken();
        auto step  = ParseEq(); assertion(token_rparen); getNextToken();
        assertion(token_lfig); getNextToken();
        std::vector<std::unique_ptr<ExprAST>> body;
        while (current_token!=token_rfig) body.push_back(ParseEq());
        getNextToken();
        return std::make_unique<ForExprAST>(std::move(start), std::move(cond), std::move(step), std::move(body));
    }
    if (current_token==token_if) {
        getNextToken(); assertion(token_lparen); getNextToken();
        auto cond = ParseBinOp(); assertion(token_rparen); getNextToken();
        assertion(token_lfig); getNextToken();
        std::vector<std::unique_ptr<ExprAST>> thenBody;
        while (current_token!=token_rfig) thenBody.push_back(ParseEq());
        getNextToken(); assertion(token_else); getNextToken();
        assertion(token_lfig); getNextToken();
        std::vector<std::unique_ptr<ExprAST>> elseBody;
        while (current_token!=token_rfig) elseBody.push_back(ParseEq());
        getNextToken();
        return std::make_unique<IfExprAST>(std::move(cond), std::move(thenBody), std::move(elseBody));
    }
    return nullptr;
}

std::unique_ptr<ExprAST> ParseBinOp() {
    if (current_token==token_ident) {
        std::string var = StrVal;
        getNextToken();
        if (current_token!=token_plus && current_token!=token_minus)
            return std::make_unique<VariableExprAST>(var);
        char op = current_token==token_plus ? '+' : '-';
        getNextToken();
        return std::make_unique<BinaryExprAST>(op, std::make_unique<VariableExprAST>(var), ParseBinOp());
    } else if (current_token==token_number) {
        int number = NumVal;
        getNextToken();
        if (current_token!=token_plus && current_token!=token_minus)
            return std::make_unique<NumberExprAST>(number);
        char op = current_token==token_plus ? '+' : '-';
        getNextToken();
        return std::make_unique<BinaryExprAST>(op, std::make_unique<NumberExprAST>(number), ParseBinOp());
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc==2) {
        init_scanner(argv[1]);
        initialize_module();
        std::unique_ptr<FunctionAST> f(Parse());
        f->codegen();
        mod->print(errs(), nullptr);
    } else {
        std::cerr << "Usage: " << argv[0] << " <input file>\n";
    }
    return 0;
}
