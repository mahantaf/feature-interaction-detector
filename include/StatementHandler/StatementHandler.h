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

#include <llvm/ADT/StringRef.h>

#include <string>

#include "../Context/Context.h"

using namespace std;

string getStatementString(const clang::Stmt* stmt);

#endif //STATEMENTHANDLER_H
