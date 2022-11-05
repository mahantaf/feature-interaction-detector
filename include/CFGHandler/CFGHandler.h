//
// Created by mahan on 3/31/21.
//

#ifndef CFGHANDLER_H
#define CFGHANDLER_H

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

#include "../Path/Path.h"
#include "../Context/Context.h"
#include "../FileSystem/FileSystem.h"
#include "../Incident/Incident.h"
#include "../CFGBlockHandler/CFGBlockHandler.h"
#include "../StatementHandler/StatementHandler.h"

using namespace std;
using namespace llvm;

class CFGHandler {
public:
    CFGHandler(const unique_ptr<clang::CFG>& cfg);

    bool hasIncident();

    void findIncidents();

    void findChildStatementIncident(const clang::Stmt* stmt, bool& varInFuncLock, bool& varInFuncTypeLock);

    void findStatementIncident(const clang::Stmt* stmt, int& varInFuncCounter, string& varInFuncCount);

    void bottomUpTraverse(const clang::CFGBlock* startBlock, unsigned int previousBlockId, vector<string>& constraints, bool previousCollect);

    vector<vector<string>> readConstraintsList();

    void clearConstraintsList();

    void collectConstraints();

    void writePaths();

    void writeVarWritePaths();

    void writeConstraintsList(vector<vector<string>> constraintsList);

    unsigned int getEntryBlockId();

    vector<const clang::CFGBlock*> getIncidentBlocks();

    vector<string> getReturnValues();

    void removeDuplicatePaths();

private:
    const unique_ptr<clang::CFG>* cfg;
    FileSystem fs;
    CFGBlockHandler cfgBlockHandler;
    vector<Path> paths;
    Incident* incident;
    vector<string> returnValues;
    vector<const clang::CFGBlock*> incidentBlocks;
    vector<const clang::Stmt*> incidentStatements;
};


#endif //CFGHANDLER_H
