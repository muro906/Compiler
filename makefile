all: parser

OBJS := parser.o  \
		codegen.o \
		main.o    \
		tokens.o  \
		corefn.o  \
		native.o  \

LLVMCONFIG := llvm-config
CPPFLAGS := `$(LLVMCONFIG) --cppflags` -std=c++17
LDFLAGS := `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS := `$(LLVMCONFIG) --libs `

clean:
	$(RM) -rf parser.cpp parser.hpp tokens.cpp parser $(OBJS)
	clear
parser.cpp: parser.y
	bison -d -o $@ $^

parser.hpp : parser.cpp

tokens.cpp: tokens.l parser.hpp
	flex -o $@ $^

%.o: %.cpp
	clang++ -gfull -c $(CPPFLAGS) -o $@ $<

	
parser: $(OBJS)
	clang++  -gfull -o $@ $(OBJS) $(LIBS) $(LDFLAGS)
	clear

test: parser test.txt
	cat test.txt | ./parser

test: parser test2.txt
	cat test2.txt | ./parser

