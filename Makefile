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

all: file_system symbol_table transpiler context statement_handler incident cfg main clear

.phony: clean
.phony: run

clean:
	rm $(TARGET) || echo -n ""

cfg:  $(TARGET).cpp
	$(CXX) -c $(HEADERS) $(LDFLAGS) $(CXXFLAGS) $(TARGET).cpp
file_system: include/FileSystem/FileSystem.cpp
	$(CXX) -c $(CXXFLAGS) include/FileSystem/FileSystem.cpp
symbol_table: include/SymbolTable/SymbolTable.cpp
	$(CXX) -c $(CXXFLAGS) include/SymbolTable/SymbolTable.cpp
transpiler: include/Transpiler/Transpiler.cpp
	$(CXX) -c $(CXXFLAGS) include/Transpiler/Transpiler.cpp
context: include/Context/Context.cpp
	$(CXX) -c $(HEADERS) $(LDFLAGS) $(CXXFLAGS) include/Context/Context.cpp
statement_handler: include/StatementHandler/StatementHandler.cpp
	$(CXX) -c $(HEADERS) $(LDFLAGS) $(CXXFLAGS) include/StatementHandler/StatementHandler.cpp
incident: include/Incident/Incident.cpp
	$(CXX) -c $(HEADERS) $(LDFLAGS) $(CXXFLAGS) include/Incident/Incident.cpp
main:
	$(CXX) $(HEADERS) $(LDFLAGS) $(CXXFLAGS) FileSystem.o SymbolTable.o Transpiler.o Context.o StatementHandler.o Incident.o cfg.o -o cfg $(LIBS)
clear:
	rm *.o
