//
// Created by mahan on 3/31/21.
//

#ifndef CFGBLOCKHANDLER_H
#define CFGBLOCKHANDLER_H

#include <clang/AST/Stmt.h>
#include <clang/Lex/Lexer.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Analysis/CFG.h>


#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <iostream>
#include <set>
#include <vector>
#include <string>

#include "../Context/Context.h"
#include "../StatementHandler/StatementHandler.h"

using namespace std;
using namespace llvm;

class CFGBlockHandler {
public:
    CFGBlockHandler();

    vector<vector<string>> getBlockCondition(const clang::CFGBlock* block, vector<string>& constraints);

    string getIfStmtCondition(const clang::Stmt* statement, bool _if);

    string getTerminatorBlockCondition(const clang::CFGBlock* block, unsigned int previousBlockId, bool collect);

    void initializeConstraints(vector<string>& constraints);

    void addStatement(string& statement);

    void addFunction(vector<vector<string>>& functionConstraintsList);

private:
    FileSystem fs;
    Transpiler transpiler;
    vector<vector<string>> constraintsList;
};


#endif //CFGBLOCKHANDLER_H
