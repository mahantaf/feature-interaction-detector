//
// Created by Mahan Tafreshipour on 3/29/21.
//

#ifndef STATEMENTHANDLER_H
#define STATEMENTHANDLER_H

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
#include <string>

#include "../Context/Context.h"
#include "../Incident/Incident.h"
#include "../Transpiler/Transpiler.h"
#include "../SymbolTable/SymbolTable.h"

using namespace std;
using namespace llvm;

string getStatementString(const clang::Stmt* stmt);

bool isInIf(const clang::CFGBlock *block, unsigned int previousBlockId);

bool hasFunctionCall(const clang::Stmt* stmt, string stmtClass, vector<string>& names, vector<vector<string>>& paramNames, vector<vector<string>>& paramTypes);

bool hasReturn(const clang::Stmt *stmt);

int hasReturnStmt(const clang::IfStmt *ifStmt);

bool hasElse(const clang::IfStmt *ifStmt);

void getStmtOperands(const clang::Stmt* stmt, set<pair<string, string>>& operands, string type);

#endif //STATEMENTHANDLER_H
