from dataclasses import dataclass


# Определение констант для типов инструкций
ALLOCA = 'alloca'
LOAD = 'load'
STORE = 'store'
BR = 'br'
CONDBR = 'condbr'
ICMP = 'icmp'
MUL = 'mul'
ADD = 'add'
SUB = 'sub'
RET = 'ret'
PHI = 'phi'


class Value:
    """Базовый класс для всех значений в IR"""
    pass


@dataclass
class Variable(Value):
    """Представление переменной в промежуточном коде"""
    
    def __init__(self, name, version):
        self.name = name
        self.version = version
        self.is_temp = False

    def __repr__(self):
        return str(self)

    def __str__(self):
        if self.is_temp:
            return f'{self.name}'
        return f'{self.name}({"" + str(self.version) + ""})'

    def __hash__(self):
        return len(self.name)

    def __eq__(self, other):
        if type(other) == type(self):
            # Сравниваем переменные по имени и версии
            return self.name == other.name and self.version == other.version
        elif type(other) == str:
            # Если сравниваем со строкой, сравниваем только имя
            return self.name == other
        return False


@dataclass
class IntConst(Value):
    """Представление целочисленной константы в промежуточном коде"""
    
    value: int

    def __repr__(self):
        return str(self)

    def __str__(self):
        return str(self.value)
    

@dataclass
class Instruction:
    """Представление инструкции в промежуточном коде"""
    
    # Тип инструкции
    typ: str
    # Аргументы инструкции
    args: dict

    def __repr__(self):
        return str(self)

    def __str__(self):
        """Строковое представление инструкции в зависимости от типа"""
        if self.typ == STORE:
            return f'{self.args["to"]} <- {self.args["from"]}'
            
        if self.typ == LOAD:
            return f'{self.args["to"]} <- {self.args["from"]}'
            
        if self.typ in [SUB, ADD, MUL]:
            return f'{self.args["to"]} <- {self.args["oper1"]} {self.typ} {self.args["oper2"]}'
            
        if self.typ == BR:
            return f'go to BLOCK{self.args["dest"]}'
            
        if self.typ == CONDBR:
            return f'if ({self.args["cond"]}) go to BLOCK{self.args["dest1"]} else go to BLOCK{self.args["dest2"]}'
            
        if self.typ == ICMP:
            return f'{self.args["to"]} <- {self.args["arg1"]} > {self.args["arg2"]}'
            
        if self.typ == PHI:
            return f'{self.args["to"]} = phi({", ".join(map(str, self.args["from"]))})'
            
        if self.typ == ALLOCA:
            return f'new variable {self.args["name"]}'
        
        # Общий случай для других типов инструкций
        ret = f'    {self.typ}: '
        for k, v in self.args.items():
            ret += f'{k} {v} '
        return ret
    

@dataclass
class BB:
    """
    Базовый блок - последовательность инструкций, 
    выполняемых без разрывов потока управления
    """
    
    def __init__(self):
        # Номер блока
        self.block_num = 0
        # Список инструкций
        self.instructions = []
        # Флаг, показывающий, содержит ли блок инструкцию возврата
        self.returned = False
        # Переменные в области видимости блока
        self.variables = {}
        # Счетчик для временных переменных
        self.varcounter = 0
        # Словарь для хранения phi-функций
        self.phi_var_blocks = {}
    
    def __hash__(self):
        return self.block_num

    def __str__(self):
        header = f'BLOCK {self.block_num}'
        body = '\n'.join(map(str, self.instructions))
        return f'{header}' + '{\n' + body + "\n}\n"
    
    def __repr__(self):
        return f'BB({self.block_num})'

    # ====== ФУНКЦИОНАЛ IR БЛОКА ======

    def add_instr(self, instr):
        """Добавляет инструкцию в блок, если блок не содержит return"""
        if self.returned:
            return
        self.instructions.append(instr)
    
    def alloca_variable(self, name):
        """Выделяет память для новой переменной"""
        newvar = Variable(name, 0)
        self.variables[name] = newvar
        instr = Instruction(ALLOCA, {'name': name})
        self.add_instr(instr)
        return newvar

    def create_tmp_var(self):
        """Создает временную переменную"""
        self.varcounter += 1
        var = Variable(f'tmp_{self.block_num}_{self.varcounter-1}', 0)
        var.is_temp = True
        return var

    def new_break(self, dest):
        """Создает инструкцию безусловного перехода"""
        self.add_instr(
            Instruction(BR, {'dest': dest.block_num})
        )

    def new_ret(self, val: Value):
        """Создает инструкцию возврата"""
        if isinstance(val, IntConst):
            self.add_instr(Instruction(RET, {'value': val}))
        elif isinstance(val, Variable):
            tmpvar = self.create_tmp_var()
            self.add_instr(Instruction(LOAD, {"from": val, "to": tmpvar}))
            self.add_instr(Instruction(RET, {'value': tmpvar}))

    def new_cond_break(self, cond: Variable, dest1, dest2):
        """Создает инструкцию условного перехода"""
        self.add_instr(
            Instruction(CONDBR, {'cond': cond, 
                                 'dest1': dest1.block_num, 
                                 'dest2': dest2.block_num})
        )

    def new_compare(self, arg1, arg2):
        """Создает инструкцию сравнения"""
        tmp = self.create_tmp_var()
        self.add_instr(Instruction(ICMP, {"arg1": arg1, "arg2": arg2, "to": tmp}))
        return tmp

    # ====== ФУНКЦИОНАЛ СТРОИТЕЛЯ ======

    def is_variable_in(self, name: str):
        """Проверяет, объявлена ли переменная в блоке"""
        return name in self.variables
    
    def set_variable(self, name: str, val: Value):
        """Устанавливает значение переменной"""
        if isinstance(val, Variable):
            tmpvar = self.create_tmp_var()
            self.add_instr(Instruction(LOAD, {"from": val, "to": tmpvar}))
            self.add_instr(Instruction(STORE, {"from": tmpvar, "to": self.variables[name]}))
        elif isinstance(val, IntConst):
            self.add_instr(Instruction(STORE, {"from": val, "to": self.variables[name]}))

    def set_map(self, parent):
        """Наследует переменные из родительского блока"""
        self.variables.update(parent.variables)

    # ====== ФУНКЦИОНАЛ ДЛЯ ЛАБОРАТОРНОЙ РАБОТЫ №4 ======

    def get_edges(self):
        """Возвращает множество исходящих рёбер из блока"""
        if not self.instructions:
            return set()
            
        last = self.instructions[-1]
        if last.typ == BR:
            return {(self.block_num, last.args["dest"])}
        elif last.typ == CONDBR:
            return {(self.block_num, last.args["dest1"]), 
                   (self.block_num, last.args["dest2"])}
        return set()

    def build_changing_variables(self):
        """Определяет множество переменных, изменяемых в блоке"""
        changed_vars = set()
        for instruction in self.instructions:
            if instruction.typ == STORE:
                changed_vars.add(instruction.args['to'])

        self.changing_variables = changed_vars
