# Lab3 LLVM Compiler

## Requirements  
- LLVM (with `llvm-config` in your PATH)  
- C++17-compatible compiler (e.g. `g++`)  

## Compilation  
```bash
g++ lab3.cpp -g -O3 -std=c++17 -Wno-format \
  `llvm-config --system-libs --cppflags --ldflags --libs core` \
  -o lab3
```

## Usage  
Prepare your input file, e.g. `example.txt`.  
   ```bash
   ./lab3 example.txt
   ```  
