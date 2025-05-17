import networkx as nx
from copy import deepcopy
from BB import *


class SsaBuilder:
    """
    Построитель SSA-формы для промежуточного представления.
    
    Класс реализует алгоритм построения SSA-формы (Static Single Assignment),
    в которой каждая переменная определяется ровно один раз.
    """
    
    def __init__(self, blocks, verbose=True):
        """
        Инициализирует построитель SSA и выполняет начальные вычисления.
        
        Args:
            blocks: Список базовых блоков промежуточного представления
            verbose: Флаг, управляющий выводом отладочной информации
        """
        self.blocks = blocks
        self.verbose = verbose
        
        # Построение доминаторов и границ доминирования
        self.build_dom()
        self.build_df()
        self.build_changed_variables()

    # ==== КОНСТРУКТОРЫ ====

    def build_dom(self):
        """
        Строит дерево доминаторов для графа потока управления.
        
        Домиратор - это узел, через который проходят все пути от стартового узла к данному.
        """
        # Создаем граф потока управления
        edges = set.union(*[x.get_edges() for x in self.blocks])
        CFG = nx.DiGraph(edges)
        for x in self.blocks_to_nums(self.blocks):
            CFG.add_node(x)
            
        # Находим все достижимые узлы из блока 0
        cc = nx.node_connected_component(CFG.to_undirected(), 0)
        self.CFG = nx.DiGraph(CFG.subgraph(cc))
        self.blocks = set(filter(lambda x: x.block_num in self.CFG, self.blocks))

        # Вычисляем непосредственные доминаторы и создаем словарь доминаторов
        imm_dom = nx.immediate_dominators(self.CFG, 0)
        self.dom_of = dict([(x, {imm_dom[x]}) for x in self.CFG])

        # Строим обратное отношение: дети для каждого узла
        self.children = dict([(x, set()) for x in self.CFG])
        for x, ys in self.dom_of.items():
            for y in ys:
                if y != x:
                    self.children[y].add(x)

    def build_df(self):
        """
        Строит границы доминирования для графа потока управления.
        
        Граница доминирования для узла X - это множество узлов Y таких, что 
        X доминирует над предшественником Y, но не доминирует над самим Y.
        """
        d = {}
        self.df = dict([(bb, set()) for bb in self.CFG])
        old = dict()
        
        # Итеративно вычисляем границы доминирования
        while old != self.df:
            old = deepcopy(self.df)
            for x in self.blocks_to_nums(self.blocks):
                # Для каждого преемника, который не доминируется x
                for y in self.get_succ(x):
                    if x not in self.dom_of[y]:
                        self.df[x].add(y)
                
                # Для каждого дочернего узла
                children = self.children[x]
                if str(x) + ' ->' not in d:
                    d[str(x) + ' ->'] = children
                
                # Распространяем границы доминирования от детей к родителям
                for z in children:
                    for y in self.df[z]:
                        if y not in self.children[x]:
                            self.df[x].add(y)

        # Проверяем совпадение с библиотечной реализацией
        assert self.df == nx.dominance_frontiers(self.CFG, 0)

    def build_changed_variables(self):
        """
        Определяет для каждого блока множество переменных, которые изменяются в нем.
        """
        for x in self.blocks:
            x.build_changing_variables()

    # ==== СЛУЖЕБНЫЕ МЕТОДЫ ====

    def print_blocks(self):
        """Печатает все блоки промежуточного представления"""
        for bb in self.blocks:
            print(bb)

    def to_graph(self):
        """
        Генерирует представление графа в формате DOT.
        
        Returns:
            str: Строка в формате DOT, представляющая граф
        """
        # Начинаем создание DOT-файла
        ret = "digraph G{\nnode [shape=box nojustify=false]\n"
        
        # Добавляем узлы графа (блоки)
        for x in self.blocks:
            # Форматируем содержимое блока
            s = str(x).replace('    ', '').replace('{', '').replace('}', '').replace("\n", "\\l    ").strip()
            while s[-2:] == '\\l':
                s = s[:-2].strip()
            ret += f'{x.block_num} [label=\"{s}\"]\n'
            
            # Добавляем ребра в зависимости от типа последней инструкции
            last = x.instructions[-1]
            if last.typ == BR:
                ret += f'{x.block_num} -> {last.args["dest"]}\n'
            elif last.typ == CONDBR:
                ret += f'{x.block_num} -> {last.args["dest1"]} [label=true]\n'
                ret += f'{x.block_num} -> {last.args["dest2"]} [label=false]\n'
                
        ret += "}\n"
        return ret

    def get_block(self, n):
        """Возвращает блок по его номеру"""
        return list(filter(lambda x: x.block_num == n, self.blocks))[0]

    def blocks_to_nums(self, s):
        """Преобразует набор блоков в набор их номеров"""
        return set(map(lambda bb: bb.block_num, s))

    def nums_to_bloks(self, nums):
        """Преобразует набор номеров блоков в набор блоков"""
        return set(filter(lambda bb: bb.block_num in nums, self.blocks))

    def get_all_vars_names(self):
        """Возвращает множество имен всех переменных в программе"""
        return set.union(*[set(bb.variables.keys()) for bb in self.blocks])

    def get_preds(self, node):
        """Возвращает множество предшественников узла в графе потока управления"""
        if isinstance(node, BB):
            node = node.block_num
        return set(self.CFG.predecessors(node))

    def get_succ(self, node):
        """Возвращает множество преемников узла в графе потока управления"""
        if isinstance(node, BB):
            node = node.block_num
        return set(self.CFG.successors(node))

    def find_blocks_that_redefine_var(self, varname):
        """
        Находит все блоки, в которых переопределяется переменная.
        
        Args:
            varname: Имя искомой переменной
            
        Returns:
            set: Множество блоков, в которых переменная переопределяется
        """
        redefining_blocks = set()
        for bb in self.blocks:
            for var in bb.changing_variables:
                if var.name == varname:
                    redefining_blocks.add(bb)
                    break
        return redefining_blocks

    # ==== РАЗМЕЩЕНИЕ PHI-ФУНКЦИЙ ====

    def find_df(self, s):
        """
        Находит границы доминирования для множества узлов.
        
        Args:
            s: Множество узлов
            
        Returns:
            set: Объединение границ доминирования для всех узлов в s
        """
        df_union = set()
        for x in s:
            df_union.update(self.df[x])
        return df_union

    def find_df_post_order(self, s):
        """
        Находит транзитивное замыкание по границам доминирования.
        
        Args:
            s: Начальное множество узлов
            
        Returns:
            set: Транзитивное замыкание границ доминирования
        """
        old = set()
        new = self.find_df(s)
        
        # Итеративно расширяем множество, пока оно не перестанет меняться
        while True:
            old |= new
            new = self.find_df(new)
            if new.difference(old) == set():
                return old

    def find_post_order(self, s):
        """Обертка для find_df_post_order"""
        return self.find_df_post_order(s)

    def insert_phi(self, varname):
        """
        Вставляет phi-функции для указанной переменной.
        
        Phi-функции вставляются в узлы, находящиеся на границах доминирования
        блоков, в которых переопределяется переменная.
        
        Args:
            varname: Имя переменной, для которой вставляются phi-функции
        """
        # Находим блоки, в которых переменная переопределяется
        stored_in_blocks = self.find_blocks_that_redefine_var(varname)
        stored_in_blocks_num = self.blocks_to_nums(stored_in_blocks)

        # Находим блоки на границах доминирования
        post_order_blocks_num = self.find_post_order(stored_in_blocks_num)
        post_order_blocks = self.nums_to_bloks(post_order_blocks_num)

        # Для каждого блока на границе доминирования добавляем phi-функцию
        for bb in post_order_blocks:
            bb.phi_var_blocks[varname] = set()
            # Добавляем всех предшественников блока как источники для phi-функции
            preds = self.CFG.predecessors(bb.block_num)
            for pred in preds:
                bb.phi_var_blocks[varname].add(pred)


    def insert_all_phi(self):
        """
        Вставляет phi-функции для всех переменных программы.
        
        Этот метод:
        1. Находит все переменные программы
        2. Для каждой переменной вставляет phi-функции
        3. Добавляет phi-инструкции в начало соответствующих блоков
        """
        # Получаем и сортируем имена всех переменных
        vars = self.get_all_vars_names()
        var_names = sorted(list(vars))
        
        # Вставляем phi-функции для каждой переменной
        for varname in var_names:
            self.insert_phi(varname)

        # Добавляем инструкции phi в начало блоков
        for bb in self.blocks:
            for varname, phiblocks in bb.phi_var_blocks.items():
                instr = Instruction(PHI, {'to': Variable(varname, 0), 
                                         'from': list(phiblocks)})
                bb.instructions.insert(0, instr)

    # ==== ОБНОВЛЕНИЕ ВЕРСИЙ ПЕРЕМЕННЫХ ====

    def update_variable_versions(self):
        """Запускает обход для обновления версий всех переменных"""
        self.traverse()

    def traverse(self):
        """
        Выполняет обход для обновления версий переменных.
        
        Для каждой переменной запускает рекурсивный обход, начиная с блока 0,
        и обновляет версии переменных.
        """
        # Получаем и сортируем имена всех переменных
        vars = self.get_all_vars_names()
        var_names = sorted(list(vars))

        # Для каждой переменной выполняем обход
        for target_var in var_names:
            self.stack = []  # Стек для хранения версий переменной
            self.counter = 0  # Счетчик для генерации новых версий
            self.traverse_rec(0, target_var)

    def which_pred(self, v, v1):
        """
        Определяет индекс предшественника v для узла v1.
        
        Args:
            v: Номер блока-предшественника
            v1: Номер целевого блока
            
        Returns:
            int: Индекс v в отсортированном списке предшественников v1
        """
        preds = list(self.get_preds(v1))
        preds.sort()
        return preds.index(v)

    def traverse_rec(self, bb, target_var):
        """
        Рекурсивно обходит граф, обновляя версии указанной переменной.
        
        Args:
            bb: Номер текущего базового блока
            target_var: Имя переменной, версии которой обновляются
        """
        if self.verbose:
            print("->>> IN BLOCK", bb)

        # Обрабатываем инструкции в текущем блоке
        for i, instr in enumerate(self.get_block(bb).instructions):
            # Просматриваем аргументы инструкции
            for key, val in instr.args.items():
                # Пропускаем нецелевые переменные
                if not isinstance(val, Variable) or val.is_temp or val.name != target_var:
                    continue
                    
                name = val.name
                
                # Обрабатываем инструкции присваивания (создают новую версию)
                if instr.typ == STORE:
                    new_ver = self.counter
                    self.stack.append(self.counter)
                    self.counter += 1
                    self.get_block(bb).instructions[i].args['to'] = Variable(name, new_ver)
                    
                # Обрабатываем phi-функции (создают новую версию)
                if instr.typ == PHI:
                    new_ver = self.counter
                    self.stack.append(self.counter)
                    self.counter += 1
                    self.get_block(bb).instructions[i].args['to'] = Variable(name, new_ver)
                    
                # Обновляем использования переменных (не phi)
                if instr.typ != PHI:
                    self.get_block(bb).instructions[i].args[key] = Variable(name, self.stack[-1])

        # Обновляем phi-функции в преемниках
        successors = self.get_succ(bb)
        for v1 in successors:
            # Определяем индекс текущего блока среди предшественников v1
            j = self.which_pred(bb, v1)
            
            # Обновляем версии переменных в phi-функциях
            for instr in self.get_block(v1).instructions:
                if instr.typ != PHI or instr.args['to'].name != target_var:
                    continue
                instr.args['from'][j] = (Variable(target_var, self.stack[-1]))

        # Рекурсивно обходим дочерние узлы в дереве доминаторов
        children = self.children[bb]
        for v1 in children:
            self.traverse_rec(v1, target_var)

        # Убираем версию со стека при выходе из определения
        for instr in self.get_block(bb).instructions:
            if instr.typ == STORE and instr.args['to'].name == target_var:
                self.stack.pop()