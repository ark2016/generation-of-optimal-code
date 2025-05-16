"""
Скрипт для автоматической генерации графов CFG и SSA

Запуск:
    python run.py [путь_к_файлу]

Если указан путь к файлу, скрипт будет пытаться разобрать его содержимое.
Иначе используются встроенные примеры программ.
"""

import os
import sys
import subprocess
from ssa import SsaBuilder
from IR import *


def generate_graphs(blocks, name_prefix):
    """
    Генерирует графы CFG и SSA для заданных блоков.
    
    Args:
        blocks: Список базовых блоков
        name_prefix: Префикс для имен выходных файлов
    
    Returns:
        SsaBuilder: Построитель SSA с построенной SSA-формой
    """
    # Создаем директорию для результатов
    os.makedirs('results', exist_ok=True)
    
    # Создаем построитель SSA без подробного вывода
    ssab = SsaBuilder(blocks, verbose=False)
    
    # Генерируем граф потока управления
    cfg_dot_path = f'results/{name_prefix}_cfg.dot'
    with open(cfg_dot_path, 'w') as f:
        f.write(ssab.to_graph())
    
    # Конвертируем DOT в PNG
    subprocess.run(['dot', '-Tpng', cfg_dot_path, '-o', f'results/{name_prefix}_cfg.png'], check=True)
    
    # Строим SSA-форму
    ssab.insert_all_phi()
    ssab.update_variable_versions()
    
    # Генерируем граф SSA
    ssa_dot_path = f'results/{name_prefix}_ssa.dot'
    with open(ssa_dot_path, 'w') as f:
        f.write(ssab.to_graph())
    
    # Конвертируем DOT в PNG
    subprocess.run(['dot', '-Tpng', ssa_dot_path, '-o', f'results/{name_prefix}_ssa.png'], check=True)
    
    return ssab


def process_input_file(file_path):
    """
    Обрабатывает входной файл с кодом.
    
    В текущей версии не реализовано, использует встроенные примеры.
    
    Args:
        file_path: Путь к файлу с исходным кодом
    """
    if not os.path.exists(file_path):
        print(f"Ошибка: файл {file_path} не найден")
        return False
        
    # TODO: Реализовать парсер кода из файла и построение IR
    print(f"Примечание: Чтение из {file_path} пока не реализовано. Используются встроенные примеры.")
    return True


def main():
    """
    Основная функция скрипта.
    
    Обрабатывает аргументы командной строки и запускает генерацию графов.
    """
    # Проверяем наличие аргументов командной строки
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
        if not process_input_file(input_file):
            return
        
    # Создаем директорию для результатов
    os.makedirs('results', exist_ok=True)
    
    # Генерируем графы для примеров
    examples = [
        (example(), "example1", "Ветвление и арифметические операции"),
        (example1(), "example2", "Цикл с суммированием"),
        (example2(), "example3", "Вложенные условия")
    ]
    
    for blocks, name, description in examples:
        print(f"Генерация графов для примера: {description}")
        generate_graphs(blocks, name)
    
    # Выводим информацию о сгенерированных файлах
    print("\nГрафы успешно сгенерированы в директории 'results/':")
    files = []
    for f in os.listdir('results'):
        if f.endswith('.png'):
            files.append(f"- results/{f}")
    
    # Сортируем файлы для более упорядоченного вывода
    for f in sorted(files):
        print(f)


if __name__ == "__main__":
    main() 