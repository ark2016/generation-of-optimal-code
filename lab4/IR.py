from BB import *


def example():
    """
    Создает пример программы с несколькими блоками и ветвлениями:
    
    a = 10
    b = 5
    if a > b then
        a = a - b
    else
        b = b - a
    c = a + b
    if c > 0 then
        c = c * 2
    else
        c = 0
    return c
    """
    # Создаем начальный блок (блок 0)
    b0 = BB()
    b0.block_num = 0
    tmp = b0.create_tmp_var()
    b0.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # Инициализация переменных
    b0.add_instr(Instruction("store", {'from': 10, 'to': Variable('a', 0)}))
    b0.add_instr(Instruction("store", {'from': 5, 'to': Variable('b', 0)}))
    # Сравнение a > b
    b0.add_instr(Instruction('icmp', {'arg1': Variable('a', 0), 
                                     'arg2': Variable('b', 0), 
                                     'to': tmp}))
    # Условный переход к блокам 1 или 2
    b0.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 1, 'dest2': 2}))
    b0.returned = False

    # Блок 1 (когда a > b)
    b1 = BB()
    b1.block_num = 1
    tmp = b1.create_tmp_var()
    b1.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # a = a - b
    b1.add_instr(Instruction("sub", {'oper1': Variable('a', 0), 
                                    'oper2': Variable('b', 0), 
                                    'to': tmp}))
    b1.add_instr(Instruction("store", {'from': tmp, 'to': Variable('a', 0)}))
    # Переход к блоку 3
    b1.add_instr(Instruction('br', {'dest': 3}))
    b1.returned = False

    # Блок 2 (когда a <= b)
    b2 = BB()
    b2.block_num = 2
    tmp = b2.create_tmp_var()
    b2.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # b = b - a
    b2.add_instr(Instruction("sub", {'oper1': Variable('b', 0), 
                                    'oper2': Variable('a', 0), 
                                    'to': tmp}))
    b2.add_instr(Instruction("store", {'from': tmp, 'to': Variable('b', 0)}))
    # Переход к блоку 3
    b2.add_instr(Instruction('br', {'dest': 3}))
    b2.returned = False

    # Блок 3 (слияние потоков)
    b3 = BB()
    b3.block_num = 3
    tmp = b3.create_tmp_var()
    b3.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # c = a + b
    b3.add_instr(Instruction("add", {'oper1': Variable('a', 0), 
                                    'oper2': Variable('b', 0), 
                                    'to': tmp}))
    b3.add_instr(Instruction("store", {'from': tmp, 'to': Variable('c', 0)}))
    # Сравнение c > 0
    tmp = b3.create_tmp_var()
    b3.add_instr(Instruction('icmp', {'arg1': Variable('c', 0), 
                                     'arg2': 0, 
                                     'to': tmp}))
    # Условный переход к блокам 4 или 5
    b3.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 4, 'dest2': 5}))
    b3.returned = False

    # Блок 4 (когда c > 0)
    b4 = BB()
    b4.block_num = 4
    tmp = b4.create_tmp_var()
    b4.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # c = c * 2
    b4.add_instr(Instruction("mul", {'oper1': Variable('c', 0), 
                                    'oper2': 2, 
                                    'to': tmp}))
    b4.add_instr(Instruction("store", {'from': tmp, 'to': Variable('c', 0)}))
    # Переход к блоку 6
    b4.add_instr(Instruction('br', {'dest': 6}))
    b4.returned = False

    # Блок 5 (когда c <= 0)
    b5 = BB()
    b5.block_num = 5
    b5.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # c = 0
    b5.add_instr(Instruction("store", {'from': 0, 'to': Variable('c', 0)}))
    # Переход к блоку 6
    b5.add_instr(Instruction('br', {'dest': 6}))
    b5.returned = False

    # Блок 6 (заключительный)
    b6 = BB()
    b6.block_num = 6
    b6.variables = {'a': Variable('a', 0), 'b': Variable('b', 0), 'c': Variable('c', 0)}
    # return c
    b6.add_instr(Instruction("ret", {'value': Variable('c', 0)}))
    b6.returned = True

    # Возвращаем список всех блоков
    return [b0, b1, b2, b3, b4, b5, b6]


def example1():
    """
    Создает пример программы с циклом:
    
    i = 0
    sum = 0
    while i < 5 do
        sum = sum + i
        i = i + 1
    return sum
    """
    # Блок 0: инициализация
    b0 = BB()
    b0.block_num = 0
    b0.variables = {'i': Variable('i', 0), 'sum': Variable('sum', 0)}
    # i = 0
    b0.add_instr(Instruction("store", {'from': 0, 'to': Variable('i', 0)}))
    # sum = 0
    b0.add_instr(Instruction("store", {'from': 0, 'to': Variable('sum', 0)}))
    # Переход к проверке условия
    b0.add_instr(Instruction('br', {'dest': 1}))
    b0.returned = False
    
    # Блок 1: проверка условия цикла
    b1 = BB()
    b1.block_num = 1
    tmp = b1.create_tmp_var()
    b1.variables = {'i': Variable('i', 0), 'sum': Variable('sum', 0)}
    # i < 5
    b1.add_instr(Instruction('icmp', {'arg1': Variable('i', 0), 
                                     'arg2': 5, 
                                     'to': tmp}))
    # Переход к телу цикла или выходу
    b1.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 2, 'dest2': 3}))
    b1.returned = False
    
    # Блок 2: тело цикла
    b2 = BB()
    b2.block_num = 2
    b2.variables = {'i': Variable('i', 0), 'sum': Variable('sum', 0)}
    
    # sum = sum + i
    tmp1 = b2.create_tmp_var()
    b2.add_instr(Instruction("add", {'oper1': Variable('sum', 0), 
                                    'oper2': Variable('i', 0), 
                                    'to': tmp1}))
    b2.add_instr(Instruction("store", {'from': tmp1, 'to': Variable('sum', 0)}))
    
    # i = i + 1
    tmp2 = b2.create_tmp_var()
    b2.add_instr(Instruction("add", {'oper1': Variable('i', 0), 
                                    'oper2': 1, 
                                    'to': tmp2}))
    b2.add_instr(Instruction("store", {'from': tmp2, 'to': Variable('i', 0)}))
    
    # Возврат к проверке условия
    b2.add_instr(Instruction('br', {'dest': 1}))
    b2.returned = False
    
    # Блок 3: выход из цикла
    b3 = BB()
    b3.block_num = 3
    b3.variables = {'i': Variable('i', 0), 'sum': Variable('sum', 0)}
    # return sum
    b3.add_instr(Instruction("ret", {'value': Variable('sum', 0)}))
    b3.returned = True
    
    # Возвращаем список всех блоков
    return [b0, b1, b2, b3]


def example2():
    """
    Создает пример программы с вложенными условиями:
    
    x = 10
    y = 20
    z = 5
    
    if x > y then
        if x > z then
            max = x
        else
            max = z
    else
        if y > z then
            max = y
        else
            max = z
    
    return max
    """
    # Блок 0: инициализация
    b0 = BB()
    b0.block_num = 0
    b0.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # x = 10
    b0.add_instr(Instruction("store", {'from': 10, 'to': Variable('x', 0)}))
    # y = 20
    b0.add_instr(Instruction("store", {'from': 20, 'to': Variable('y', 0)}))
    # z = 5
    b0.add_instr(Instruction("store", {'from': 5, 'to': Variable('z', 0)}))
    
    # Проверка x > y
    tmp = b0.create_tmp_var()
    b0.add_instr(Instruction('icmp', {'arg1': Variable('x', 0), 
                                     'arg2': Variable('y', 0), 
                                     'to': tmp}))
    # Переход к ветвям
    b0.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 1, 'dest2': 3}))
    b0.returned = False
    
    # Блок 1: x > y, проверка x > z
    b1 = BB()
    b1.block_num = 1
    tmp = b1.create_tmp_var()
    b1.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # x > z
    b1.add_instr(Instruction('icmp', {'arg1': Variable('x', 0), 
                                     'arg2': Variable('z', 0), 
                                     'to': tmp}))
    # Переход к ветвям
    b1.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 2, 'dest2': 5}))
    b1.returned = False
    
    # Блок 2: x > y и x > z, max = x
    b2 = BB()
    b2.block_num = 2
    b2.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # max = x
    b2.add_instr(Instruction("store", {'from': Variable('x', 0), 'to': Variable('max', 0)}))
    # Переход к финальному блоку
    b2.add_instr(Instruction('br', {'dest': 6}))
    b2.returned = False
    
    # Блок 3: x <= y, проверка y > z
    b3 = BB()
    b3.block_num = 3
    tmp = b3.create_tmp_var()
    b3.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # y > z
    b3.add_instr(Instruction('icmp', {'arg1': Variable('y', 0), 
                                     'arg2': Variable('z', 0), 
                                     'to': tmp}))
    # Переход к ветвям
    b3.add_instr(Instruction('condbr', {'cond': tmp, 'dest1': 4, 'dest2': 5}))
    b3.returned = False
    
    # Блок 4: x <= y и y > z, max = y
    b4 = BB()
    b4.block_num = 4
    b4.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # max = y
    b4.add_instr(Instruction("store", {'from': Variable('y', 0), 'to': Variable('max', 0)}))
    # Переход к финальному блоку
    b4.add_instr(Instruction('br', {'dest': 6}))
    b4.returned = False
    
    # Блок 5: z максимальный, max = z
    b5 = BB()
    b5.block_num = 5
    b5.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # max = z
    b5.add_instr(Instruction("store", {'from': Variable('z', 0), 'to': Variable('max', 0)}))
    # Переход к финальному блоку
    b5.add_instr(Instruction('br', {'dest': 6}))
    b5.returned = False
    
    # Блок 6: завершение
    b6 = BB()
    b6.block_num = 6
    b6.variables = {'x': Variable('x', 0), 'y': Variable('y', 0), 
                   'z': Variable('z', 0), 'max': Variable('max', 0)}
    # return max
    b6.add_instr(Instruction("ret", {'value': Variable('max', 0)}))
    b6.returned = True
    
    # Возвращаем список всех блоков
    return [b0, b1, b2, b3, b4, b5, b6]