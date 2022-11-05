//
// Created by mahan on 3/31/21.
//

#include "CFGBlockHandler.h"

CFGBlockHandler::CFGBlockHandler() {
    this->fs = FileSystem();
    this->transpiler = Transpiler();
}

vector<vector<string>> CFGBlockHandler::getBlockCondition(const clang::CFGBlock* block, vector<string>& constraints) {
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
//                    cout << "Function name: " << name << endl;
                    Context* context = context->getInstance();
                    SymbolTable* se = se->getInstance();
                    se->saveState();
                    se->saveParameters(name, paramNames[i]);
                    se->saveParameterTypes(name, paramNames[i], paramTypes[i]);

                    string sysCall = "./cfg " + context->getFilePath() + " -- " + name + " c RETURN 0";
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

string CFGBlockHandler::getIfStmtCondition(const clang::Stmt* statement, bool _if) {
    set<pair<string, string>> operands;

    getStmtOperands(statement, operands, "COND");

    string stmt = getStatementString(statement);
    this->transpiler.replaceVariables(stmt, operands);

    return _if ? (stmt) : ("!(" + stmt + ")");
}

string CFGBlockHandler::getTerminatorBlockCondition(const clang::CFGBlock* block, unsigned int previousBlockId, bool collect) {
    const clang::Stmt *terminatorStmt = block->getTerminatorStmt();
    if (terminatorStmt && terminatorStmt->getStmtClass() == clang::Stmt::IfStmtClass) {
        const clang::IfStmt *ifStmt = cast<clang::IfStmt>(terminatorStmt);
        int hasReturn = hasReturnStmt(ifStmt);
        if (hasReturn != -1 || collect) {
            if (!(hasReturn != -1) && !isInIf(block, previousBlockId) && !hasElse(ifStmt))
                return "";
            set<pair<string, string>> operands;
            getStmtOperands(block->getTerminatorCondition(), operands, "COND");

            string stmt;
            if (collect) {
                if (isInIf(block, previousBlockId))
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

void CFGBlockHandler::initializeConstraints(vector<string>& constraints) {
    this->constraintsList.clear();
    this->constraintsList.push_back(constraints);
}

void CFGBlockHandler::addStatement(string& statement) {
    for (long unsigned int i = 0; i < this->constraintsList.size(); i++)
        this->constraintsList[i].push_back(statement);
}

void CFGBlockHandler::addFunction(vector<vector<string>>& functionConstraintsList) {
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