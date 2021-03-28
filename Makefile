TARGET := cfg
HEADERS := -isystem `llvm-config --includedir`
WARNINGS := -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS := $(WARNINGS) -std=c++14 -fpermissive -fno-exceptions -O3 -Os #-fno-rtti
LDFLAGS := `llvm-config --ldflags`

CLANG_LIBS := \
  -lclangFrontendTool \
  -lclangRewriteFrontend \
  -lclangDynamicASTMatchers \
  -lclangToolingCore \
  -lclangTooling \
  -lclangFrontend \
  -lclangASTMatchers \
  -lclangParse \
  -lclangDriver \
  -lclangSerialization \
  -lclangRewrite \
  -lclangSema \
  -lclangEdit \
  -lclangAnalysis \
  -lclangAST \
  -lclangLex \
  -lclangBasic

LIBS := $(CLANG_LIBS) `llvm-config --libs --system-libs`

all: file_system symbol_table transpiler cfg main clear

.phony: clean
.phony: run

clean:
	rm $(TARGET) || echo -n ""

cfg:  $(TARGET).cpp
	$(CXX) -c $(HEADERS) $(LDFLAGS) $(CXXFLAGS) $(TARGET).cpp
file_system: include/FileSystem/FileSystem.cpp
	$(CXX) -c include/FileSystem/FileSystem.cpp
symbol_table: include/SymbolTable/SymbolTable.cpp
	$(CXX) -c include/SymbolTable/SymbolTable.cpp
transpiler: include/Transpiler/Transpiler.cpp
	$(CXX) -c include/Transpiler/Transpiler.cpp
main:
	$(CXX) $(HEADERS) $(LDFLAGS) $(CXXFLAGS) FileSystem.o SymbolTable.o Transpiler.o cfg.o -o cfg $(LIBS)
clear:
	rm *.o
