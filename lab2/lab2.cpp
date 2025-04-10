#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main() {
    // Создаем контекст для всех объектов LLVM
    LLVMContext Context;
    // Создаем модуль с именем "llvm_lab2" в данном контексте
    auto *ModuleOb = new Module("llvm_lab2", Context);
    // IRBuilder помогает нам последовательно генерировать инструкции
    IRBuilder<> Builder(Context);

    // Определяем тип функции main, возвращающей int без аргументов
    FunctionType *FuncType = FunctionType::get(Builder.getInt32Ty(), false);
    // Создаем функцию main с внешним связыванием
    Function *MainFunc = Function::Create(FuncType, Function::ExternalLinkage, "main", ModuleOb);

    // Создаем базовый блок (entry), куда будут вставляться инструкции
    BasicBlock *EntryBB = BasicBlock::Create(Context, "entry", MainFunc);
    Builder.SetInsertPoint(EntryBB);

    // Определяем две константы типа i32: 353 и 48
    Value *Const353 = ConstantInt::get(Builder.getInt32Ty(), 353);
    Value *Const48  = ConstantInt::get(Builder.getInt32Ty(), 48);

    // Генерируем инструкцию сложения констант. Результат сохраняем в переменную "sum"
    Value *Sum = Builder.CreateAdd(Const353, Const48, "sum");

    // Создаем инструкцию возврата полученного результата из функции main
    Builder.CreateRet(Sum);

    // Опционально: проверяем корректность сгенерированного LLVM IR
    verifyFunction(*MainFunc);

    // Выводим сгенерированный LLVM IR на стандартный вывод
    ModuleOb->print(outs(), nullptr);

    // Освобождаем память, выделенную для модуля
    delete ModuleOb;
    return 0;
}
