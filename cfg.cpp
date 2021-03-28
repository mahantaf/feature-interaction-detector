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

using namespace std;
using namespace llvm;

string filePath, functionName, incident, incidentType, root, varInFuncCount;

int varInFuncCounter = 0;
bool varInFuncLock = false;
bool varInFuncTypeLock = false;

class Context {
public:

    static Context *getInstance() {
        if (!instance)
            instance = new Context;
        return instance;
    }

    clang::ast_matchers::MatchFinder::MatchResult* getContext() {
        return this->context;
    }

    vector<vector<string>> getInitialConstraintsList() {
        return this->initialConstraintsList;
    }

    bool getConstraintsListSet() {
        return this->constraintsListSet;
    }

    string getCurrentFunction() {
        return this->currentFunction;
    }

    void setContext(clang::ast_matchers::MatchFinder::MatchResult context) {
        this->context = &context;
    }

    void setInitialConstraintsList(vector<vector<string>> initialConstraintsList) {
        this->initialConstraintsList = initialConstraintsList;
        this->constraintsListSet = true;
    }

    void setCurrentFunction(string currentFunction) {
        this->currentFunction = currentFunction;
    }

private:
    Context() { this->constraintsListSet = false; }
    static Context *instance;
    clang::ast_matchers::MatchFinder::MatchResult* context;
    vector<vector<string>> initialConstraintsList;
    bool constraintsListSet;
    string currentFunction;
};

string getStatementString(const clang::Stmt* stmt) {
    Context *context = context->getInstance();
    clang::SourceRange range = stmt->getSourceRange();
    const clang::SourceManager *SM = context->getContext()->SourceManager;
    llvm::StringRef ref = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), *SM,
                                                      clang::LangOptions());

    return ref.str();
}

class FileSystem {
public:
    FileSystem() {
        this->folder = "temp/";
        this->constraintFile = "constraints.txt";
        this->returnConstraintFile = "return_constraints.txt";
        this->varWriteConstraintFile = "var_write_constraints.txt";
        this->symbolTableFile = "symbols.txt";
        this->parametersFile = "_params.txt";
        this->parameterTypesFile = "_param_types.txt";
        this->functionParametersFile = "params.txt";
        this->functionParameterTypesFile = "param_types.txt";
    }

    void clearMainConstraintsFile() {
        std::ofstream ofs;
        ofs.open(this->constraintFile, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    void clearWriteConstarintsFile() {
        std::ofstream ofs;
        ofs.open(this->folder + this->returnConstraintFile, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    void clearVarWriteConstraintsFile() {
        std::ofstream ofs;
        ofs.open(this->folder + this->varWriteConstraintFile, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    map<string, pair<set<string>, string>> readSymbolTable() {
        string line;
        ifstream infile(this->folder + this->symbolTableFile);

        bool firstLine = true;
        bool lastLine = false;
        string symbol;
        set<string> symbols;
        map<string, pair<set<string>, string>> symbolTable;

        while (getline(infile, line)) {
            if (firstLine) {
                symbol = line;
                firstLine = false;
            } else if (line.compare("TYPE") == 0) {
                lastLine = true;
            } else if (line.compare("---------------") == 0) {
                firstLine = true;
            } else if (lastLine) {
                lastLine = false;
                pair<set<string>, string> p(symbols, line);
                symbolTable[symbol] = p;
                symbols.clear();
            } else {
                symbols.insert(line);
            }
        }
        return symbolTable;
    }

    vector<string> readParameterTypes(string functionName) {
        ifstream infile(this->folder + functionName + this->parameterTypesFile);
        string line;
        vector<string> paramTypes;

        while (getline(infile, line) && line.compare("---------------"))
            paramTypes.push_back(line);
        return paramTypes;
    }

    vector<string> readParameters(string functionName) {
        ifstream infile(this->folder + functionName + this->parametersFile);
        string line;
        vector<string> params;

        while (getline(infile, line) && line.compare("---------------"))
            params.push_back(line);
        return params;
    }

    vector<string> readFunctionParameterTypes() {
        ifstream infile(this->folder + this->functionParameterTypesFile);
        string line;
        vector<string> paramTypes;

        while (getline(infile, line) && line.compare("---------------"))
            paramTypes.push_back(line);
        return paramTypes;
    }

    vector<string> readFunctionParameters() {
        ifstream infile(this->folder + this->functionParametersFile);
        string line;
        vector<string> params;

        while (getline(infile, line) && line.compare("---------------"))
            params.push_back(line);
        return params;
    }

    vector<vector<string>> readFunctionConstraints() {
        cout << "In reading constraint from previous pipeline\n";
        ifstream infile(this->constraintFile);
        string line;
        vector<vector<string>> constraintsList;
        vector<string> constraints;

        while (getline(infile, line)) {
            cout << line << endl;
            if (line.compare("---------------") == 0) {
                constraintsList.push_back(constraints);
                constraints.clear();
            } else {
                constraints.push_back(line);
            }
        }
        return constraintsList;
    }

    vector<vector<string>> readVarWriteConstraints() {
        ifstream infile(this->folder + this->varWriteConstraintFile);
        string line;
        vector<vector<string>> constraintsList;
        vector<string> constraints;

        while (getline(infile, line)) {
            cout << line << endl;
            if (line.compare("---------------") == 0) {
                constraintsList.push_back(constraints);
                constraints.clear();
            } else {
                constraints.push_back(line);
            }
        }
        return constraintsList;
    }

    vector<vector<string>> readReturnConstraints(vector<string>& returnValues) {
        ifstream infile(this->folder + this->returnConstraintFile);
        string line;
        bool firstLine = true;
        vector<vector<string>> constraintsList;
        vector<string> constraints;

        while (getline(infile, line)) {
            if (firstLine) {
                returnValues.push_back(line);
                firstLine = false;
            } else if (line.compare("---------------") == 0) {
                constraintsList.push_back(constraints);
                constraints.clear();
                firstLine = true;
            } else {
                constraints.push_back(line);
            }
        }
        return constraintsList;
    }

    void writeSymbolTable(map<string, pair<set<string>, string>>& symbolTable) {
        ofstream symbolFile;
        symbolFile.open(this->folder + this->symbolTableFile, ofstream::out | ofstream::trunc);
        if (symbolFile.is_open()) {
            for (map<string, pair<set<string>, string>>::const_iterator it = symbolTable.begin(); it != symbolTable.end(); ++it) {
                symbolFile << it->first << endl;
                pair<set<string>, string> symbols = it->second;
                for (string s: symbols.first) {
                    symbolFile << s << endl;
                }
                symbolFile << "TYPE" << endl;
                symbolFile << symbols.second << endl;
                symbolFile << "---------------\n";

            }
            symbolFile.close();
        }
    }

    void writeParametersType(string functionName, vector<string> paramTypes) {
        string fileName = this->folder + functionName + this->parameterTypesFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(paramTypes, file);
    }

    void writeFunctionParametersType(vector<string> paramTypes) {
        string fileName = this->folder + this->functionParameterTypesFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(paramTypes, file);
    }

    void writeVarInFuncParametersType(vector<string> paramTypes) {
        string fileName = this->folder + this->functionParameterTypesFile;
        if (varInFuncTypeLock) {
            ofstream file (fileName, ios_base::app);
            return this->writeConstraints(paramTypes, file);
        } else {
            varInFuncTypeLock = true;
            ofstream file (fileName, ofstream::out | ofstream::trunc);
            return this->writeConstraints(paramTypes, file);
        }
    }

    void writeParameters(string functionName, vector<string> params) {
        string fileName = this->folder + functionName + this->parametersFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(params, file);
    }

    void writeFunctionParameters(vector<string> params) {
        string fileName = this->folder + this->functionParametersFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(params, file);
    }

    void writeVarInFuncParameters(vector<string> params) {
        string fileName = this->folder + this->functionParametersFile;
        if (varInFuncLock) {
            ofstream file (fileName, ios_base::app);
            return this->writeConstraints(params, file);
        } else {
            varInFuncLock = true;
            ofstream file (fileName, ofstream::out | ofstream::trunc);
            return this->writeConstraints(params, file);
        }
    }

    void writeMainConstraints(vector<string>& constraints) {
        ofstream file (this->constraintFile, ios_base::app);
        return this->writeConstraints(constraints, file);
    }

    void writeVarWriteConstraints(vector<string>& constraints) {
        ofstream file (this->folder + this->varWriteConstraintFile, ios_base::app);
        return this->writeConstraints(constraints, file);
    }

    void writeReturnConstraints(vector<string>& constraints) {
        ofstream file;
        file.open(this->folder + this->returnConstraintFile, ios_base::app);
        return this->writeConstraints(constraints, file);
    }

protected:
    void writeConstraints(vector<string>& constraints, ofstream& file) {
        if (file.is_open()) {
            for (string s: constraints)
                file << s << endl;
            file << "---------------\n";
            file.close();
        }
    }
private:
    string folder;
    string constraintFile;
    string returnConstraintFile;
    string varWriteConstraintFile;
    string symbolTableFile;
    string parametersFile;
    string parameterTypesFile;
    string functionParametersFile;
    string functionParameterTypesFile;
};

class SymbolTable {
public:
    static SymbolTable *getInstance() {
        if (!instance)
            instance = new SymbolTable;
        return instance;
    }

    string addVariableSymbol(string var, string type) {
        /**
         * Description
         * 1. If variable symbol has not been generated yet
         *      If the variable type is not LVALUE -> create new symbol
         * 2. If variable symbol has been generated yet
         *      If type is not LVALUE -> get its last symbol
         * 3. If variable symbol has been generated yet
         *      If type is LVALUE
         *          If last type is not LVALUE -> get its last symbol
         * 4. If variable symbol has been generated yet
         *      If type is LVALUE
         *          If last type is LVALUE -> create new symbol
         */
        string varSymbol = "";
        if ((this->type.compare("RETURN") == 0 || this->type.compare("FUNCTION") == 0)) {
            if (this->isInParams(var) != -1) {
                int paramIndex = this->isInParams(var);
                string passParam = this->passParams[paramIndex];
                string passParamType = this->passParamTypes[paramIndex];

                if (this->isLiteral(passParamType)) {
                    varSymbol = passParam;
                    setVariableSymbol(var, varSymbol);
                } else {
                    string copyType = this->type;
                    this->type = "";
                    varSymbol = addVariableSymbol(passParam, type);
                    setVariableSymbol(var, varSymbol);
                    this->type = copyType;
                }
            } else if (this->isMemberVariable(var)) {
                string copyType = this->type;
                this->type = "";
                varSymbol = addVariableSymbol(var, type);
                setVariableSymbol(var, varSymbol);
                this->type = copyType;
            } else {
                Context *context = context->getInstance();
                var = incidentType.compare("VARWRITE") == 0 ? var + "_" + context->getCurrentFunction() : var + "_" + functionName;
                if (this->getVariableSymbols(var).size() == 0) {
                    if (type.compare("LVALUE"))
                        varSymbol = this->insertNewSymbol(var);
                    if (type.compare("IVALUE") == 0)
                        this->insertNewSymbol(var);
                } else {
                    varSymbol = this->getVariableLastSymbol(var);
                    if (type.compare("LVALUE") == 0 || type.compare("IVALUE") == 0)
                        this->insertNewSymbol(var);
                }
            }
        } else {
            if (this->getVariableSymbols(var).size() == 0) {
                if (type.compare("LVALUE"))
                    varSymbol = this->insertNewSymbol(var);
                if (type.compare("IVALUE") == 0)
                    this->insertNewSymbol(var);
            } else {
                varSymbol = this->getVariableLastSymbol(var);
                if (type.compare("LVALUE") == 0 || type.compare("IVALUE") == 0)
                    this->insertNewSymbol(var);
            }
        }

        return varSymbol;
    }

    map<string, pair<set<string>, string>> getTable() {
        return this->symbolTable;
    }

    void setTable(map<string, pair<set<string>, string>> table) {
        this->symbolTable = table;
    }

    void setType(string type) {
        this->type = type;
    }

    void setParams(vector<string> params) {
        this->params = params;
    }

    void setPassParams(vector<string> passParams) {
        this->passParams = passParams;
    }

    void saveState() {
        this->fs.writeSymbolTable(this->symbolTable);
    }

    void saveParameters(string functionName, vector<string> parameters) {
        vector<string> passParameters;
        for (string p : parameters) {
            int paramIndex = this->isInParams(p);
            if (paramIndex != -1) {
                passParameters.push_back(this->passParams[paramIndex]);
            } else {
                passParameters.push_back(p);
            }
        }
        this->fs.writeParameters(functionName, passParameters);
    }

    void saveParameterTypes(string functionName, vector<string> parameters, vector<string> parameterTypes) {
        vector<string> passParameterTypes;
        for (vector<string>::iterator p = parameters.begin(); p != parameters.end(); ++p) {
            int paramIndex = this->isInParams(*p);
            if (paramIndex != -1) {
                passParameterTypes.push_back(this->passParamTypes[paramIndex]);
            } else {
                passParameterTypes.push_back(parameterTypes[p - parameters.begin()]);
            }
        }
        this->fs.writeParametersType(functionName, passParameterTypes);
    }

    void loadParameterTypes(string functionName) {
        this->passParamTypes = this->fs.readParameterTypes(functionName);
    }

    void loadParameters(string functionName) {
        this->passParams = this->fs.readParameters(functionName);
    }

    void loadFunctionParameterTypes() {
        this->passParamTypes = this->fs.readFunctionParameterTypes();
    }

    void loadFunctionParameters() {
        this->passParams = this->fs.readFunctionParameters();
    }

    void loadState() {
        this->symbolTable = this->fs.readSymbolTable();
    }

    void print() {
        for (map<string, pair<set<string>, string>>::const_iterator it = symbolTable.begin(); it != symbolTable.end(); ++it) {
            cout << it->first << ": (";
            pair<set<string>, string> symbols = it->second;
            for (string s: symbols.first) {
                cout << s << ' ';
            }
            cout << ") " << symbols.second << endl;
        }
    }

protected:
    string insertNewSymbol(string variable) {
//        if (this->type.compare("RETURN") == 0 || this->type.compare("FUNCTION") == 0) {
//            this->symbol = "ref";
//        }
        set<string> variableSymbols = this->getVariableSymbols(variable);
        string s = variable + "_" + this->symbol + to_string(variableSymbols.size());
        variableSymbols.insert(s);
        this->setVariableSymbols(variable, variableSymbols);
        this->symbol = "s";
        return s;
    }

    string getVariableLastSymbol(string variable) {
        set<string> variableSymbols = this->getVariableSymbols(variable);
        return *--variableSymbols.end();
    }

    set<string> getVariableSymbols(string variable) {
        return this->symbolTable[variable].first;
    }

    bool isMemberVariable(string variable) {
        if (variable.find("this->") != string::npos)
            return true;
        return false;
    }

    bool isLiteral(string variableType) {
        if (variableType.find("Literal") != string::npos)
            return true;
        return false;
    }

    int isInParams(string variable) {
        for (int i = 0; i < params.size(); ++i)
            if (params[i].compare(variable) == 0)
                return i;
        return -1;
    }

    void setVariableSymbol(string variable, string symbol) {
        set<string> variableSymbols = this->getVariableSymbols(variable);
        variableSymbols.insert(symbol);
        this->symbolTable[variable].first = variableSymbols;
    }

    void setVariableSymbols(string variable, set<string> symbols) {
        this->symbolTable[variable].first = symbols;
    }
private:
    SymbolTable() {
        this->fs = FileSystem();
        map<string, pair<set<string>, string>> st;
        symbol = "s";
        symbolTable = st;
    }
    FileSystem fs;
    string type;
    vector<string> params;
    vector<string> passParams;
    vector<string> passParamTypes;
    static SymbolTable *instance;
    string symbol;
    map<string, pair<set<string>, string>> symbolTable;
};

vector<string> split(string s, char delimiter) {
    vector <string> substrings;
    string word = "";
    for (auto x : s) {
        if (x == delimiter) {
            substrings.push_back(word);
            word = "";
        } else {
            word = word + x;
        }
    }
    substrings.push_back(word);
    return substrings;
}

set<pair<string, string>> findPairsWithSameFirst(set<pair<string, string>> vars) {
    set<pair<string, string>> duplicatePairs;
    for (set<pair<string, string>>::iterator i = vars.begin(); i != vars.end(); ++i) {
        pair<string, string> selector = *i;
        int num = 0;
        for (set<pair<string, string>>::iterator j = i; j != vars.end(); ++j) {
            num++;
            if (selector.first.compare((*j).first) == 0 && selector.second.compare((*j).second)) {
                pair<string, string> duplicate = *j;
                --j;
                duplicatePairs.insert(duplicate);
                vars.erase(duplicate);
            }
        }
    }
    return duplicatePairs;
}

class Incident {
public:
    Incident(string incident, string type) : incident(incident), type(type) {}

    virtual bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) = 0;

    virtual void print() = 0;

    string getType() {
        return this->type;
    }

protected:
    string incident;
    string type;
};

class CallIncident : public Incident {
public:
    CallIncident(string incident) : Incident(incident, "CALL") {}

    bool hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("CallExpr") == 0 || stmtClass.compare("CXXMemberCallExpr") == 0) {
            const clang::FunctionDecl *functionDecl = cast<clang::CallExpr>(stmt)->getDirectCallee();
            if (functionDecl) {
              string stmtString = functionDecl->getNameInfo().getAsString();
              if (stmtString.compare(this->incident) == 0)
                return true;
            }
        }
        return false;
    }

    void print() {
        cout << "Call Incident" << endl;
    }
};

class WriteIncident: public Incident {
public:
    WriteIncident(string incident) : Incident(incident, "WRITE") {}

    bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("BinaryOperator") == 0) {
//            cout << getStatementString(stmt) << endl;
            const clang::BinaryOperator* binaryOperator = cast<clang::BinaryOperator>(stmt);
            if (binaryOperator->isAssignmentOp()) {
                const clang::Stmt* lhs = binaryOperator->getLHS();

                string stmtString;
                string lhsStmtClass(lhs->getStmtClassName());

                if (lhsStmtClass.compare("MemberExpr") == 0) {
                    stmtString = cast<clang::MemberExpr>(lhs)->getMemberNameInfo().getAsString();
                } else {
                    stmtString = getStatementString(lhs);
                }

                if (stmtString.compare(this->incident) == 0)
                    return true;
            }
        }
        return false;
    }

    void print() {
        cout << "Write Incident" << endl;
    }
};

class ReturnIncident: public Incident {
public:
    ReturnIncident(string incident) : Incident(incident, "RETURN") {}

    bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("ReturnStmt") == 0) {
            return true;
        }
        return false;
    }

    void print() {
        cout << "Return Incident" << endl;
    }

private:

};

class FunctionIncident : public Incident {
public:
    FunctionIncident(string incident) : Incident(incident, "FUNCTION") {}

    vector<string> getParams() {
        return this->params;
    }

    vector<string> getParamTypes() {
        return this->paramTypes;
    }

    void setParamsAndParamTypes(const clang::Stmt* stmt) {

        const clang::CallExpr* callExpr = cast<clang::CallExpr>(stmt);
        const clang::Expr* const* args = callExpr->getArgs();

        for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i) {
            this->params.push_back(getStatementString(args[i]));
            this->paramTypes.push_back(args[i]->getStmtClassName());
        }
    }

    bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("CallExpr") == 0 || stmtClass.compare("CXXMemberCallExpr") == 0) {

            const clang::CallExpr* callExpr = cast<clang::CallExpr>(stmt);
            const clang::FunctionDecl *functionDecl = cast<clang::CallExpr>(stmt)->getDirectCallee();

            if (functionDecl) {
                string stmtString = functionDecl->getNameInfo().getAsString();
                if (stmtString.compare(this->incident) == 0) {
                    const clang::Expr* const* args = callExpr->getArgs();

                    for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i) {
                        this->params.push_back(getStatementString(args[i]));
                        this->paramTypes.push_back(args[i]->getStmtClassName());
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void print() {

    }

private:
    vector<string> params;
    vector<string> paramTypes;
};

class VarInFuncIncident: public Incident {
public:
    VarInFuncIncident(string incident) : Incident(incident, "VARINFFUNC") {}

    vector<string> getParams() {
        return this->params;
    }

    vector<string> getParamTypes() {
        return this->paramTypes;
    }

    bool hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues) {
        return true;
    }

    int hasIncidentExtend(const clang::Stmt *stmt, vector<const clang::Stmt*> &incidentValues) {
        const clang::IfStmt *ifStmt = cast<clang::IfStmt>(stmt);

        set<string> variables;
        const clang::Stmt* conditionStatement = ifStmt->getCond();
        this->getConditionVariables(conditionStatement, variables);

        if (hasVariable(variables)) {
            const clang::Stmt* thenStatement = ifStmt->getThen();
            const clang::Stmt* elseStatement = ifStmt->getElse();

            if (thenStatement && this->hasCall(thenStatement)) {
                incidentValues.push_back(conditionStatement);
                return 1;
            }
            if (elseStatement && this->hasCall(elseStatement)) {
                incidentValues.push_back(conditionStatement);
                return 0;
            }
        }
        return -1;
    };

    bool hasCall(const clang::Stmt* stmt) {
        CallIncident callIncident(this->incident);
        vector <string> incidentValues;
        for (clang::ConstStmtIterator it = stmt->child_begin(); it != stmt->child_end(); it++) {
            const clang::Stmt* child = (*it);
            if (callIncident.hasIncident(child, incidentValues)) {
                FunctionIncident functionIncident(this->incident);
                functionIncident.setParamsAndParamTypes(child);
                this->params = functionIncident.getParams();
                this->paramTypes = functionIncident.getParamTypes();
                return true;
            }
        }
        if (callIncident.hasIncident(stmt, incidentValues)) {
            FunctionIncident functionIncident(this->incident);
            functionIncident.setParamsAndParamTypes(stmt);
            this->params = functionIncident.getParams();
            this->paramTypes = functionIncident.getParamTypes();
            return true;
        }
        return false;
    }

    bool hasVariable(set<string>& variables) {
        for (string variable: variables)
            if (variable.compare(functionName) == 0)
                return true;
        return false;
    }

    void getConditionVariables(const clang::Stmt* stmt, set<string>& variables) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("ParenExpr") == 0) {

            const clang::ParenExpr *parenExpr = cast<clang::ParenExpr>(stmt);
            const clang::Stmt *subParen = parenExpr->getSubExpr();
            getConditionVariables(subParen, variables);

        } else if (stmtClass.compare("BinaryOperator") == 0) {

            const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
            const clang::Stmt *lhs = binaryOperator->getLHS();
            const clang::Stmt *rhs = binaryOperator->getRHS();
            getConditionVariables(lhs, variables);
            getConditionVariables(rhs, variables);

        } else if (stmtClass.compare("UnaryOperator") == 0) {

            const clang::Stmt *subExpr = cast<clang::UnaryOperator>(stmt)->getSubExpr();
            getConditionVariables(subExpr, variables);

        } else if (stmtClass.compare("ImplicitCastExpr") == 0) {

            const clang::Stmt *subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
            getConditionVariables(subExpr, variables);

        } else if (stmtClass.compare("MemberExpr") == 0) {

            variables.insert(cast<clang::MemberExpr>(stmt)->getMemberNameInfo().getAsString());

        } else if (stmtClass.compare("DeclRefExpr") == 0) {
            variables.insert(getStatementString(stmt));
        }
    }

    void print() {
        cout << "VarInFunc Incident" << endl;
    }
private:
    vector<string> params;
    vector<string> paramTypes;
};

class VarWriteIncident: public Incident {
public:
    VarWriteIncident(string incident) : Incident(incident, "VARWRITE") {}

    bool hasIncident(const clang::Stmt *stmt, vector<string> &incidentValues) {
        const string stmtClass(stmt->getStmtClassName());
        string lhsVar = "";
        set<string> variables;

        if (stmtClass.compare("BinaryOperator") == 0 || stmtClass.compare("CompoundAssignOperator") == 0) {
            const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
            const clang::Stmt *lhs = binaryOperator->getLHS();
            const clang::Stmt *rhs = binaryOperator->getRHS();

            lhsVar = this->getBinaryOperatorLHSVariable(lhs);
            this->getBinaryOperatorRHSVariables(rhs, variables);

        } else if (stmtClass.compare("DeclStmt") == 0) {
            lhsVar = this->getDeclStatementLHSVariable(stmt);
            this->getDeclStatementRHSVariables(stmt, variables);
        }
        return lhsVar.compare(incident) == 0 && hasVariable(functionName, variables);
    }

    bool hasIncidentExtend(const clang::Stmt* stmt) {
        const string stmtClass(stmt->getStmtClassName());
        string lhsVar = "";
        set<string> variables;

        if (stmtClass.compare("BinaryOperator") == 0 || stmtClass.compare("CompoundAssignOperator") == 0) {
            const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
            const clang::Stmt *lhs = binaryOperator->getLHS();
            const clang::Stmt *rhs = binaryOperator->getRHS();

            lhsVar = this->getBinaryOperatorLHSVariable(lhs);
            this->getBinaryOperatorRHSVariables(rhs, variables);

        } else if (stmtClass.compare("DeclStmt") == 0) {
            lhsVar = this->getDeclStatementLHSVariable(stmt);
            this->getDeclStatementRHSVariables(stmt, variables);
        }
        return lhsVar.compare(incident) == 0 && hasVariable(functionName, variables);
    }

    bool hasVariable(string variable, set<string>& variables) {
        for (string var : variables)
            if (var.compare(variable) == 0)
                return true;
        return false;
    }

    void getBinaryOperatorRHSVariables(const clang::Stmt* stmt, set<string>& variables) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("ParenExpr") == 0) {
            const clang::ParenExpr *parenExpr = cast<clang::ParenExpr>(stmt);
            const clang::Stmt *subParen = parenExpr->getSubExpr();
            getBinaryOperatorRHSVariables(subParen, variables);

        } else if (stmtClass.compare("BinaryOperator") == 0) {

            const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
            const clang::Stmt *lhs = binaryOperator->getLHS();
            const clang::Stmt *rhs = binaryOperator->getRHS();
            getBinaryOperatorRHSVariables(lhs, variables);
            getBinaryOperatorRHSVariables(rhs, variables);

        } else if (stmtClass.compare("UnaryOperator") == 0) {

            const clang::Stmt *subExpr = cast<clang::UnaryOperator>(stmt)->getSubExpr();
            getBinaryOperatorRHSVariables(subExpr, variables);

        } else if (stmtClass.compare("ImplicitCastExpr") == 0) {

            const clang::Stmt *subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
            getBinaryOperatorRHSVariables(subExpr, variables);

        } else if (stmtClass.compare("MemberExpr") == 0) {

            variables.insert(cast<clang::MemberExpr>(stmt)->getMemberNameInfo().getAsString());

        } else if (stmtClass.compare("DeclRefExpr") == 0) {
            variables.insert(getStatementString(stmt));
        }
    }

    void getDeclStatementRHSVariables(const clang::Stmt* stmt, set<string>& variables) {
        const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
        const clang::Decl* declaration = declStmt->getSingleDecl();
        if (declaration) {
            const clang::VarDecl* varDecl = cast<clang::VarDecl>(declaration);
            if (varDecl && varDecl->hasInit()) {
                const clang::Stmt* rhs = varDecl->getInit();
                getBinaryOperatorRHSVariables(rhs, variables);
            }
        }
    }

    string getBinaryOperatorLHSVariable(const clang::Stmt* stmt) {
        const string stmtClass(stmt->getStmtClassName());
        if (stmtClass.compare("ImplicitCastExpr") == 0) {
            const clang::Stmt *subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
            return getBinaryOperatorLHSVariable(subExpr);
        } else if (stmtClass.compare("MemberExpr") == 0) {
            return cast<clang::MemberExpr>(stmt)->getMemberNameInfo().getAsString();
        } else if (stmtClass.compare("DeclRefExpr") == 0) {
            return getStatementString(stmt);
        }
    }

    string getDeclStatementLHSVariable(const clang::Stmt* stmt) {
        const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
        const clang::Decl* declaration = declStmt->getSingleDecl();
        if (declaration) {
            const clang::VarDecl *varDecl = cast<clang::VarDecl>(declaration);
            if (varDecl && varDecl->hasInit()) {
                const clang::Stmt *rhs = varDecl->getInit();
                return varDecl->getNameAsString();
            }
        }
    }

    void print() {
        cout << "VarWrite Incident" << endl;
    }

};

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
                vector<string> splitDecl = split(declType, ' ');
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

class Transpiler {
public:
    Transpiler() {}

    void replaceVariables(string& stmt, set<pair<string, string>>& vars) {
        for (pair<string, string> p: vars) {
            regex e(p.first + "((?=\\W)|$)");
            stmt = regex_replace(stmt, e, p.second);
        }
        for (pair<string, string> p: vars) {
            regex em("->");
            stmt = regex_replace(stmt, em, "_");
        }
    }
    void replaceDeclaration(string& statement) {
        regex e("^\\S*\\s");
        statement = regex_replace(statement, e, "");
        regex sem(";");
        statement = regex_replace(statement, sem, "");
    }

    void replaceReturnStatement(string& statement) {
        regex e("return\\s");
        statement = regex_replace(statement, e, "");
    }

    string replaceFunctionCallByReturnValue(string& statement, string& functionName, string& returnValue) {
        regex e("(this->)*" + functionName + "\\((.*)\\)");
        return regex_replace(statement, e, returnValue);
    }

    void replaceCompoundAssignment(string& statement) {
        string lhs = "";
        string rhs = "";
        string op = "";
        bool seen = false;

        for (auto x : statement) {
            if (seen) {
                rhs += x;
            } else if (x == '+' || x == '-' || x == '*' || x == '/') {
                op += x;
            } else if (x == '=') {
                seen = true;
            } else {
                lhs += x;
            }
        }
        statement = lhs + "= " + lhs + op + rhs;
    }

    void replaceStaticSingleAssignment(
            string& statement,
            set<pair<string, string>>& operands,
            set<pair<string, string>>& duplicate
    ) {

        vector<string> splitStatement = split(statement, '=');
        string lhs = splitStatement[0], rhs = splitStatement[1];

        this->replaceVariables(lhs, operands);
        this->replaceVariables(rhs, duplicate);

        statement = lhs + "=" + rhs;
    }

    void replaceStatement(string& statement, string& statementClass, set<pair<string, string>>& operands) {
        if (statementClass.compare("DeclStmt") == 0)
            this->replaceDeclaration(statement);

        if (statementClass.compare("ReturnStmt") == 0)
            this->replaceReturnStatement(statement);

        if (statementClass.compare("CompoundAssignOperator") == 0)
            this->replaceCompoundAssignment(statement);

        if (operands.size()) {
            set<pair<string, string>> duplicate = findPairsWithSameFirst(operands);
            if (duplicate.size())
                return this->replaceStaticSingleAssignment(statement, operands, duplicate);
            return this->replaceVariables(statement, operands);
        }
    }

    string replaceFunction(
            string& statement,
            string& statementClass,
            string& functionName,
            string& returnValue,
            set<pair<string, string>>& operands) {

        if (statementClass.compare("DeclStmt") == 0)
            this->replaceDeclaration(statement);

        if (statementClass.compare("CompoundAssignOperator") == 0)
            this->replaceCompoundAssignment(statement);

        string replacedStatement = this->replaceFunctionCallByReturnValue(statement, functionName, returnValue);

        if (operands.size()) {
            set<pair<string, string>> duplicate = findPairsWithSameFirst(operands);
            if (duplicate.size())
                this->replaceStaticSingleAssignment(replacedStatement, operands, duplicate);
            else
                this->replaceVariables(replacedStatement, operands);
        }
        return replacedStatement;
    }
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
            this->fs.writeVarInFuncParameters(varInFuncIncident->getParams());
            this->fs.writeVarInFuncParametersType(varInFuncIncident->getParamTypes());
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

class VarWriteHandler {
public:
    VarWriteHandler() {
        this->incident = createIncident();
    };

    bool findIncidents(const clang::Stmt* stmt) {
        const string stmtClass(stmt->getStmtClassName());
        VarWriteIncident* varWriteIncident = dynamic_cast<VarWriteIncident*>(this->incident);
        return varWriteIncident->hasIncidentExtend(stmt);
    }
private:
    Incident* incident;
};

class DeclCallBack: public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    DeclCallBack() {}
    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        Context *context = context->getInstance();
        context->setContext(Result);

        const auto* declStmt = Result.Nodes.getNodeAs<clang::Stmt>("declaration");
        clang::SourceLocation SL = declStmt->getBeginLoc();
        const clang::SourceManager *SM = Result.SourceManager;
        if (SM->isInMainFile(SL)) {
            VarWriteHandler* varWriteHandler = new VarWriteHandler();
            bool hasIncident = varWriteHandler->findIncidents(declStmt);
            if (hasIncident) {
                cout << getStatementString(declStmt);
            }
        }
    }
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

                string functionName = Function->getNameInfo().getAsString();
                context->setCurrentFunction(functionName);
                cout << "------------" << functionName << "------------" << endl;

                SymbolTable *se = se->getInstance();
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
        } else if(incidentType.compare("VARWRITE") == 0) {
            const auto* binaryStmt = Result.Nodes.getNodeAs<clang::Stmt>("binary");

            clang::SourceLocation SL = binaryStmt->getBeginLoc();
            const clang::SourceManager *SM = Result.SourceManager;
            if (SM->isInMainFile(SL)) {
                VarWriteHandler* varWriteHandler = new VarWriteHandler();
                varWriteHandler->findIncidents(binaryStmt);
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
    explicit MyConsumer() : handler(), declHandler(), varWriteCallBack(), cfgHandler(nullptr) {
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
    DeclCallBack declHandler;
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

    SymbolTable* se = se->getInstance();
    se->loadState();
    executeAction(argc, argv);
    se->saveState();

    return 0;
}
