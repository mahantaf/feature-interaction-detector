//
// Created by Mahan on 11/6/20.
//

// Clang includes
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/Inclusions/HeaderIncludes.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>

// LLVM includes
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <map>
#include <set>
#include <stdlib.h>
#include <thread>
#include <exception>

#include "include/Context/Context.h"
#include "include/Incident/Incident.h"
#include "include/FileSystem/FileSystem.h"
#include "include/Transpiler/Transpiler.h"
#include "include/SymbolTable/SymbolTable.h"
#include "include/StatementHandler/StatementHandler.h"

using namespace std;
using namespace llvm;

string filePath, functionName, incident, incidentType, root, varInFuncCount;

int varInFuncCounter = 0;
bool varInFuncLock = false;
bool varInFuncTypeLock = false;


Incident* createIncident() {
    if (incidentType.compare("CALL") == 0) {
        return new CallIncident(incident);
    }
    if (incidentType.compare("WRITE") == 0) {
        return new WriteIncident(incident);
    }
    if (incidentType.compare("RETURN") == 0) {
        return new ReturnIncident(incident);
    }
    if (incidentType.compare("FUNCTION") == 0) {
        return new FunctionIncident(incident);
    }
    if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {
        return new VarInFuncIncident(incident);
    }
    if (incidentType.compare("VARWRITE") == 0) {
        return new VarWriteIncident(incident);
    }
    return nullptr;
}

void getStmtOperands(const clang::Stmt* stmt, set<pair<string, string>>& operands, string type) {
    const string stmtClass(stmt->getStmtClassName());
//    cout << getStatementString(stmt) << ' ' << stmtClass << ' ' << type << endl;
    if (stmtClass.compare("CXXOperatorCallExpr") == 0) {
        const clang::CXXOperatorCallExpr* cxxOperatorCallExpr = cast<clang::CXXOperatorCallExpr>(stmt);
        const clang::Stmt *lhs, *rhs;
        int i = 0;
        for (clang::ConstStmtIterator it = stmt->child_begin(); it != stmt->child_end(); it++) {
            if (i == 0) {
                ++i;
                continue;
            }
            if (i == 1)
                lhs = (*it);
            if (i == 2)
                rhs = (*it);
            ++i;
        }
        if (lhs && rhs) {
            if (cxxOperatorCallExpr->isAssignmentOp()) {
                getStmtOperands(lhs, operands, "LVALUE");
                if (operands.size())
                    getStmtOperands(rhs, operands, "RVALUE");
            } else {
                getStmtOperands(lhs, operands, "COND");
                getStmtOperands(rhs, operands, "COND");
            }
        }
    }
    if (stmtClass.compare("BinaryOperator") == 0 || stmtClass.compare("CompoundAssignOperator") == 0) {

        const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
        const clang::Stmt *lhs = binaryOperator->getLHS();
        const clang::Stmt *rhs = binaryOperator->getRHS();

        if (binaryOperator->isCompoundAssignmentOp()) {
            Incident* incident = createIncident();
            vector<string> temp;
            if ((incident->getType().compare("WRITE") == 0 || incident->getType().compare("VARWRITE") == 0) && incident->hasIncident(stmt, temp)) {
                getStmtOperands(lhs, operands, "IVALUE");
            } else {
                getStmtOperands(lhs, operands, "LVALUE");
            }
//            getStmtOperands(lhs, operands, "LVALUE");
            if (operands.size()) {
                getStmtOperands(lhs, operands, "RVALUE");
                getStmtOperands(rhs, operands, "RVALUE");
            }

        } else if (binaryOperator->isAssignmentOp()) {
            Incident* incident = createIncident();
            vector<string> temp;
            if ((incident->getType().compare("WRITE") == 0 || incident->getType().compare("VARWRITE") == 0) && incident->hasIncident(stmt, temp)) {
                getStmtOperands(lhs, operands, "IVALUE");
            } else {
                getStmtOperands(lhs, operands, "LVALUE");
            }
            if (operands.size())
                getStmtOperands(rhs, operands, "RVALUE");
        } else {
            getStmtOperands(lhs, operands, type);
            getStmtOperands(rhs, operands, type);
        }
    }
    else if (stmtClass.compare("UnaryOperator") == 0) {
        const clang::Stmt *subExpr = cast<clang::UnaryOperator>(stmt)->getSubExpr();
        getStmtOperands(subExpr, operands, type);
    }
    else if (stmtClass.compare("ImplicitCastExpr") == 0 || stmtClass.compare("DeclRefExpr") == 0 || stmtClass.compare("MemberExpr") == 0) {
        if (type.compare("")) {
            if (stmtClass.compare("DeclRefExpr") == 0) {
                string declType = cast<clang::DeclRefExpr>(stmt)->getDecl()->getType().getAsString();
                vector<string> splitDecl = Transpiler::split(declType, ' ');
                string exactDeclType = splitDecl[0];
                if (exactDeclType.compare("enum") == 0) {
                    return;
                }
            } else if (stmtClass.compare("ImplicitCastExpr") == 0) {
                const clang::Stmt* subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
                string subExprClassName(subExpr->getStmtClassName());
                if (subExprClassName.compare("DeclRefExpr") == 0)
                    return getStmtOperands(subExpr, operands, type);
            }
            string var = getStatementString(stmt);
            SymbolTable *st = st->getInstance();
            string varSymbol = st->addVariableSymbol(var, type);
            if (varSymbol.compare("")) {
                pair <string, string> op(var, varSymbol);
                operands.insert(op);
            }
        }

    }
    else if (stmtClass.compare("ParenExpr") == 0) {
        const clang::ParenExpr *parenExpr = cast<clang::ParenExpr>(stmt);
        const clang::Stmt *subParen = parenExpr->getSubExpr();
        getStmtOperands(subParen, operands, type);
    }
    else if (stmtClass.compare("DeclStmt") == 0) {
        const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
        const clang::Decl* declaration = declStmt->getSingleDecl();
        if (declaration) {
            const clang::VarDecl* varDecl = cast<clang::VarDecl>(declaration);
            if (varDecl && varDecl->hasInit()) {
                const clang::Stmt* rhs = varDecl->getInit();
                string var = varDecl->getNameAsString();
                SymbolTable *st = st->getInstance();

                Incident* incident = createIncident();
                vector<string> temp;
                string varSymbol = "";
                if (incident->getType().compare("VARWRITE") == 0 && incident->hasIncident(stmt, temp)) {
                    varSymbol = st->addVariableSymbol(var, "IVALUE");
                } else {
                    varSymbol = st->addVariableSymbol(var, "LVALUE");
                }
                if (varSymbol.compare("")) {
                    pair <string, string> op(var, varSymbol);
                    operands.insert(op);
                }
                if (operands.size())
                    getStmtOperands(rhs, operands, "RVALUE");
            }
        }
    }
    else if (stmtClass.compare("ReturnStmt") == 0 && incidentType.compare("RETURN") == 0) {
        const clang::Stmt* returnValue = cast<clang::ReturnStmt>(stmt)->getRetValue();
        if (returnValue) {
            string returnValueClass(returnValue->getStmtClassName());
            if (returnValueClass.find("Literal") != string::npos) {
                pair <string, string> op(getStatementString(returnValue), getStatementString(returnValue));
                operands.insert(op);
            } else {
                getStmtOperands(returnValue, operands, "RVALUE");
            }
        }
    }
}

bool hasFunctionCall(const clang::Stmt* stmt, string stmtClass, vector<string>& names, vector<vector<string>>& paramNames, vector<vector<string>>& paramTypes) {
    cout << getStatementString(stmt) << " " << stmtClass << endl;
    if (stmtClass.compare("BinaryOperator") == 0 || stmtClass.compare("CompoundAssignOperator") == 0) {
        const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
        const clang::Stmt *lhs = binaryOperator->getLHS();
        const clang::Stmt *rhs = binaryOperator->getRHS();
        return hasFunctionCall(lhs, lhs->getStmtClassName(), names, paramNames, paramTypes) ||
               hasFunctionCall(rhs, rhs->getStmtClassName(), names, paramNames, paramTypes);
    }
    else if (stmtClass.compare("ImplicitCastExpr") == 0) {

        const clang::Stmt *subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
        return hasFunctionCall(subExpr, subExpr->getStmtClassName(), names, paramNames, paramTypes);

    }
    else if (stmtClass.compare("CallExpr") == 0 || stmtClass.compare("CXXMemberCallExpr") == 0) {

        const clang::CallExpr *callExpr = cast<clang::CallExpr>(stmt);

        const clang::Stmt *callee = callExpr->getCallee();
        const clang::Expr *const *args = callExpr->getArgs();
        const clang::FunctionDecl *functionDecl = callExpr->getDirectCallee();

        if (functionDecl) {
            vector <string> params;
            vector <string> paramType;
            for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i) {
                params.push_back(getStatementString(args[i]));
                paramType.push_back(args[i]->getStmtClassName());
            }
            paramNames.push_back(params);
            paramTypes.push_back(paramType);
            names.push_back(functionDecl->getNameInfo().getAsString());
            return true;
        }
    }
    else if (stmtClass.compare("DeclStmt") == 0)
    {
        const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
        const clang::Decl* declaration = declStmt->getSingleDecl();
        if (declaration) {
            const clang::VarDecl *varDecl = cast<clang::VarDecl>(declaration);
            cout << "VarDecl: " << varDecl->getNameAsString() << " has init: " << varDecl->hasInit() << endl;
             if (varDecl && varDecl->hasInit()) {
                cout << "Has init:\n";
                const clang::Stmt* rhs = varDecl->getInit();
                return hasFunctionCall(rhs, rhs->getStmtClassName(), names, paramNames, paramTypes);
            }
        }
    }
    return false;
}

class Path {
public:
    Path(vector<string> constraints) {
        this->constraints = constraints;
    }

    bool compare(Path path) {
        vector<string> pathConstraints = path.getConstraints();
        if (pathConstraints.size() != this->constraints.size())
            return false;

        for (int i = 0; i < this->constraints.size(); i++)
            if (this->constraints[i].compare(pathConstraints[i]) != 0)
                return false;
        return true;
    }

    vector<string> getConstraints() {
        return this->constraints;
    }
private:
    vector<string> constraints;
};

class CFGBlockHandler {
public:
    CFGBlockHandler() {
        this->fs = FileSystem();
        this->transpiler = Transpiler();
    }

    bool isInIf(const clang::CFGBlock *block, unsigned int previousBlockId) {
        int i = 0;
        for (clang::CFGBlock::const_succ_iterator I = block->succ_begin(), E = block->succ_end(); I != E; I++) {
            if (((*I).getReachableBlock()->getBlockID() == previousBlockId) && i == 0)
                return true;
            return false;
            i++;
        }
        return false;
    }

    bool hasReturn(const clang::Stmt *stmt) {
        for (clang::ConstStmtIterator it = stmt->child_begin(); it != stmt->child_end(); it++) {
            const clang::Stmt *child = (*it);
            string stmtClass(child->getStmtClassName());
            if (stmtClass.compare("ReturnStmt") == 0)
                return true;
        }
        string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("ReturnStmt") == 0)
            return true;
        return false;
    }

    int hasReturnStmt(const clang::IfStmt *ifStmt) {
        const clang::Stmt *thenStatement = ifStmt->getThen();
        const clang::Stmt *elseStatement = ifStmt->getElse();

        if (thenStatement && this->hasReturn(thenStatement))
            return 1;
        if (elseStatement && this->hasReturn(elseStatement))
            return 0;
        return -1;
    }

    bool hasElse(const clang::IfStmt *ifStmt) {
        const clang::Stmt *elseStatement = ifStmt->getElse();
        if (elseStatement)
            return true;
        return false;
    }

    vector<vector<string>> getBlockCondition(const clang::CFGBlock* block, vector<string>& constraints) {
        this->initializeConstraints(constraints);

        string terminator = "";
        const clang::Stmt *terminatorStmt = block->getTerminatorStmt();
        if (terminatorStmt && terminatorStmt->getStmtClass() != clang::Stmt::IfStmtClass)
            terminator = getStatementString(terminatorStmt);

        for (clang::CFGBlock::const_reverse_iterator I = block->rbegin(), E = block->rend(); I != E ; ++I) {
            clang::CFGElement El = *I;
            if (auto SE = El.getAs<clang::CFGStmt>()) {

                const clang::Stmt *stmt = SE->getStmt();

                string statement = getStatementString(stmt);
                if (terminator.compare("") != 0 && terminator.find(statement) != std::string::npos)
                    continue;

                string stmtClass(stmt->getStmtClassName());
                set<pair<string, string>> operands;
                getStmtOperands(stmt, operands, "");

                vector<string> funcNames;
                vector<vector<string>> paramNames;
                vector<vector<string>> paramTypes;
                if (operands.size() && stmtClass.compare("CallExpr") && stmtClass.compare("CXXMemberCallExpr") && hasFunctionCall(stmt, stmtClass, funcNames, paramNames, paramTypes)) {
                    int i = 0;
                    for (string name: funcNames) {
                        cout << "Function name: " << name << endl;
                        SymbolTable* se = se->getInstance();
                        se->saveState();
                        se->saveParameters(name, paramNames[i]);
                        se->saveParameterTypes(name, paramNames[i], paramTypes[i]);

                        string sysCall = "./cfg " + filePath + " -- " + name + " c RETURN 0";
                        cout << sysCall << endl;
                        system(sysCall.c_str());

                        se->loadState();

                        vector<string> returnValues;
                        vector<vector<string>> functionConstraintsList = this->fs.readReturnConstraints(returnValues);

                        int index = 0;
                        string copyStatement = statement;
                        for (string returnValue: returnValues) {
                            statement = copyStatement;
                            string ts = this->transpiler.replaceFunction(statement, stmtClass, name, returnValue, operands);
                            functionConstraintsList[index].insert(functionConstraintsList[index].begin(), ts);
                            index++;
                        }

                        this->addFunction(functionConstraintsList);
                        i++;
                    }
                }
                else {
                    this->transpiler.replaceStatement(statement, stmtClass, operands);
                    if (operands.size())
                        this->addStatement(statement);

                    if (stmtClass.compare("ReturnStmt") == 0)
                        ++I;
                }
            }
        }
        return constraintsList;
    }

    const string getIfStmtCondition(const clang::Stmt* statement, bool _if) {
        set<pair<string, string>> operands;

        getStmtOperands(statement, operands, "COND");

        string stmt = getStatementString(statement);
        this->transpiler.replaceVariables(stmt, operands);

        return _if ? (stmt) : ("!(" + stmt + ")");
    }

    const string getTerminatorCondition(const clang::CFGBlock* block, unsigned int previousBlockId, bool collect) {
        const clang::Stmt *terminatorStmt = block->getTerminatorStmt();
        if (terminatorStmt && terminatorStmt->getStmtClass() == clang::Stmt::IfStmtClass) {
            const clang::IfStmt *ifStmt = cast<clang::IfStmt>(terminatorStmt);
            int hasReturn = this->hasReturnStmt(ifStmt);
            if (hasReturn != -1 || collect) {
                if (!(hasReturn != -1) && !isInIf(block, previousBlockId) && !hasElse(ifStmt))
                    return "";
                set<pair<string, string>> operands;
                getStmtOperands(block->getTerminatorCondition(), operands, "COND");

                string stmt;
                if (collect) {
                    if (this->isInIf(block, previousBlockId))
                        stmt = getStatementString(block->getTerminatorCondition());
                    else
                        stmt = "!(" + getStatementString(block->getTerminatorCondition()) + ")";
                } else {
                    if (hasReturn == 0)
                        stmt = getStatementString(block->getTerminatorCondition());
                    else
                        stmt = "!(" + getStatementString(block->getTerminatorCondition()) + ")";
                }
                this->transpiler.replaceVariables(stmt, operands);
                return stmt;
            }
        }
        return "";
    }

    void initializeConstraints(vector<string>& constraints) {
        this->constraintsList.clear();
        this->constraintsList.push_back(constraints);
    }

    void addStatement(string& statement) {
        for (int i = 0; i < this->constraintsList.size(); i++)
            this->constraintsList[i].push_back(statement);
    }

    void addFunction(vector<vector<string>>& functionConstraintsList) {
        if (functionConstraintsList.size()) {
            vector<vector<string>> newConstraintsList;
            for (vector<string> c: this->constraintsList) {
                for (vector<string> fc: functionConstraintsList) {
                  vector<string> newConstraints = c;
                  newConstraints.insert(newConstraints.end(), fc.begin(), fc.end());
                  newConstraintsList.push_back(newConstraints);
                }
            }
            this->constraintsList = newConstraintsList;
        }
    }

private:
    FileSystem fs;
    Transpiler transpiler;
    vector<vector<string>> constraintsList;
};

class CFGHandler {
public:
    CFGHandler(const unique_ptr<clang::CFG>& cfg) {
        this->fs = FileSystem();
        this->cfgBlockHandler = CFGBlockHandler();
        this->incident = createIncident();
        if (cfg)
            this->cfg = &cfg;
    }

    bool hasIncident() {
        return this->incidentBlocks.size() > 0;
    }

    void findIncidents() {
        for (const auto* blk : **(this->cfg)) {
            for (clang::CFGBlock::const_iterator I = blk->begin(), E = blk->end(); I != E; ++I) {
                clang::CFGElement El = *I;
                if (auto SE = El.getAs<clang::CFGStmt>()) {
                    const clang::Stmt *stmt = SE->getStmt();
                    if (this->incident->hasIncident(stmt, this->returnValues)) {
                        this->incidentBlocks.push_back(blk);
                        this->incidentStatements.push_back(stmt);
                    }
                }
            }
            blk->dump();
        }
    }

    void findChildStatementIncident(const clang::Stmt* stmt) {
        VarInFuncIncident* varInFuncIncident = dynamic_cast<VarInFuncIncident*>(this->incident);
        vector<const clang::Stmt*> incidentValues;
        int result = varInFuncIncident->hasIncidentExtend(stmt, incidentValues);
        if (result != -1) {
            this->fs.writeVarInFuncParameters(varInFuncIncident->getParams(), varInFuncLock);
            this->fs.writeVarInFuncParametersType(varInFuncIncident->getParamTypes(), varInFuncTypeLock);
        }
    }

    void findStatementIncident(const clang::Stmt* stmt) {

        VarInFuncIncident* varInFuncIncident = dynamic_cast<VarInFuncIncident*>(this->incident);

        vector<const clang::Stmt*> incidentValues;
        int result = varInFuncIncident->hasIncidentExtend(stmt, incidentValues);

        if (result != -1) {

            if (to_string(varInFuncCounter).compare(varInFuncCount) != 0) {
                varInFuncCounter++;
                return;
            }
            varInFuncCounter++;

            bool _if = result == 1;
            Context *context = context->getInstance();
            vector<vector<string>> initialConstraintsList = context->getInitialConstraintsList();

            if (initialConstraintsList.size()) {
                SymbolTable *st = st->getInstance();
                map <string, pair<set<string>, string>> tableCopy = st->getTable();
                for (const clang::Stmt* statement : incidentValues) {
                    this->paths.clear();
                    for (vector<string> initialConstraints : initialConstraintsList) {
                        st->setTable(tableCopy);
                        vector<string> constraints = initialConstraints;
                        string constraint = this->cfgBlockHandler.getIfStmtCondition(statement, _if);
                        constraints.push_back(constraint);
                        this->paths.push_back(Path(constraints));
                    }
                }
            } else {
                for (const clang::Stmt* statement : incidentValues) {
                    this->paths.clear();
                    vector<string> constraints;
                    string constraint = this->cfgBlockHandler.getIfStmtCondition(statement, _if);
                    constraints.push_back(constraint);
                    this->paths.push_back(Path(constraints));
                }
            }
            this->writePaths();
        }
    }

    void bottomUpTraverse(const clang::CFGBlock* startBlock, unsigned int previousBlockId, vector<string>& constraints, bool previousCollect) {
        cout << "Visiting Block " << startBlock->getBlockID() << endl;
        if (startBlock->getBlockID() == this->getEntryBlockId()) {
            return this->paths.push_back(Path(constraints));
        }
        string terminatorCondition = "";
        if (previousBlockId != 0) {
            terminatorCondition = this->cfgBlockHandler.getTerminatorCondition(startBlock, previousBlockId, previousCollect);
            if (terminatorCondition.compare(""))
                constraints.push_back(terminatorCondition);
        }
        vector<vector<string>> constraintsList = this->cfgBlockHandler.getBlockCondition(startBlock, constraints);
        SymbolTable *st = st->getInstance();
        map<string, pair<set<string>, string>> tableCopy = st->getTable();
        for (clang::CFGBlock::const_pred_iterator I = startBlock->pred_begin(), E = startBlock->pred_end(); I != E; I++) {
            bool collect = (previousBlockId == 0) || (terminatorCondition.compare("") != 0) || (!(constraintsList.size() == 1 && constraintsList[0].size() == constraints.size()));
            for (vector<string> c: constraintsList) {
                vector<string> constraintsCopy = c;
                st->setTable(tableCopy);
                this->bottomUpTraverse((*I).getReachableBlock(), startBlock->getBlockID(), constraintsCopy, collect);
            }
        }
    }

    vector<vector<string>> readConstraintsList() {
        return this->fs.readFunctionConstraints();
    }

    void clearConstraintsList() {
        this->fs.clearMainConstraintsFile();
    }

    void collectConstraints() {
        if (this->incident->getType().compare("FUNCTION") == 0) {
            FunctionIncident* functionIncident = dynamic_cast<FunctionIncident*>(this->incident);
            this->fs.writeFunctionParameters(functionIncident->getParams());
            this->fs.writeFunctionParametersType(functionIncident->getParamTypes());
            return;
        }
        vector<vector<string>> initialConstraintsList;
        if (this->incident->getType().compare("RETURN") != 0) {
            initialConstraintsList = this->fs.readFunctionConstraints();
            this->fs.clearMainConstraintsFile();
        } else {
            this->fs.clearWriteConstarintsFile();
        }
        if (initialConstraintsList.size()) {
            SymbolTable *st = st->getInstance();
            map<string, pair<set<string>, string>>tableCopy = st->getTable();
            for (vector<string> initialConstraints : initialConstraintsList) {
                for (vector<const clang::CFGBlock*>::iterator blk = this->incidentBlocks.begin(); blk != this->incidentBlocks.end(); ++blk) {
                    st->setTable(tableCopy);
                    cout << "Incident Block: " << (*blk)->getBlockID() << endl;

                    this->paths.clear();
                    vector <string> pathConstraints = initialConstraints;

                    this->bottomUpTraverse((*blk), 0, pathConstraints, false);
                    this->writePaths();
                }
            }
        } else {
            SymbolTable *st = st->getInstance();
            map<string, pair<set<string>, string>>tableCopy = st->getTable();
            for (vector<const clang::CFGBlock *>::iterator blk = this->incidentBlocks.begin(); blk != this->incidentBlocks.end(); ++blk) {
                st->setTable(tableCopy);
                cout << "Incident Block: " << (*blk)->getBlockID() << endl;

                this->paths.clear();
                vector <string> pathConstraints;

                this->bottomUpTraverse((*blk), 0, pathConstraints, false);
                this->writePaths();
            }
        }
    }

    void writePaths() {
        cout << "Constrains:" << endl;
        this->removeDuplicatePaths();
        for (Path path: this->paths) {
            vector<string> constraints = path.getConstraints();
            if (incidentType.compare("RETURN") == 0)
                this->fs.writeReturnConstraints(constraints);
            else if (incidentType.compare("VARWRITE") == 0)
                this->fs.writeVarWriteConstraints(constraints);
            else
                this->fs.writeMainConstraints(constraints);

            cout << "Sub Path: ";
            for (string s: constraints)
                cout << s << " & ";
            cout << endl;
        }
    }
    void writeVarWritePaths() {
        vector<vector<string>> varWritePaths = this->fs.readVarWriteConstraints();
        this->fs.clearMainConstraintsFile();
        for (vector<string> constraints: varWritePaths)
            this->fs.writeMainConstraints(constraints);
        this->fs.clearVarWriteConstraintsFile();
    }

    unsigned int getEntryBlockId() {
        return (*(this->cfg))->getEntry().getBlockID();
    }

    vector<const clang::CFGBlock*> getIncidentBlocks() {
        return this->incidentBlocks;
    }

    vector<string> getReturnValues() {
        return this->returnValues;
    }

    void removeDuplicatePaths() {
        for (int i = 0; i < this->paths.size(); i++) {
            for (int j = i + 1; j < this->paths.size(); j++) {
                if (this->paths[i].compare(this->paths[j])) {
                    this->paths.erase(this->paths.begin() + j);
                    j--;
                }
            }
        }
    }

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

class VarWriteCallBack : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    VarWriteCallBack() {}

    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        Context *context = context->getInstance();
        context->setContext(Result);
        const auto *Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
        if (Function->isThisDeclarationADefinition() && Function->hasBody()) {
            const clang::Stmt *funcBody = Function->getBody();
            clang::SourceLocation SL = funcBody->getBeginLoc();
            const clang::SourceManager *SM = Result.SourceManager;
            if (SM->isInMainFile(SL)) {

                SymbolTable *se = se->getInstance();
                se->setFunctionName(Function->getNameInfo().getAsString());
                se->loadState();

                vector <string> params;
                for (clang::FunctionDecl::param_const_iterator I = Function->param_begin(), E = Function->param_end();
                     I != E; ++I) {
                    string name = (*I)->getNameAsString();
                    params.push_back(name);
                }
                se->setParams(params);
                se->setType("FUNCTION");

                const auto cfg = clang::CFG::buildCFG(Function, Function->getBody(), Result.Context,
                                                      clang::CFG::BuildOptions());

                CFGHandler cfgHandler(cfg);
                cfgHandler.findIncidents();
                cfgHandler.collectConstraints();
            }

        }
    }
};

class MyCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    MyCallback() {}
    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        Context *context = context->getInstance();
        context->setContext(Result);

        if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {

            CFGHandler cfgHandler(nullptr);
            if (!context->getConstraintsListSet()) {
                context->setInitialConstraintsList(cfgHandler.readConstraintsList());
                cfgHandler.clearConstraintsList();
            }

            SymbolTable *se = se->getInstance();
            se->setType("FUNCTION");

            const auto *If = Result.Nodes.getNodeAs<clang::IfStmt>("if");
            clang::SourceLocation SL = If->getBeginLoc();
            const clang::SourceManager *SM = Result.SourceManager;
            if (SM->isInMainFile(SL)) {
                CFGHandler cfgHandler(nullptr);
                if (incidentType.compare("VARINFFUNC") == 0)
                    cfgHandler.findStatementIncident(If);
                else if (incidentType.compare("VARINFUNCEXTEND") == 0)
                    cfgHandler.findChildStatementIncident(If);
                return;
            }
        } else {
            const auto* Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
            if (Function->isThisDeclarationADefinition()) {
                SymbolTable *se = se->getInstance();
//                se->loadState();
                vector <string> params;
                for (clang::FunctionDecl::param_const_iterator I = Function->param_begin(), E = Function->param_end();
                     I != E; ++I) {
                    string name = (*I)->getNameAsString();
                    params.push_back(name);
                }
                se->setParams(params);
                if (incidentType.compare("RETURN") == 0) {
                    se->loadParameters(functionName);
                    se->loadParameterTypes(functionName);
                    se->setType("RETURN");
                } else {
                    se->loadFunctionParameters();
                    se->loadFunctionParameterTypes();
                    if (root.compare("1"))
                        se->setType("FUNCTION");
                }

                const auto cfg = clang::CFG::buildCFG(Function,
                                                      Function->getBody(),
                                                      Result.Context,
                                                      clang::CFG::BuildOptions());
                CFGHandler cfgHandler(cfg);

                cfgHandler.findIncidents();
                cfgHandler.collectConstraints();
            }
        }
    }
};

class MyConsumer : public clang::ASTConsumer {
public:
    explicit MyConsumer() : handler(), varWriteCallBack(), cfgHandler(nullptr) {
        if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {
            const auto matching_node = clang::ast_matchers::ifStmt().bind("if");
            match_finder.addMatcher(matching_node, &handler);
        } else if (incidentType.compare("VARWRITE") == 0) {
            const auto matching_node = clang::ast_matchers::functionDecl().bind("fn");
            match_finder.addMatcher(matching_node, &varWriteCallBack);
        } else {
            const auto matching_node = clang::ast_matchers::functionDecl(
                    clang::ast_matchers::hasName(functionName)).bind("fn");
            match_finder.addMatcher(matching_node, &handler);
        }
    }

    void HandleTranslationUnit(clang::ASTContext& ctx) {
        match_finder.matchAST(ctx);
        if (incidentType.compare("VARWRITE") == 0)
            this->cfgHandler.writeVarWritePaths();
    }
private:
    MyCallback handler;
    VarWriteCallBack varWriteCallBack;
    CFGHandler cfgHandler;
    clang::ast_matchers::MatchFinder match_finder;
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr <clang::ASTConsumer>
    virtual CreateASTConsumer(clang::CompilerInstance& CI, llvm::StringRef) override {
        // Following two lines are added in order to not rising error while not finding #include location
        clang::Preprocessor &pp = CI.getPreprocessor();
        pp.SetSuppressIncludeNotFoundError(true);
        clang::DiagnosticsEngine& de = pp.getDiagnostics();
        de.setErrorLimit(1000);
        return std::make_unique<MyConsumer>();
    }
};

int executeAction(int argc, const char** argv) {

    auto CFGCategory = llvm::cl::OptionCategory("CFG");
    clang::tooling::CommonOptionsParser OptionsParser(argc, argv, CFGCategory);
    clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    return Tool.run(clang::tooling::newFrontendActionFactory<MyFrontendAction>().get());
}

SymbolTable *SymbolTable::instance = 0;
Context *Context::instance = 0;

int main(int argc, const char **argv) {

    filePath = *(&argv[1]);
    functionName = *(&argv[3]);
    incident = *(&argv[4]);
    incidentType = *(&argv[5]);
    root = *(&argv[6]);

    if (argc == 8) {
        varInFuncCount = *(&argv[7]);
    }

    Context* context = context->getInstance();
    SymbolTable* se = se->getInstance();

    se->loadState();
    se->setFunctionName(functionName);
    context->setCurrentFunction(functionName);

    executeAction(argc, argv);

    se->saveState();

    return 0;
}
