# Project built by 
1. Sade Gatheca
2. Purity Chao
3. Millicent Wamuru

# Basic Commands

- Uses references from building kaleindoscope using LLVM
- To build the parser, use command 'make' on terminal
- Any modifications on the dependencies listed in make causes the parser to be rebuilt
- The 'tokens.cpp', 'parser.cpp', 'parser.hpp' files are generated from flex(1) and bison(2)
- Any modifications on the grammar rules or the lexer statements should make the generated files invalid
- Don't modify tokens.cpp, parser.cpp, parser.hpp
- The project is uses LLVM-19. Any use of non-compatible versions may cause errors. E.g if the LLVM    version uses MakeArrayRef(), it's not compatible with this project
- The corefn.cpp creates the core functions used for printing integers and double which are defined in llvm
- To clean all the files run `make clean`. It will delete all the *.o and *.cpp files that are created during compilation
- To run program, `parser <filename.extension>`. The extension type of file doesn't matter but just to be save use '.txt'
- There are two test file available so far: test.txt and test2.txt. You can compile and run test2.txt using `make test2` and the LLVM IR and code results are printed after the other. You can compile and run test.txt using `make test`
- The code was written on Ubuntu 20.0 LTS. Building the project on any other platforms will cause errors especially on windows. There are some header files referenced by flex that are not available on Windows e.g. unistd.h . You can tell flex not to look for these files using various flex options but it may or maynot work.
- The code editor used was VS Code
- The generated IR and output of code are printed on the terminal. Additionally, when you run `make test` or `make test2`, it creates an output file called output.txt where the generated IR and output code are stored. Instead on reading on terminal, you can view that file. Note that the file is truncated each time