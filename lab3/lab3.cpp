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
#include <memory>
#include <stdexcept>

using namespace llvm;
x enum Token {
    kTokenError = 0x4000,  // Ошибка лексического анализа
    kTokenEof = 0x2000,    // Конец файла (End Of File)
    kTokenIdent = 0x1000,  // Идентификатор (имя переменной/функции)
    kTokenNumber = 0x0800, // Целочисленное число
    kTokenIf = 0x0400,     // Ключевое слово 'if'
    kTokenFor = 0x0200,    // Ключевое слово 'for'
    kTokenReturn = 0x0100, // Ключевое слово 'return'
    kTokenLParen = 0x0080, // Левая круглая скобка '('
    kTokenRParen = 0x0040, // Правая круглая скобка ')'
    kTokenLFig = 0x0020,   // Левая фигурная скобка '{'
    kTokenRFig = 0x0010,   // Правая фигурная скобка '}'
    kTokenComma = 0x0008,  // Запятая ','
    kTokenEq = 0x0004,     // Знак равенства '='
    kTokenPlus = 0x0002,   // Знак сложения '+'
    kTokenMinus = 0x0001,  // Знак вычитания '-'
    kTokenElse = 0x0000    // Ключевое слово 'else'
};

// Глобальные переменные для лексера
static std::string g_input_seq;    // Содержимое входного файла
static int g_current_position = 0; // Текущая позиция чтения в g_input_seq
static std::string g_str_val;      // Хранит значение последнего прочитанного идентификатора
static int g_num_val;              // Хранит значение последнего прочитанного числа

// Возвращает следующий символ из входной строки или '$' при достижении конца
static char next_char()
{
    if (g_current_position >= g_input_seq.length())
    {
        return '$'; // Специальный символ, обозначающий конец ввода
    }
    return g_input_seq[g_current_position++];
}

// Основная функция лексера: читает символы и возвращает следующий токен
static Token get_token()
{
    static char last_char = ' '; // переменная для хранения последнего непрочитанного символа

    // Пропускаем пробельные символы
    while (isspace(last_char))
    {
        last_char = next_char();
    }

    // Распознавание идентификаторов и ключевых слов
    if (isalpha(last_char))
    {
        g_str_val = last_char;
        while (isalnum((last_char = next_char())))
        {
            g_str_val += last_char;
        }
        // Проверка на ключевые слова
        if (g_str_val == "if")
            return kTokenIf;
        if (g_str_val == "for")
            return kTokenFor;
        if (g_str_val == "return")
            return kTokenReturn;
        if (g_str_val == "else")
            return kTokenElse;
        return kTokenIdent; // Если не ключевое слово, то это идентификатор
    }

    // Распознавание чисел
    if (isdigit(last_char))
    {
        std::string num_str;
        do
        {
            num_str += last_char;
            last_char = next_char();
        } while (isdigit(last_char));
        // Преобразуем строку в число (int)
        g_num_val = strtol(num_str.c_str(), nullptr, 10);
        return kTokenNumber;
    }

    // Распознавание символов
    Token t = kTokenError; // По умолчанию - ошибка
    switch (last_char)
    {
    case '$':
        t = kTokenEof;
        break;
    case '(':
        t = kTokenLParen;
        break;
    case ')':
        t = kTokenRParen;
        break;
    case '{':
        t = kTokenLFig;
        break;
    case '}':
        t = kTokenRFig;
        break;
    case ',':
        t = kTokenComma;
        break;
    case '=':
        t = kTokenEq;
        break;
    case '+':
        t = kTokenPlus;
        break;
    case '-':
        t = kTokenMinus;
        break;
        // Любой другой символ считается ошибкой
    }

    // Читаем следующий символ для следующего вызова get_token
    last_char = next_char();
    return t;
}

// Глобальная переменная для хранения текущего токена
static Token g_current_token = kTokenEof;

// Обновляет g_current_token следующим токеном из потока
static Token getNextToken()
{
    return g_current_token = get_token();
}

// читаем весь файл в строку
void init_scanner(const char *filepath)
{
    g_current_position = 0;
    std::ifstream t(filepath);
    if (!t)
    {
        std::cerr << "Ошибка: не удалось открыть файл " << filepath << std::endl;
        exit(1); // Простой выход в случае ошибки
    }
    // Читаем весь файл в строку g_input_seq
    g_input_seq = std::string((std::istreambuf_iterator<char>(t)),
                              std::istreambuf_iterator<char>());
}

// Глобальные объекты LLVM
static std::unique_ptr<LLVMContext> g_context; // Контекст LLVM, хранит основные структуры данных
static std::unique_ptr<Module> g_module;       // Модуль LLVM, аналог единицы трансляции
static std::unique_ptr<IRBuilder<>> g_builder; // Помощник для генерации инструкций LLVM IR

static std::map<std::string, AllocaInst *> g_named_values; // Таблица символов

// Инициализация глобальных объектов LLVM
static void initialize_module()
{
    g_context = std::make_unique<LLVMContext>();
    // Создаем модуль с именем "lab3" в текущем контексте
    g_module = std::make_unique<Module>("lab3", *g_context);
    // Создаем IRBuilder для генерации кода
    g_builder = std::make_unique<IRBuilder<>>(*g_context);
}

class ExprAST
{
public:
    virtual ~ExprAST() = default;
    virtual Value *codegen() = 0; // Виртуальный метод для генерации LLVM IR для данного узла AST
};

// Узел AST для числовых литералов
class NumberExprAST : public ExprAST
{
    int m_val;

public:
    NumberExprAST(int val) : m_val(val) {}
    Value *codegen() override;
};

// Узел AST для ссылок на переменные
class VariableExprAST : public ExprAST
{
    std::string m_name;

public:
    VariableExprAST(const std::string &name) : m_name(name) {}
    const std::string &getName() const { return m_name; }
    Value *codegen() override;
};

// Узел AST для бинарных операций 
class BinaryExprAST : public ExprAST
{
    char m_op;                             
    std::unique_ptr<ExprAST> m_lhs, m_rhs; 

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs)
        : m_op(op), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}
    Value *codegen() override;
};

// Узел AST для выражения 'if-else'
class IfExprAST : public ExprAST
{
public:
    std::unique_ptr<ExprAST> m_cond;              // Условие
    std::vector<std::unique_ptr<ExprAST>> m_then; // Блок 'then' 
    std::vector<std::unique_ptr<ExprAST>> m_else; // Блок 'else' 

    IfExprAST(std::unique_ptr<ExprAST> cond,
              std::vector<std::unique_ptr<ExprAST>> then_block,
              std::vector<std::unique_ptr<ExprAST>> else_block)
        : m_cond(std::move(cond)), m_then(std::move(then_block)), m_else(std::move(else_block)) {}
    Value *codegen() override;
};

// Узел AST для цикла for(init, cond, step) { body }
class ForExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> m_start;             // Выражение инициализации 
    std::unique_ptr<ExprAST> m_end;               // Выражение условия 
    std::unique_ptr<ExprAST> m_step;              // Выражение шага 
    std::vector<std::unique_ptr<ExprAST>> m_body; // Тело цикла 

public:
    ForExprAST(std::unique_ptr<ExprAST> start,
               std::unique_ptr<ExprAST> end,
               std::unique_ptr<ExprAST> step,
               std::vector<std::unique_ptr<ExprAST>> body)
        : m_start(std::move(start)), m_end(std::move(end)), m_step(std::move(step)), m_body(std::move(body)) {}
    Value *codegen() override;
};

// Узел AST для прототипа функции
class PrototypeAST
{
    std::string m_name;              // Имя функции
    std::vector<std::string> m_args; // Имена аргументов

public:
    PrototypeAST(const std::string &name, std::vector<std::string> args)
        : m_name(name), m_args(std::move(args)) {}
    Function *codegen();
    const std::string &getName() const { return m_name; }
};

// Узел AST для определения функции 
class FunctionAST
{
public:
    std::unique_ptr<PrototypeAST> m_proto;        // Прототип функции
    std::vector<std::unique_ptr<ExprAST>> m_body; // Тело функции (список выражений)

    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::vector<std::unique_ptr<ExprAST>> body)
        : m_proto(std::move(proto)), m_body(std::move(body)) {}
    Function *codegen(); // Генерирует определение функции LLVM
};

// Генерация кода (Codegen)

// Вспомогательная функция для создания инструкции 'alloca' в начале функции.
// 'alloca' выделяет память на стеке для локальной переменной.
static AllocaInst *CreateEntryBlockAlloca(Function *the_function, StringRef var_name)
{
    // Создаем временный IRBuilder, который вставляет инструкции в начало входного блока функции
    IRBuilder<> tmpB(&the_function->getEntryBlock(), the_function->getEntryBlock().begin());
    // Создаем инструкцию alloca для 32-битного целого числа
    return tmpB.CreateAlloca(Type::getInt32Ty(*g_context), nullptr, var_name);
}

// Генерация кода для числового литерала
Value *NumberExprAST::codegen()
{
    // Создаем константу LLVM типа i32
    return ConstantInt::get(*g_context, APInt(32, m_val, false)); // false - знаковое число
}

// Генерация кода для ссылки на переменную
Value *VariableExprAST::codegen()
{
    // Ищем переменную в таблице символов
    AllocaInst *a = g_named_values[m_name];
    if (!a)
    {
        std::cerr << "Предупреждение: переменная '" << m_name << "' используется до присваивания?" << std::endl;
        Function *the_function = g_builder->GetInsertBlock()->getParent();
        a = CreateEntryBlockAlloca(the_function, m_name);
        g_named_values[m_name] = a;
    }
    // Создаем инструкцию load для чтения значения из памяти (alloca)
    return g_builder->CreateLoad(a->getAllocatedType(), a, m_name.c_str());
}

// Генерация кода для бинарного оператора
Value *BinaryExprAST::codegen()
{
    // Особый случай: оператор присваивания '='
    if (m_op == '=')
    {
        // Левый операнд должен быть переменной (VariableExprAST)
        VariableExprAST *lhs_e = static_cast<VariableExprAST *>(m_lhs.get());
        if (!lhs_e)
        {
            throw std::runtime_error("Ошибка: левая часть присваивания должна быть переменной");
        }

        // Генерируем код для правого операнда (вычисляем значение для присваивания)
        Value *val = m_rhs->codegen();
        if (!val)
        {
            return nullptr; // Ошибка при генерации RHS
        }

        // Ищем аллокацию для переменной в таблице символов
        AllocaInst *variable = g_named_values[lhs_e->getName()];
        if (!variable)
        {
            // Если переменной нет, создаем для нее 'alloca' в начале функции
            Function *the_function = g_builder->GetInsertBlock()->getParent();
            variable = CreateEntryBlockAlloca(the_function, lhs_e->getName());
            g_named_values[lhs_e->getName()] = variable;
        }
        // Создаем инструкцию store для записи значения в память
        g_builder->CreateStore(val, variable);
        return val;
    }

    Value *l = m_lhs->codegen(); // Генерируем код для левого операнда
    Value *r = m_rhs->codegen(); // Генерируем код для правого операнда
    if (!l || !r)
    {
        return nullptr; // Ошибка при генерации операндов
    }

    switch (m_op)
    {
    case '+':
        return g_builder->CreateAdd(l, r, "addtmp");
    case '-':
        return g_builder->CreateSub(l, r, "subtmp");
    default:
        throw std::runtime_error("Неизвестный бинарный оператор");
        return nullptr; 
    }
}

// Генерация кода для выражения 'if-else'
Value *IfExprAST::codegen()
{
    // 1. Генерируем код для условия
    Value *cond_v = m_cond->codegen();
    if (!cond_v)
        return nullptr;

    // 2. Сравниваем результат условия с нулем.
    // В C-подобных языках, 0 - ложь, не 0 - истина.
    cond_v = g_builder->CreateICmpNE(cond_v,
                                     ConstantInt::get(*g_context, APInt(32, 0, false)), "ifcond");

    // Получаем текущую функцию, в которую добавляется код
    Function *the_function = g_builder->GetInsertBlock()->getParent();

    // 3. Создаем базовые блоки для веток 'then', 'else' и для слияния после 'if'
    BasicBlock *then_bb = BasicBlock::Create(*g_context, "then", the_function);
    BasicBlock *else_bb = BasicBlock::Create(*g_context, "else");    
    BasicBlock *merge_bb = BasicBlock::Create(*g_context, "ifcont"); 

    // 4. Создаем инструкцию условного перехода
    g_builder->CreateCondBr(cond_v, then_bb, else_bb);

    // 5. Генерируем код для блока 'then'
    g_builder->SetInsertPoint(then_bb); // Переключаем IRBuilder на блок 'then'
    // Генерируем код для каждого выражения в блоке 'then'
    for (const auto &expr : m_then)
    {
        expr->codegen(); // Результат выражений внутри if/else игнорируется
    }
    g_builder->CreateBr(merge_bb); // В конце 'then' переходим к блоку слияния

    // 6. Генерируем код для блока 'else'
    the_function->insert(the_function->end(), else_bb); // Добавляем блок 'else' в функцию
    g_builder->SetInsertPoint(else_bb);                 // Переключаем IRBuilder на блок 'else'
    for (const auto &expr : m_else)
    {
        expr->codegen(); // Результат выражений внутри if/else игнорируется
    }
    g_builder->CreateBr(merge_bb); // В конце 'else' переходим к блоку слияния

    // 7. Код после 'if-else' будет генерироваться в блоке слияния
    the_function->insert(the_function->end(), merge_bb); // Добавляем блок слияния в функцию
    g_builder->SetInsertPoint(merge_bb);                 // Переключаем IRBuilder на блок слияния

    return nullptr;
}

// Генерация кода для цикла 'for'
Value *ForExprAST::codegen()
{
    // Получаем текущую функцию
    Function *the_function = g_builder->GetInsertBlock()->getParent();

    // 1. Генерируем код для инициализации цикла (выполняется один раз перед циклом)
    // Результат инициализации (если он есть) игнорируется в контексте управления циклом.
    Value *start_val = m_start->codegen();
    if (!start_val)
        return nullptr;

    // 2. Создаем базовый блок для тела цикла ('loop') и блок для кода после цикла ('afterloop')
    BasicBlock *loop_bb = BasicBlock::Create(*g_context, "loop", the_function);
    BasicBlock *after_bb = BasicBlock::Create(*g_context, "afterloop"); 

    // 3. Переходим из текущего блока в начало блока цикла
    g_builder->CreateBr(loop_bb);

    // 4. Генерируем код для тела цикла
    g_builder->SetInsertPoint(loop_bb); // Переключаем IRBuilder на блок 'loop'
    for (const auto &expr : m_body)
    {
        expr->codegen(); // Результаты выражений в теле цикла игнорируются
    }

    // 5. Генерируем код для шага цикла (step)
    Value *step_val = nullptr;
    if (m_step)
    {
        step_val = m_step->codegen(); // Генерируем код для выражения шага
        if (!step_val)
            return nullptr;
    }

    // 6. Генерируем код для условия выхода из цикла
    Value *end_cond = m_end->codegen(); 
    if (!end_cond)
        return nullptr;

    // Сравниваем результат условия с 0 (продолжаем цикл, если не 0)
    end_cond = g_builder->CreateICmpNE(end_cond,
                                       ConstantInt::get(*g_context, APInt(32, 0, false)), "loopcond");

    // 7. Создаем условный переход в конце тела цикла
    g_builder->CreateCondBr(end_cond, loop_bb, after_bb);

    // 8. Добавляем блок 'afterloop' в функцию и устанавливаем точку вставки туда
    the_function->insert(the_function->end(), after_bb);
    g_builder->SetInsertPoint(after_bb);

    return Constant::getNullValue(Type::getInt32Ty(*g_context));
}

// Генерация кода для прототипа функции (объявление)
Function *PrototypeAST::codegen()
{
    // Создаем вектор типов аргументов (все int32)
    std::vector<Type *> arg_types(m_args.size(), Type::getInt32Ty(*g_context));

    // Создаем тип функции: int32(int32, int32, ...)
    FunctionType *ft = FunctionType::get(Type::getInt32Ty(*g_context), arg_types, false); // false - не вариативная

    // Создаем функцию LLVM
    Function *f = Function::Create(ft, Function::ExternalLinkage, m_name, g_module.get());

    // Устанавливаем имена для аргументов функции LLVM
    unsigned idx = 0;
    for (auto &arg : f->args())
    {
        arg.setName(m_args[idx++]);
    }
    return f;
}

// Генерация кода для определения функции (прототип + тело)
Function *FunctionAST::codegen()
{
    // Сначала генерируем прототип функции
    Function *the_function = m_proto->codegen();
    if (!the_function)
    {
        return nullptr;
    }

    // Создаем базовый блок "entry" для начала выполнения функции
    BasicBlock *bb = BasicBlock::Create(*g_context, "entry", the_function);
    g_builder->SetInsertPoint(bb); // Устанавливаем точку вставки IRBuilder в этот блок

    // Очищаем таблицу символов для локальных переменных этой функции
    g_named_values.clear();

    for (auto &arg : the_function->args())
    {
        AllocaInst *alloca = CreateEntryBlockAlloca(the_function, arg.getName());
        g_builder->CreateStore(&arg, alloca);
        g_named_values[std::string(arg.getName())] = alloca;
    }

    // Генерируем код для тела функции
    Value *ret_val = nullptr;
    for (const auto &expr : m_body)
    {
        ret_val = expr->codegen(); // Генерируем код для каждого выражения
    }

    // Завершаем функцию инструкцией 'ret'
    if (ret_val)
    {
        g_builder->CreateRet(ret_val); // Возвращаем результат последнего выражения
    }
    else
    {
        std::cerr << "Предупреждение: нет значения для возврата в функции " << m_proto->getName() << std::endl;
        // Вставим возврат 0 по умолчанию, чтобы IR был валидным
        ret_val = ConstantInt::get(*g_context, APInt(32, 0, false));
        g_builder->CreateRet(ret_val);
    }

    // Проверяем сгенерированную функцию на корректность
    verifyFunction(*the_function);

    return the_function;
}

// Парсер 

void assertion(Token expected_token)
{
    if (g_current_token != expected_token)
    {
        throw std::runtime_error("Ошибка синтаксического анализа: неожиданный токен");
    }
}

// Прототипы функций парсера (для взаимной рекурсии)
std::vector<std::string> ParseVars();              // Парсинг списка аргументов функции
std::vector<std::unique_ptr<ExprAST>> ParseBody(); // Парсинг тела функции
std::unique_ptr<ExprAST> ParseExpr();              // Парсинг одного выражения/оператора (if, for, присваивание)
std::unique_ptr<ExprAST> ParseBinOp();             // Парсинг бинарного выражения (+, -) или переменной/числа
std::unique_ptr<ExprAST> ParseEq();                // Парсинг выражения присваивания

// Главная функция парсера: парсит всю программу 
// program ::= ident '(' vars ')' '{' body '}'
FunctionAST *Parse()
{
    getNextToken(); // Получаем первый токен

    // Ожидаем имя функции
    assertion(kTokenIdent);
    std::string func_name = g_str_val;
    getNextToken();

    // Парсим список аргументов
    auto args = ParseVars();
    // Создаем узел AST для прототипа функции
    auto proto = std::make_unique<PrototypeAST>(func_name, std::move(args));

    // Парсим тело функции
    auto body = ParseBody();

    // Ожидаем конец файла после тела функции
    assertion(kTokenEof);

    return new FunctionAST(std::move(proto), std::move(body));
}

// Парсинг списка аргументов функции: '(' [ident (',' ident)*] ')'
std::vector<std::string> ParseVars()
{
    std::vector<std::string> args;
    assertion(kTokenLParen); // Ожидаем '('
    getNextToken();

    // Пока не встретим ')' или конец файла
    while (g_current_token != kTokenRParen && g_current_token != kTokenEof)
    {
        assertion(kTokenIdent); // Ожидаем идентификатор (имя аргумента)
        args.push_back(g_str_val);
        getNextToken();
        // Если следующий токен - запятая, пропускаем ее
        if (g_current_token == kTokenComma)
        {
            getNextToken();
        }
        else if (g_current_token != kTokenRParen)
        {
            // Если не запятая и не ')', то ошибка
            throw std::runtime_error("Ошибка синтаксического анализа: ожидалась ',' или ')' в списке аргументов");
        }
    }
    assertion(kTokenRParen); // Ожидаем ')'
    getNextToken();
    return args;
}

// Парсинг тела функции: '{' (expr)* 'return' binop '}'
std::vector<std::unique_ptr<ExprAST>> ParseBody()
{
    assertion(kTokenLFig); // Ожидаем '{'
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> exprs;

    // Парсим выражения/операторы (if, for, присваивание) до ключевого слова 'return'
    while (g_current_token == kTokenIdent || g_current_token == kTokenFor || g_current_token == kTokenIf)
    {
        exprs.push_back(ParseExpr());
    }

    assertion(kTokenReturn);
    getNextToken();

    exprs.push_back(ParseBinOp());

    assertion(kTokenRFig);
    getNextToken();
    return exprs;
}

// Парсинг выражения присваивания: ident '=' binop
std::unique_ptr<ExprAST> ParseEq()
{
    assertion(kTokenIdent); 
    auto lhs = std::make_unique<VariableExprAST>(g_str_val);
    getNextToken();
    assertion(kTokenEq); 
    getNextToken();
    auto rhs = ParseBinOp(); // Правая часть - бинарное выражение
    return std::make_unique<BinaryExprAST>('=', std::move(lhs), std::move(rhs));
}

// Парсинг одного "главного" выражения или оператора внутри тела функции
// expr ::= assignment | for_loop | if_stmt
std::unique_ptr<ExprAST> ParseExpr()
{
    if (g_current_token == kTokenIdent)
    {
        // Если идентификатор, то это начало присваивания
        return ParseEq();
    }
    else if (g_current_token == kTokenFor)
    {
        // Парсинг цикла for: 'for' '(' assign ',' binop ',' assign ')' '{' assign* '}'
        getNextToken();
        assertion(kTokenLParen);
        getNextToken();
        auto start = ParseEq(); 
        assertion(kTokenComma);
        getNextToken();
        auto cond = ParseBinOp(); 
        assertion(kTokenComma);
        getNextToken();
        auto step = ParseEq(); 
        assertion(kTokenRParen);
        getNextToken();
        assertion(kTokenLFig);
        getNextToken();
        std::vector<std::unique_ptr<ExprAST>> body;
        // Парсим тело цикла 
        while (g_current_token != kTokenRFig)
        {
            body.push_back(ParseEq());
        }
        assertion(kTokenRFig); 
        getNextToken();
        return std::make_unique<ForExprAST>(std::move(start), std::move(cond), std::move(step), std::move(body));
    }
    else if (g_current_token == kTokenIf)
    {
        // Парсинг if: 'if' '(' binop ')' '{' assign* '}' 'else' '{' assign* '}'
        getNextToken();
        assertion(kTokenLParen);
        getNextToken();
        auto cond = ParseBinOp(); 
        assertion(kTokenRParen);
        getNextToken();
        assertion(kTokenLFig);
        getNextToken();
        std::vector<std::unique_ptr<ExprAST>> then_body;
        // Парсим тело 'then' 
        while (g_current_token != kTokenRFig)
        {
            then_body.push_back(ParseEq());
        }
        assertion(kTokenRFig); 
        getNextToken();
        assertion(kTokenElse); 
        getNextToken();
        assertion(kTokenLFig);
        getNextToken();
        std::vector<std::unique_ptr<ExprAST>> else_body;
        // Парсим тело 'else' 
        while (g_current_token != kTokenRFig)
        {
            else_body.push_back(ParseEq());
        }
        assertion(kTokenRFig); // Проверяем '}'
        getNextToken();
        return std::make_unique<IfExprAST>(std::move(cond), std::move(then_body), std::move(else_body));
    }
    // Если токен не соответствует ни одному из ожидаемых, возвращаем nullptr или бросаем ошибку
    throw std::runtime_error("Ошибка синтаксического анализа: ожидалось присваивание, 'for' или 'if'");
    return nullptr;
}

// Парсинг бинарного выражения 
// binop ::= primary (('+' | '-') primary)*
// primary ::= ident | number
std::unique_ptr<ExprAST> ParsePrimary()
{
    if (g_current_token == kTokenIdent)
    {
        std::string var_name = g_str_val;
        getNextToken(); // Съедаем идентификатор
        return std::make_unique<VariableExprAST>(var_name);
    }
    else if (g_current_token == kTokenNumber)
    {
        int number = g_num_val;
        getNextToken(); // Съедаем число
        return std::make_unique<NumberExprAST>(number);
    }
    else
    {
        throw std::runtime_error("Ошибка синтаксического анализа: ожидалась переменная или число");
        return nullptr;
    }
}

std::unique_ptr<ExprAST> ParseBinOp()
{
    auto lhs = ParsePrimary(); // Начинаем с парсинга первичного выражения 
    if (!lhs)
        return nullptr;

    // Пока текущий токен - это '+' или '-', продолжаем парсить правую часть
    while (g_current_token == kTokenPlus || g_current_token == kTokenMinus)
    {
        char op = (g_current_token == kTokenPlus) ? '+' : '-';
        getNextToken();            // Съедаем оператор
        auto rhs = ParsePrimary(); // Парсим правую часть (следующее первичное выражение)
        if (!rhs)
            return nullptr;
        // Создаем узел бинарной операции, делая предыдущий результат левым операндом (левая ассоциативность)
        lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
    }
    // Возвращаем построенное дерево выражения
    return lhs;
}


int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        try
        {
            init_scanner(argv[1]);
            initialize_module();

            std::unique_ptr<FunctionAST> ast_root(Parse());
            if (!ast_root)
            {
                std::cerr << "Ошибка: не удалось построить AST." << std::endl;
                return 1;
            }

            Function *generated_function = ast_root->codegen();
            if (!generated_function)
            {
                std::cerr << "Ошибка: не удалось сгенерировать LLVM IR." << std::endl;
                return 1;
            }

            // Используем errs() вместо std::cout, чтобы вывод IR не смешивался с потенциальным выводом программы
            g_module->print(errs(), nullptr);
        }
        catch (const std::runtime_error &e)
        {
            // Ловим ошибки парсинга или другие ошибки времени выполнения
            std::cerr << "Ошибка компиляции: " << e.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            // Ловим любые другие исключения
            std::cerr << "Неизвестная ошибка компиляции." << std::endl;
            return 1;
        }
    }
    else
    {н
        std::cerr << "Использование: " << argv[0] << " <input file>" << std::endl;
        return 1; 
    }
    return 0; 
}
