#include <iostream>
#include <sstream>
#include <string>

#include <gcc-plugin.h>
#include <plugin-version.h>

#include <coretypes.h>
#include <tree.h>
#include <tree-pass.h>
#include <context.h>
#include <basic-block.h>
#include <gimple.h>
#include <gimple-iterator.h>

// Макрос для подавления предупреждений об неиспользуемых переменных.
#define PREFIX_UNUSED(variable) ((void)variable)

// Обязательно для плагинов – объявляем совместимость с GPL.
int plugin_is_GPL_compatible = 1;

// Имя плагина
#define PLUGIN_NAME "lab1"

// Инициализация структуры данных прохода.
// Порядок полей: type, name, optinfo_flags, tv_id, properties_required, properties_provided,
// properties_destroyed, todo_flags_start, todo_flags_finish.
static const struct pass_data lab1_pass_data = {
    GIMPLE_PASS,
    PLUGIN_NAME,
    OPTGROUP_NONE,
    TV_NONE,
    0,
    0,
    0,
    0,
    0
};

// Функция для вывода информации о базовом блоке.
// Выводит: (список индексов предшественников) -> (индекс блока) -> (список индексов последователей)
static unsigned int lab1_bb_id(basic_block bb)
{
    std::cout << "\t" << "bb: ";
    edge e;
    edge_iterator it;

    std::stringstream src_stream;
    src_stream << "(";
    FOR_EACH_EDGE(e, it, bb->preds) {
        src_stream << e->src->index << ", ";
    }
    std::string src = src_stream.str();
    if (src.size() > 1)
        src = src.substr(0, src.size() - 2);
    src += ")";
    std::cout << src << " -> (" << bb->index << ") -> ";

    std::stringstream dst_stream;
    dst_stream << "(";
    FOR_EACH_EDGE(e, it, bb->succs) {
        dst_stream << e->dest->index << ", ";
    }
    std::string dst = dst_stream.str();
    if (dst.size() > 1)
        dst = dst.substr(0, dst.size() - 2);
    dst += ")";
    std::cout << dst;
    return 0;
}

// Функция для вывода информации о дереве (tree).
// Обрабатываются основные типы: константы, объявления, обращения к массивам, обращение к памяти, SSA-имена.
static unsigned int lab1_tree(tree t)
{
    switch (TREE_CODE(t)) {
        case INTEGER_CST:
            std::cout << TREE_INT_CST_LOW(t);
            break;
        case STRING_CST:
            std::cout << "\"" << TREE_STRING_POINTER(t) << "\"";
            break;
        case LABEL_DECL:
            std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "label_decl") << ":";
            break;
        case VAR_DECL:
            std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "var_decl");
            break;
        case CONST_DECL:
            std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "const_decl");
            break;
        // arr[5]
        case ARRAY_REF:
            lab1_tree(TREE_OPERAND(t, 0));
            std::cout << "[";
            lab1_tree(TREE_OPERAND(t, 1));
            std::cout << "]";
            break;
        case MEM_REF:
            std::cout << "(*";
            lab1_tree(TREE_OPERAND(t, 0));
            std::cout << ")";
            break;
                // Новый кейс: COMPONENT_REF – доступ к полю структуры
        case COMPONENT_REF:
        {
            // Первый операнд – выражение структуры
            lab1_tree(TREE_OPERAND(t, 0));
            std::cout << ".";
            // Второй операнд – имя поля (FIELD_DECL)
            tree field = TREE_OPERAND(t, 1);
            if (TREE_CODE(field) == FIELD_DECL && DECL_NAME(field))
                std::cout << IDENTIFIER_POINTER(DECL_NAME(field));
            else
                lab1_tree(field);
            break;
        }
        case SSA_NAME: {
            // Если SSA_NAME имеет определяющую инструкцию типа GIMPLE_PHI, выводим информацию о ней.
            gimple* st = SSA_NAME_DEF_STMT(t);
            if (gimple_code(st) == GIMPLE_PHI) {
                std::cout << "(" << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "ssa_name")
                          << "__v" << SSA_NAME_VERSION(t) << " = GIMPLE_PHI(";
                for (unsigned int i = 0; i < gimple_phi_num_args(st); i++) {
                    lab1_tree(gimple_phi_arg(st, i)->def);
                    if (i != gimple_phi_num_args(st) - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << "))";
            } else {
                std::cout << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "ssa_name")
                          << "__v" << SSA_NAME_VERSION(t);
            }
            break;
        }
        default:
            std::cout << "tree_code(" << TREE_CODE(t) << ")";
            break;
    }
    return 0;
}

// Функция для вывода оператора (арифметические, логические и отношения).
static unsigned int lab1_op(enum tree_code code)
{
    switch (code) {
    case PLUS_EXPR:      std::cout << "+"; break;
    case MINUS_EXPR:     std::cout << "-"; break;
    case MULT_EXPR:      std::cout << "*"; break;
    case RDIV_EXPR:      std::cout << "/"; break;
    case BIT_IOR_EXPR:   std::cout << "|"; break;
    case BIT_NOT_EXPR:   std::cout << "~"; break;
    case TRUTH_AND_EXPR: std::cout << "&&"; break;
    case TRUTH_OR_EXPR:  std::cout << "||"; break;
    case TRUTH_NOT_EXPR: std::cout << "!"; break;
    case LT_EXPR:        std::cout << "<"; break;
    case LE_EXPR:        std::cout << "<="; break;
    case GT_EXPR:        std::cout << ">"; break;
    case GE_EXPR:        std::cout << ">="; break;
    case EQ_EXPR:        std::cout << "=="; break;
    case NE_EXPR:        std::cout << "!="; break;
    default:
        std::cout << "op(" << code << ")";
        break;
    }
    return 0;
}

// Обработка GIMPLE_ASSIGN.
static unsigned int lab1_on_gimple_assign(gimple* stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_ASSIGN\" { ";
    switch (gimple_num_ops(stmt)) {
    case 2:
        lab1_tree(gimple_assign_lhs(stmt));
        std::cout << " = ";
        lab1_tree(gimple_assign_rhs1(stmt));
        break;
    case 3:
        lab1_tree(gimple_assign_lhs(stmt));
        std::cout << " = ";
        lab1_tree(gimple_assign_rhs1(stmt));
        std::cout << " ";
        lab1_op(gimple_assign_rhs_code(stmt));
        std::cout << " ";
        lab1_tree(gimple_assign_rhs2(stmt));
        break;
    }
    std::cout << " }" << std::endl;
    return 0;
}

// Обработка GIMPLE_CALL.
static unsigned int lab1_on_gimple_call(gimple* stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_CALL\" { ";
    tree lhs = gimple_call_lhs(stmt);
    if (lhs) {
        lab1_tree(lhs);
        std::cout << " = ";
    }
    std::cout << fndecl_name(gimple_call_fndecl(stmt)) << "(";
    for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++) {
        lab1_tree(gimple_call_arg(stmt, i));
        if (i != gimple_call_num_args(stmt) - 1)
            std::cout << ", ";
    }
    std::cout << ")";
    std::cout << " }" << std::endl;
    return 0;
}

// Обработка GIMPLE_COND.
static unsigned int lab1_on_gimple_cond(gimple* stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_COND\" { ";
    lab1_tree(gimple_cond_lhs(stmt));
    std::cout << " ";
    lab1_op(gimple_assign_rhs_code(stmt));
    std::cout << " ";
    lab1_tree(gimple_cond_rhs(stmt));
    std::cout << " }" << std::endl;
    return 0;
}

// Обработка GIMPLE_LABEL.
static unsigned int lab1_on_gimple_label(gimple* stmt)
{
    PREFIX_UNUSED(stmt);
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_LABEL\" { }" << std::endl;
    return 0;
}

// Обработка GIMPLE_RETURN.
static unsigned int lab1_on_gimple_return(gimple* stmt)
{
    PREFIX_UNUSED(stmt);
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_RETURN\" { }" << std::endl;
    return 0;
}

// Обход всех инструкций базового блока.
static unsigned int lab1_statements(basic_block bb)
{
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple* stmt = gsi_stmt(gsi);
        switch (gimple_code(stmt)) {
        case GIMPLE_ASSIGN:
            lab1_on_gimple_assign(stmt);
            break;
        case GIMPLE_CALL:
            lab1_on_gimple_call(stmt);
            break;
        case GIMPLE_COND:
            lab1_on_gimple_cond(stmt);
            break;
        case GIMPLE_LABEL:
            lab1_on_gimple_label(stmt);
            break;
        case GIMPLE_RETURN:
            lab1_on_gimple_return(stmt);
            break;
        default:
            break;
        }
    }
    return 0;
}

// Обход функции: вывод имени функции, базовых блоков и инструкций.
static unsigned int lab1_func(function* fn)
{
    std::cout << "func: \"" << function_name(fn) << "\" {" << std::endl;
    basic_block bb;
    FOR_EACH_BB_FN(bb, fn) {
        lab1_bb_id(bb);
        std::cout << " {" << std::endl;
        lab1_statements(bb);
        std::cout << "\t" << "}" << std::endl;
    }
    std::cout << "}" << std::endl << std::endl;
    return 0;
}

// Определяем класс прохода lab1_pass.
struct lab1_pass : public gimple_opt_pass {
    lab1_pass(gcc::context* ctx) : gimple_opt_pass(lab1_pass_data, ctx) { }
    unsigned int execute(function* fn) { return lab1_func(fn); }
    lab1_pass* clone() { return this; }
};

// Регистрируем проход.
// Для простоты используем глобальный контекст g, который должен быть определён в <context.h>.
static struct register_pass_info lab1_pass_info = {
    new lab1_pass(g),
    "ssa",                       // Привязываемся к ssa-пассу
    1,                           // Запуск один раз
    PASS_POS_INSERT_AFTER        // Позиционирование: после ssa-пасса
};

int plugin_init(struct plugin_name_args *args, struct plugin_gcc_version *version)
{
    if (!plugin_default_version_check(version, &gcc_version))
        return 1;
    register_callback(args->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &lab1_pass_info);
    return 0;
}
