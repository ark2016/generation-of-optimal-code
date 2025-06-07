"""
Модуль для преобразования простого текстового кода в IR.

Поддерживаемый синтаксис:
    - Объявление переменных: x = 10
    - Арифметические операции: x = a + b, x = a - b, x = a * b
    - Условные операторы: if x > y then ... else ...
    - Циклы: while x < y do ...
    - Возврат значения: return x
"""

from BB import *


class Parser:
    """
    Парсер простого языка программирования для создания IR и CFG.
    """
    
    def __init__(self):
        """Инициализирует парсер."""
        self.blocks = []  # Список базовых блоков
        self.next_block_num = 0  # Счетчик для нумерации блоков
        self.variables = {}  # Словарь переменных
        self.current_block = None  # Текущий обрабатываемый блок
    
    def create_block(self):
        """Создает новый базовый блок."""
        block = BB()
        block.block_num = self.next_block_num
        self.next_block_num += 1
        block.variables = self.variables.copy()
        self.blocks.append(block)
        return block
    
    def parse(self, code):
        """
        Преобразует исходный код в IR.
        
        Args:
            code: Строка с исходным кодом программы
            
        Returns:
            list: Список базовых блоков IR
        """
        # Очищаем состояние парсера
        self.blocks = []
        self.next_block_num = 0
        self.variables = {}
        
        # Создаем первый блок
        self.current_block = self.create_block()
        
        # Разбиваем код на строки и удаляем пустые строки
        lines = [line.strip() for line in code.strip().split('\n') if line.strip()]
        
        # Обрабатываем каждую строку
        self._parse_lines(lines)
        
        return self.blocks
    
    def _parse_lines(self, lines, end_markers=None):
        """
        Рекурсивно обрабатывает список строк кода.
        
        Args:
            lines: Список строк кода
            end_markers: Маркеры для завершения обработки (например, 'else', 'end')
            
        Returns:
            int: Количество обработанных строк
        """
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            # Проверяем маркеры завершения
            if end_markers and any(line.startswith(marker) for marker in end_markers):
                return i
            
            # Обрабатываем различные конструкции языка
            if line.startswith('if'):
                i += self._parse_if(lines[i:])
            elif line.startswith('while'):
                i += self._parse_while(lines[i:])
            elif line.startswith('return'):
                self._parse_return(line)
                i += 1
            elif '=' in line:
                self._parse_assignment(line)
                i += 1
            else:
                # Неизвестная конструкция
                raise ValueError(f"Неизвестная конструкция: {line}")
        
        return i
    
    def _parse_assignment(self, line):
        """
        Обрабатывает оператор присваивания.
        
        Args:
            line: Строка с оператором присваивания
        """
        # Разбиваем строку по оператору присваивания
        left, right = [part.strip() for part in line.split('=', 1)]
        
        # Если переменная не объявлена, добавляем ее
        if left not in self.variables:
            self.variables[left] = Variable(left, 0)
            self.current_block.variables[left] = self.variables[left]
        
        # Разбираем правую часть
        if '+' in right:
            self._parse_binary_op(left, right, ADD)
        elif '-' in right:
            self._parse_binary_op(left, right, SUB)
        elif '*' in right:
            self._parse_binary_op(left, right, MUL)
        else:
            # Простое присваивание
            try:
                # Если правая часть - число
                value = int(right)
                self.current_block.add_instr(
                    Instruction(STORE, {'from': value, 'to': self.variables[left]})
                )
            except ValueError:
                # Если правая часть - переменная
                if right not in self.variables:
                    self.variables[right] = Variable(right, 0)
                    self.current_block.variables[right] = self.variables[right]
                
                self.current_block.add_instr(
                    Instruction(STORE, {'from': self.variables[right], 'to': self.variables[left]})
                )
    
    def _parse_binary_op(self, left, right, op_type):
        """
        Обрабатывает бинарную операцию.
        
        Args:
            left: Левая часть присваивания
            right: Правая часть присваивания
            op_type: Тип операции (ADD, SUB, MUL)
        """
        # Разбиваем правую часть по оператору
        if op_type == ADD:
            parts = [part.strip() for part in right.split('+')]
        elif op_type == SUB:
            parts = [part.strip() for part in right.split('-')]
        else:  # MUL
            parts = [part.strip() for part in right.split('*')]
        
        # Получаем операнды
        operands = []
        for part in parts:
            try:
                # Если операнд - число
                operands.append(int(part))
            except ValueError:
                # Если операнд - переменная
                if part not in self.variables:
                    self.variables[part] = Variable(part, 0)
                    self.current_block.variables[part] = self.variables[part]
                operands.append(self.variables[part])
        
        # Создаем временную переменную для результата
        tmp = self.current_block.create_tmp_var()
        
        # Добавляем инструкцию операции
        self.current_block.add_instr(
            Instruction(op_type, {'oper1': operands[0], 'oper2': operands[1], 'to': tmp})
        )
        
        # Сохраняем результат в переменную
        self.current_block.add_instr(
            Instruction(STORE, {'from': tmp, 'to': self.variables[left]})
        )
    
    def _parse_if(self, lines):
        """
        Обрабатывает условный оператор.
        
        Args:
            lines: Список строк кода, начиная с 'if'
            
        Returns:
            int: Количество обработанных строк
        """
        # Выделяем условие
        if_line = lines[0].strip()
        condition_parts = if_line.split('then')[0][2:].strip().split()
        
        # Определяем переменные в условии
        left_var = condition_parts[0].strip()
        right_var = condition_parts[2].strip()
        
        # Если переменные не объявлены, добавляем их
        if left_var not in self.variables:
            self.variables[left_var] = Variable(left_var, 0)
            self.current_block.variables[left_var] = self.variables[left_var]
        
        try:
            # Если правая часть - число
            right_val = int(right_var)
        except ValueError:
            # Если правая часть - переменная
            if right_var not in self.variables:
                self.variables[right_var] = Variable(right_var, 0)
                self.current_block.variables[right_var] = self.variables[right_var]
            right_val = self.variables[right_var]
        
        # Создаем временную переменную для результата сравнения
        tmp = self.current_block.create_tmp_var()
        
        # Добавляем инструкцию сравнения
        self.current_block.add_instr(
            Instruction(ICMP, {'arg1': self.variables[left_var], 'arg2': right_val, 'to': tmp})
        )
        
        # Создаем блоки для true и false ветвей
        true_block = self.create_block()
        false_block = self.create_block()
        merge_block = self.create_block()
        
        # Добавляем условный переход
        self.current_block.add_instr(
            Instruction(CONDBR, {'cond': tmp, 'dest1': true_block.block_num, 'dest2': false_block.block_num})
        )
        
        # Обрабатываем true ветвь
        self.current_block = true_block
        lines_then = lines[1:]
        then_count = self._parse_lines(lines_then, ['else'])
        
        # Добавляем переход к блоку слияния
        if not self.current_block.returned:
            self.current_block.add_instr(
                Instruction(BR, {'dest': merge_block.block_num})
            )
        
        # Обрабатываем false ветвь
        self.current_block = false_block
        else_line = lines_then[then_count].strip()
        if else_line.startswith('else'):
            lines_else = lines_then[then_count+1:]
            else_count = self._parse_lines(lines_else, ['end'])
        else:
            else_count = 0
        
        # Добавляем переход к блоку слияния
        if not self.current_block.returned:
            self.current_block.add_instr(
                Instruction(BR, {'dest': merge_block.block_num})
            )
        
        # Переходим к блоку слияния
        self.current_block = merge_block
        
        # Вычисляем количество обработанных строк
        return 1 + then_count + 1 + else_count + 1
    
    def _parse_while(self, lines):
        """
        Обрабатывает цикл while.
        
        Args:
            lines: Список строк кода, начиная с 'while'
            
        Returns:
            int: Количество обработанных строк
        """
        # Выделяем условие
        while_line = lines[0].strip()
        condition_parts = while_line.split('do')[0][5:].strip().split()
        
        # Определяем переменные в условии
        left_var = condition_parts[0].strip()
        right_var = condition_parts[2].strip()
        
        # Если переменные не объявлены, добавляем их
        if left_var not in self.variables:
            self.variables[left_var] = Variable(left_var, 0)
            self.current_block.variables[left_var] = self.variables[left_var]
        
        try:
            # Если правая часть - число
            right_val = int(right_var)
        except ValueError:
            # Если правая часть - переменная
            if right_var not in self.variables:
                self.variables[right_var] = Variable(right_var, 0)
                self.current_block.variables[right_var] = self.variables[right_var]
            right_val = self.variables[right_var]
        
        # Создаем блок для условия цикла
        cond_block = self.create_block()
        
        # Добавляем переход к блоку условия
        self.current_block.add_instr(
            Instruction(BR, {'dest': cond_block.block_num})
        )
        
        # Переходим к блоку условия
        self.current_block = cond_block
        
        # Создаем временную переменную для результата сравнения
        tmp = self.current_block.create_tmp_var()
        
        # Добавляем инструкцию сравнения
        self.current_block.add_instr(
            Instruction(ICMP, {'arg1': self.variables[left_var], 'arg2': right_val, 'to': tmp})
        )
        
        # Создаем блоки для тела цикла и выхода
        body_block = self.create_block()
        exit_block = self.create_block()
        
        # Добавляем условный переход
        self.current_block.add_instr(
            Instruction(CONDBR, {'cond': tmp, 'dest1': body_block.block_num, 'dest2': exit_block.block_num})
        )
        
        # Обрабатываем тело цикла
        self.current_block = body_block
        lines_body = lines[1:]
        body_count = self._parse_lines(lines_body, ['end'])
        
        # Добавляем переход обратно к условию
        self.current_block.add_instr(
            Instruction(BR, {'dest': cond_block.block_num})
        )
        
        # Переходим к блоку выхода из цикла
        self.current_block = exit_block
        
        # Вычисляем количество обработанных строк
        return 1 + body_count + 1
    
    def _parse_return(self, line):
        """
        Обрабатывает оператор return.
        
        Args:
            line: Строка с оператором return
        """
        # Выделяем возвращаемое значение
        value_str = line.split('return')[1].strip()
        
        try:
            # Если значение - число
            value = int(value_str)
            self.current_block.add_instr(
                Instruction(RET, {'value': value})
            )
        except ValueError:
            # Если значение - переменная
            if value_str not in self.variables:
                self.variables[value_str] = Variable(value_str, 0)
                self.current_block.variables[value_str] = self.variables[value_str]
            
            self.current_block.add_instr(
                Instruction(RET, {'value': self.variables[value_str]})
            )
        
        self.current_block.returned = True 