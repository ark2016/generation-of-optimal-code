# LLVM Lab 2 — Простейший компилятор

Программа генерирует LLVM IR, эквивалентный следующей функции на C++:

```cpp
int main() {
    return 353 + 48;
}
````

С помощью API LLVM мы создаём IR-код напрямую, не компилируя C++-исходник.

---

## 🔧 Зависимости

Убедитесь, что установлены:

- `clang++`
    
- `llvm`
    
- `llvm-config`
    

Установить их можно командой:

```bash
sudo apt install llvm clang
```

---

## 🚀 Сборка и запуск

```bash
clang++ lab2.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o lab2
./lab2
```

---

## ✅ Ожидаемый вывод

```
; ModuleID = 'llvm_lab2'
source_filename = "llvm_lab2"

define i32 @main() {
entry:
  ret i32 401
}
```

Вывод означает, что сгенерирован LLVM IR с функцией `main`, возвращающей результат `353 + 48 = 401`.
