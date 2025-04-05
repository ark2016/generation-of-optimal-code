# lab1
## start

Скомпилируем программу с подключением поагина
```
gcc -fplugin=./lab1.so -O0 src/test.c -o test
```

Запуск тестовой программы
```
./test
```

## Вывод
```
func: "sum" {
        bb: (0) -> (2) -> (3) {
                stmt: "GIMPLE_ASSIGN" { res__v3 = a__v1 + b__v2 }
                stmt: "GIMPLE_ASSIGN" { ssa_name__v4 = res__v3 }
        }
        bb: (2) -> (3) -> (1) {
                stmt: "GIMPLE_LABEL" { }
                stmt: "GIMPLE_RETURN" { }
        }
}

func: "main" {
        bb: (0) -> (2) -> (3, 4) {
                stmt: "GIMPLE_ASSIGN" { n__v7 = 3 }
                stmt: "GIMPLE_ASSIGN" { ssa_name__v1 = n__v7 }
                stmt: "GIMPLE_ASSIGN" { ssa_name__v2 = ssa_name__v1 * 4 }
                stmt: "GIMPLE_CALL" { arr__v10 = malloc(ssa_name__v2) }
                stmt: "GIMPLE_ASSIGN" { a__v11 = 5 }
                stmt: "GIMPLE_ASSIGN" { b__v12 = 1 }
                stmt: "GIMPLE_ASSIGN" { c__v13 = 6 }
                stmt: "GIMPLE_CALL" { ssa_name__v3 = rand() }
                stmt: "GIMPLE_COND" { ssa_name__v3 > 0 }
        }
        bb: (2) -> (3) -> (5) {
                stmt: "GIMPLE_ASSIGN" { a__v16 = a__v11 + b__v12 }
        }
        bb: (2) -> (4) -> (5) {
                stmt: "GIMPLE_ASSIGN" { a__v15 = a__v11 - c__v13 }
        }
        bb: (3, 4) -> (5) -> (6) {
                stmt: "GIMPLE_CALL" { ssa_name__v4 = sum((a__v6 = GIMPLE_PHI(a__v16, a__v15)), b__v12) }
                stmt: "GIMPLE_CALL" { printf(tree_code(131), (a__v6 = GIMPLE_PHI(a__v16, a__v15)), b__v12, ssa_name__v4) }
                stmt: "GIMPLE_CALL" { ssa_name__v5 = sum((a__v6 = GIMPLE_PHI(a__v16, a__v15)), b__v12) }
                stmt: "GIMPLE_ASSIGN" { (*arr__v10) = ssa_name__v5 }
                stmt: "GIMPLE_ASSIGN" { z__v21 = (*arr__v10) }
                stmt: "GIMPLE_CALL" { printf(tree_code(131), z__v21) }
                stmt: "GIMPLE_CALL" { free(arr__v10) }
                stmt: "GIMPLE_ASSIGN" { ssa_name__v24 = 0 }
        }
        bb: (5) -> (6) -> (1) {
                stmt: "GIMPLE_LABEL" { }
                stmt: "GIMPLE_RETURN" { }
        }
}
```
