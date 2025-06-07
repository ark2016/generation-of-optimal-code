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
import json
from ssa import SsaBuilder
from IR import *
from parser import Parser


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
    with open(cfg_dot_path, 'w', encoding='utf-8') as f:
        f.write(ssab.to_graph())
    
    # Конвертируем DOT в PNG
    subprocess.run(['dot', '-Tpng', cfg_dot_path, '-o', f'results/{name_prefix}_cfg.png'], check=True)
    
    # Создаем интерактивный граф CFG в формате D3.js
    generate_d3_graph(ssab, f'results/{name_prefix}_cfg_interactive.html', is_ssa=False)
    
    # Строим SSA-форму
    ssab.insert_all_phi()
    ssab.update_variable_versions()
    
    # Генерируем граф SSA
    ssa_dot_path = f'results/{name_prefix}_ssa.dot'
    with open(ssa_dot_path, 'w', encoding='utf-8') as f:
        f.write(ssab.to_graph())
    
    # Конвертируем DOT в PNG
    subprocess.run(['dot', '-Tpng', ssa_dot_path, '-o', f'results/{name_prefix}_ssa.png'], check=True)
    
    # Создаем интерактивный граф SSA в формате D3.js
    generate_d3_graph(ssab, f'results/{name_prefix}_ssa_interactive.html', is_ssa=True)
    
    return ssab


def generate_d3_graph(ssab, output_path, is_ssa=False):
    """
    Генерирует интерактивный граф с использованием D3.js.
    
    Args:
        ssab: Построитель SSA
        output_path: Путь к выходному HTML-файлу
        is_ssa: Флаг, указывающий, является ли граф SSA-формой
    """
    # Создаем данные для D3.js
    graph_data = {
        "nodes": [],
        "links": []
    }
    
    # Добавляем узлы
    for block in ssab.blocks:
        # Преобразуем содержимое блока в строку
        content = str(block).replace('    ', '').replace('{', '').replace('}', '').strip()
        content = content.replace('\n', '<br>')
        
        # Добавляем узел
        graph_data["nodes"].append({
            "id": str(block.block_num),
            "label": f"BLOCK {block.block_num}",
            "content": content
        })
    
    # Добавляем ребра
    for block in ssab.blocks:
        # Получаем исходящие ребра из блока
        edges = block.get_edges()
        
        for src, dest in edges:
            # Определяем тип ребра (true/false) для условных переходов
            edge_type = None
            if len(block.instructions) > 0:
                last_instr = block.instructions[-1]
                if last_instr.typ == CONDBR:
                    if dest == last_instr.args["dest1"]:
                        edge_type = "true"
                    else:
                        edge_type = "false"
            
            # Добавляем ребро
            link = {
                "source": str(src),
                "target": str(dest)
            }
            if edge_type:
                link["type"] = edge_type
            
            graph_data["links"].append(link)
    
    # Шаблон HTML с D3.js
    html_template = """
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>Интерактивный граф {graph_type}</title>
        <script src="https://d3js.org/d3.v7.min.js"></script>
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 0;
                background-color: #f5f5f5;
            }}
            .container {{
                display: flex;
                width: 100%;
            }}
            .graph-container {{
                flex: 1;
                height: 100vh;
                overflow: hidden;
            }}
            .info-panel {{
                width: 300px;
                background-color: #fff;
                padding: 20px;
                box-shadow: -2px 0 5px rgba(0, 0, 0, 0.1);
                height: 100vh;
                overflow-y: auto;
            }}
            svg {{
                width: 100%;
                height: 100%;
            }}
            .node {{
                cursor: pointer;
            }}
            .node circle {{
                fill: #4285f4;
                stroke: #fff;
                stroke-width: 2px;
            }}
            .node:hover circle {{
                fill: #ea4335;
            }}
            .node text {{
                font-size: 12px;
            }}
            .link {{
                stroke: #999;
                stroke-opacity: 0.6;
            }}
            .link.true {{
                stroke: #0c0;
                stroke-width: 2px;
            }}
            .link.false {{
                stroke: #c00;
                stroke-width: 2px;
            }}
            .node-info {{
                margin-bottom: 20px;
            }}
            h1, h2 {{
                color: #333;
                border-bottom: 1px solid #eee;
                padding-bottom: 10px;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <div class="graph-container" id="graph"></div>
            <div class="info-panel">
                <h1>{graph_type}</h1>
                <p>Кликните на узел для просмотра деталей.</p>
                <div id="nodeInfo" class="node-info">
                    <h2>Выберите блок</h2>
                    <p>Информация о выбранном блоке будет отображена здесь.</p>
                </div>
            </div>
        </div>

        <script>
            // Данные графа
            const graphData = {graph_data};
            
            // Настройка визуализации
            const width = document.getElementById('graph').clientWidth;
            const height = document.getElementById('graph').clientHeight;
            
            // Создание SVG
            const svg = d3.select('#graph')
                .append('svg')
                .attr('width', width)
                .attr('height', height);
            
            // Создание контейнера для графа
            const g = svg.append('g');
            
            // Добавляем масштабирование и перемещение
            const zoom = d3.zoom()
                .scaleExtent([0.1, 4])
                .on('zoom', (event) => {{
                    g.attr('transform', event.transform);
                }});
            svg.call(zoom);
            
            // Создание силового макета
            const simulation = d3.forceSimulation(graphData.nodes)
                .force('link', d3.forceLink(graphData.links).id(d => d.id).distance(150))
                .force('charge', d3.forceManyBody().strength(-300))
                .force('center', d3.forceCenter(width / 2, height / 2))
                .force('x', d3.forceX(width / 2).strength(0.1))
                .force('y', d3.forceY(height / 2).strength(0.1));
            
            // Добавление ребер
            const link = g.append('g')
                .selectAll('line')
                .data(graphData.links)
                .enter()
                .append('line')
                .attr('class', d => `link ${{d.type}}`)
                .attr('marker-end', d => d.type ? `url(#arrow-${{d.type}})` : 'url(#arrow)');
            
            // Добавление узлов
            const node = g.append('g')
                .selectAll('.node')
                .data(graphData.nodes)
                .enter()
                .append('g')
                .attr('class', 'node')
                .call(d3.drag()
                    .on('start', dragstarted)
                    .on('drag', dragged)
                    .on('end', dragended));
            
            // Добавление круга для узла
            node.append('circle')
                .attr('r', 20);
            
            // Добавление текста узла
            node.append('text')
                .attr('dy', 4)
                .attr('text-anchor', 'middle')
                .text(d => d.id);
            
            // Обработка клика по узлу
            node.on('click', showNodeInfo);
            
            // Обновление позиций при симуляции
            simulation.on('tick', () => {{
                link
                    .attr('x1', d => d.source.x)
                    .attr('y1', d => d.source.y)
                    .attr('x2', d => d.target.x)
                    .attr('y2', d => d.target.y);
                
                node
                    .attr('transform', d => `translate(${{d.x}},${{d.y}})`);
            }});
            
            // Функции для перетаскивания узлов
            function dragstarted(event, d) {{
                if (!event.active) simulation.alphaTarget(0.3).restart();
                d.fx = d.x;
                d.fy = d.y;
            }}
            
            function dragged(event, d) {{
                d.fx = event.x;
                d.fy = event.y;
            }}
            
            function dragended(event, d) {{
                if (!event.active) simulation.alphaTarget(0);
                d.fx = null;
                d.fy = null;
            }}
            
            // Показать информацию о узле
            function showNodeInfo(event, d) {{
                const nodeInfo = document.getElementById('nodeInfo');
                nodeInfo.innerHTML = `
                    <h2>Block ${{d.id}}</h2>
                    <div>${{d.content}}</div>
                `;
            }}
        </script>
    </body>
    </html>
    """
    
    # Определяем тип графа
    graph_type = "SSA Form" if is_ssa else "Control Flow Graph"
    
    # Создаем HTML-файл
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(html_template.format(
            graph_type=graph_type,
            graph_data=json.dumps(graph_data)
        ))


def process_input_file(file_path):
    """
    Обрабатывает входной файл с кодом.
    
    Args:
        file_path: Путь к файлу с исходным кодом
        
    Returns:
        bool: True если обработка успешна, False в противном случае
    """
    if not os.path.exists(file_path):
        print(f"Ошибка: файл {file_path} не найден")
        return False
    
    try:
        # Читаем содержимое файла
        with open(file_path, 'r', encoding='utf-8') as f:
            code = f.read()
        
        # Создаем парсер и преобразуем код в IR
        parser = Parser()
        blocks = parser.parse(code)
        
        # Генерируем графы
        name_prefix = os.path.splitext(os.path.basename(file_path))[0]
        generate_graphs(blocks, name_prefix)
        
        return True
    except Exception as e:
        print(f"Ошибка при обработке файла: {e}")
        return False


def main():
    """
    Основная функция скрипта.
    
    Обрабатывает аргументы командной строки и запускает генерацию графов.
    """
    # Проверяем наличие аргументов командной строки
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
        if process_input_file(input_file):
            print(f"Графы успешно сгенерированы для файла {input_file}")
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
        if f.endswith('.png') or f.endswith('.html'):
            files.append(f"- results/{f}")
    
    # Сортируем файлы для более упорядоченного вывода
    for f in sorted(files):
        print(f)


if __name__ == "__main__":
    main() 