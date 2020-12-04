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

using namespace std;

string functionName, incident, incidentType;

//class StringExtra {
//public:
//  StringHandler(string str) : str(str) {};
//
//  string replace(string rgx, string substitute) {
//    regex e(rgx);
//    return regex_replace(str, e, substitute);
//  }
//
//  vector<string> split(char delimiter) {
//    vector <string> substrings;
//    string word = "";
//    for (auto x : str) {
//      if (x == delimiter) {
//        substrings.push_back(word);
//        word = "";
//      } else {
//        word = word + x;
//      }
//    }
//    substrings.push_back(word);
//    return substrings;
//  }
//
//private:
//  string str;
//};

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

  void setContext(clang::ast_matchers::MatchFinder::MatchResult context) {
    this->context = &context;
  }
private:
  Context() {}
  static Context *instance;
  clang::ast_matchers::MatchFinder::MatchResult* context;
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
    this->constraintFile = "constraints.txt";
    this->returnConstraintFile = "return_constraints.txt";
    this->symbolTableFile = "symbols.txt";
    this->parametersFile = "_params.txt";
  }

  map<string, pair<set<string>, string>> readSymbolTable() {
    string line;
    ifstream infile(this->symbolTableFile);

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

  vector<string> readParameters(string functionName) {
    ifstream infile(functionName + this->parametersFile);
    string line;
    vector<string> params;

    while (getline(infile, line) && line.compare("---------------"))
      params.push_back(line);
    return params;
  }

  vector<vector<string>> readReturnConstraints(vector<string>& returnValues) {
    ifstream infile(this->returnConstraintFile);
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
    symbolFile.open(this->symbolTableFile, ofstream::out | ofstream::trunc);
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

  void writeParameters(string functionName, vector<string> params) {
    string fileName = functionName + this->parametersFile;
    ofstream file (fileName, ofstream::out | ofstream::trunc);
    return this->writeConstraints(params, file);
  }

  void writeMainConstraints(vector<string>& constraints) {
    ofstream file (this->constraintFile, ios_base::app);
    return this->writeConstraints(constraints, file);
  }

  void writeReturnConstraints(vector<string>& constraints) {
    ofstream file;
    file.open(this->returnConstraintFile, ios_base::app);
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
  string constraintFile;
  string returnConstraintFile;
  string symbolTableFile;
  string parametersFile;
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
    if (this->type.compare("RETURN") == 0) {
      cout << "Creating symbol for variable" << endl;
      int paramIndex = this->isInParams(var);
      if (paramIndex != -1) {
        string passParam = this->passParams[paramIndex];
        cout << "Found that this variable is a passed variable: " << passParam << endl;
        this->type = "";
        varSymbol = addVariableSymbol(passParam, type);
        setVariableSymbol(var, varSymbol);
        this->type = "RETURN";
      }
    } else {
      if (this->getVariableSymbols(var).size() == 0) {
        if (type.compare("LVALUE"))
          varSymbol = this->insertNewSymbol(var);
      } else {
        if (type.compare("LVALUE"))
          varSymbol = this->getVariableLastSymbol(var);
        else {
          if (this->getVariableType(var).compare("LVALUE"))
            varSymbol = this->getVariableLastSymbol(var);
          else
            varSymbol = this->insertNewSymbol(var);
        }
      }
    }

    if (this->getVariableType(var).compare("LVALUE"))
      this->setVariableType(var, type);

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

  void loadParameters(string functionName) {
    this->passParams = this->fs.readParameters(functionName);
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
    if (this->type.compare("RETURN") == 0) {
      this->symbol = "ref";
    }
    set<string> variableSymbols = this->getVariableSymbols(variable);
    string s = this->symbol + to_string(variableSymbols.size()) + "_" + variable;
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

  string getVariableType(string variable) {
    return this->symbolTable[variable].second;
  }

  int isInParams(string variable) {
    for (int i = 0; i < params.size(); ++i)
      if (params[i].compare(variable) == 0)
        return i;
    return -1;
  }

  void setVariableType(string variable, string type) {
    this->symbolTable[variable].second = type;
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

set<pair<string, string>> findPairsWithSameFirst(set<pair<string, string>>& vars) {
  set<pair<string, string>> duplicatePairs;
  for (set<pair<string, string>>::iterator i = vars.begin(); i != vars.end(); ++i) {
    pair<string, string> selector = *i;
    for (set<pair<string, string>>::iterator j = i; j != vars.end(); ++j) {
      if (selector.first.compare((*j).first) == 0 && selector.second.compare((*j).second)) {
        duplicatePairs.insert(*j);
        vars.erase(j);
      }
    }
  }
  return duplicatePairs;
}

void getStmtOperands(const clang::Stmt* stmt, set<pair<string, string>>& operands, string type) {

  const string stmtClass(stmt->getStmtClassName());
  if (stmtClass.compare("BinaryOperator") == 0) {

    const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
    const clang::Stmt *lhs = binaryOperator->getLHS();
    const clang::Stmt *rhs = binaryOperator->getRHS();

    if (binaryOperator->isAssignmentOp()) {
      getStmtOperands(rhs, operands, "RVALUE");
      getStmtOperands(lhs, operands, "LVALUE");
    } else {

      getStmtOperands(rhs, operands, type);
      getStmtOperands(lhs, operands, type);
    }
  } else if (stmtClass.compare("ImplicitCastExpr") == 0 || stmtClass.compare("DeclRefExpr") == 0) {

    if (type.compare("")) {
      string var = getStatementString(stmt);

      SymbolTable *st = st->getInstance();
      string varSymbol = st->addVariableSymbol(var, type);

      if (varSymbol.compare("")) {
        pair <string, string> op(var, varSymbol);
        operands.insert(op);
      }
    }

  } else if (stmtClass.compare("ParenExpr") == 0) {
      const clang::ParenExpr *parenExpr = cast<clang::ParenExpr>(stmt);
      const clang::Stmt *subParen = parenExpr->getSubExpr();
      getStmtOperands(subParen, operands, type);
  } else if (stmtClass.compare("DeclStmt") == 0) {
    const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
    const clang::Decl* declaration = declStmt->getSingleDecl();
    if (declaration) {
      const clang::VarDecl* varDecl = cast<clang::VarDecl>(declaration);
      if (varDecl && varDecl->hasInit()) {
        const clang::Stmt* rhs = varDecl->getInit();
        string var = varDecl->getNameAsString();
        SymbolTable *st = st->getInstance();
        string varSymbol = st->addVariableSymbol(var, "LVALUE");
        if (varSymbol.compare("")) {
          pair <string, string> op(var, varSymbol);
          operands.insert(op);
        }
        getStmtOperands(rhs, operands, "RVALUE");
      }
    }
  }
}

bool hasFunctionCall(
    const clang::Stmt* stmt, string stmtClass,
    vector<string>& names,
    vector<vector<string>>& paramNames
) {
  if (stmtClass.compare("BinaryOperator") == 0) {
    const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
    const clang::Stmt *lhs = binaryOperator->getLHS();
    const clang::Stmt *rhs = binaryOperator->getRHS();
    return hasFunctionCall(lhs, lhs->getStmtClassName(), names, paramNames) ||
           hasFunctionCall(rhs, rhs->getStmtClassName(), names, paramNames);
  } else if (stmtClass.compare("CallExpr") == 0) {
    const clang::CallExpr* callExpr = cast<clang::CallExpr>(stmt);

    const clang::Stmt *callee = callExpr->getCallee();
    const clang::Expr* const* args = callExpr->getArgs();

    vector<string> params;
    for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i)
      params.push_back(getStatementString(args[i]));
    paramNames.push_back(params);
    names.push_back(getStatementString(callee));
    return true;
  }
  return false;
}

class Incident {
public:
  Incident(string incident, string type) : incident(incident), type(type) {}

  virtual bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) = 0;

  virtual void print() = 0;

protected:
  string incident;
  string type;
};

class CallIncident : public Incident {
public:
  CallIncident(string incident) : Incident(incident, "CALL") {}

  bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
    const string stmtClass(stmt->getStmtClassName());
    if (stmtClass.compare("CallExpr") == 0) {
      const clang::Stmt *callee = cast<clang::CallExpr>(stmt)->getCallee();
      string stmtString = getStatementString(callee);
      if (stmtString.compare(this->incident) == 0)
        return true;
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
      const clang::BinaryOperator* binaryOperator = cast<clang::BinaryOperator>(stmt);
      if (binaryOperator->isAssignmentOp()) {
        const clang::Stmt* lhs = binaryOperator->getLHS();
        string stmtString = getStatementString(lhs);
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
      const clang::ReturnStmt* returnStmt = cast<clang::ReturnStmt>(stmt);
      const clang::Stmt* returnValue = returnStmt->getRetValue();
      incidentValues.push_back(getStatementString(returnValue));
      return true;
    }
    return false;
  }

  void print() {
    cout << "Return Incident" << endl;
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
  return nullptr;
}

class Path {
public:
  Path(vector<string> constraints) {
    this->constraints = constraints;
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
  }
  void replaceDeclaration(string& statement) {
    regex e("^\\S*");
    statement = regex_replace(statement, e, "");
  }

  string replaceFunctionCallByReturnValue(string& statement, string& functionName, string& returnValue) {
    regex e(functionName + "\\((.*)\\)");
    return regex_replace(statement, e, returnValue);
  }

  void replaceStaticSingleAssignment(
      string& statement,
      set<pair<string, string>>& operands,
      set<pair<string, string>>& duplicate
      ) {

    vector<string> splitStatement = split(statement, '=');
    string lhs = splitStatement[0], rhs = splitStatement[1];

    this->replaceVariables(lhs, duplicate);
    this->replaceVariables(rhs, operands);

    statement = lhs + "=" + rhs;
  }

  void replaceStatement(string& statement, string& statementClass, set<pair<string, string>>& operands) {
    if (statementClass.compare("DeclStmt") == 0)
      this->replaceDeclaration(statement);

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

    string replacedStatement = this->replaceFunctionCallByReturnValue(statement, functionName, returnValue);

    this->replaceVariables(replacedStatement, operands);
    return replacedStatement;
  }
};

class CFGBlockHandler {
public:
  CFGBlockHandler() {
    this->fs = FileSystem();
    this->transpiler = Transpiler();
  }

  bool isInIf(const clang::CFGBlock* block, unsigned int previousBlockId) {
    int i = 0;
    for (clang::CFGBlock::const_succ_iterator I = block->succ_begin(), E = block->succ_end(); I != E; I++) {
      if (((*I).getReachableBlock()->getBlockID() == previousBlockId) && i == 0)
        return true;
      return false;
      i++;
    }
    return false;
  }

  vector<vector<string>> getBlockCondition(const clang::CFGBlock* block, vector<string>& constraints) {

    this->initializeConstraints(constraints);

    for (clang::CFGBlock::const_reverse_iterator I = block->rbegin(), E = block->rend(); I != E ; ++I) {
      clang::CFGElement El = *I;
      if (auto SE = El.getAs<clang::CFGStmt>()) {
        const clang::Stmt *stmt = SE->getStmt();
        string stmtClass(stmt->getStmtClassName());

        set<pair<string, string>> operands;
        getStmtOperands(stmt, operands, "");

        string statement = getStatementString(stmt);

        vector<string> funcNames;
        vector<vector<string>> paramNames;

        if (stmtClass.compare("CallExpr") && hasFunctionCall(stmt, stmtClass, funcNames, paramNames)) {
          int i = 0;
          for (string name: funcNames) {

            SymbolTable* se = se->getInstance();
            se->saveState();
            se->saveParameters(name, paramNames[i]);

            string sysCall = "./cfg test.cpp -- " + name + " c RETURN";
            system(sysCall.c_str());

            se->loadState();

            vector<string> returnValues;
            vector<vector<string>> functionConstraintsList = this->fs.readReturnConstraints(returnValues);

            int index = 0;
            for (string returnValue: returnValues) {
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
        }
      }
    }
    return constraintsList;
  }

  const string getTerminatorCondition(const clang::CFGBlock* block, unsigned int previousBlockId) {
    const clang::Stmt *terminatorStmt = block->getTerminatorStmt();
    if (terminatorStmt && terminatorStmt->getStmtClass() == clang::Stmt::IfStmtClass) {

      set<pair<string, string>> operands;
      getStmtOperands(block->getTerminatorCondition(), operands, "COND");

      string stmt;
      if (this->isInIf(block, previousBlockId))
        stmt = getStatementString(block->getTerminatorCondition());
      else
        stmt = "!(" + getStatementString(block->getTerminatorCondition()) + ")";

      this->transpiler.replaceVariables(stmt, operands);
      return stmt;
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
    this->cfg = &cfg;
  }

  void findIncidents() {
    for (const auto* blk : **(this->cfg)) {
      for (clang::CFGBlock::const_iterator I = blk->begin(), E = blk->end(); I != E; ++I) {
        clang::CFGElement El = *I;
        if (auto SE = El.getAs<clang::CFGStmt>()) {
          const clang::Stmt *stmt = SE->getStmt();
          if (this->incident->hasIncident(stmt, this->returnValues)) {
            this->incidentBlocks.push_back(blk);
          }
        }
      }
      blk->dump();
    }
  }

  void bottomUpTraverse(const clang::CFGBlock* startBlock, unsigned int previousBlockId, vector<string>& constraints) {
    cout << "Visiting Block " << startBlock->getBlockID() << endl;
    if (startBlock->getBlockID() == this->getEntryBlockId()) {
      return this->paths.push_back(Path(constraints));
    }
    if (previousBlockId != 0) {
      const string terminatorCondition = this->cfgBlockHandler.getTerminatorCondition(startBlock, previousBlockId);
      if (terminatorCondition.compare(""))
        constraints.push_back(terminatorCondition);
    }
    vector<vector<string>> constraintsList = this->cfgBlockHandler.getBlockCondition(startBlock, constraints);

    SymbolTable *st = st->getInstance();
    map<string, pair<set<string>, string>>tableCopy = st->getTable();
    for (clang::CFGBlock::const_pred_iterator I = startBlock->pred_begin(), E = startBlock->pred_end(); I != E; I++) {
      for (vector<string> c: constraintsList) {
        vector<string> constraintsCopy = c;
        st->setTable(tableCopy);
        this->bottomUpTraverse((*I).getReachableBlock(), startBlock->getBlockID(), constraintsCopy);
      }
    }
  }

  void collectConstraints() {
    for (vector<const clang::CFGBlock*>::iterator blk = this->incidentBlocks.begin(); blk != this->incidentBlocks.end(); ++blk) {

      cout << "Incident Block: " << (*blk)->getBlockID() << endl;

      this->paths.clear();
      vector<string> pathConstraints;

      if (incidentType.compare("RETURN") == 0)
        pathConstraints.push_back(this->returnValues[blk - this->incidentBlocks.begin()]);

      this->bottomUpTraverse((*blk), 0, pathConstraints);
      this->writePaths();
    }
  }

  void writePaths() {
    cout << "Constrains:" << endl;
    for (Path path: this->paths) {
      vector<string> constraints = path.getConstraints();
      if (incidentType.compare("RETURN"))
        this->fs.writeMainConstraints(constraints);
      else
        this->fs.writeReturnConstraints(constraints);
      cout << "Sub Path: ";
      for (string s: constraints)
        cout << s << " & ";
      cout << endl;
    }
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

private:
  const unique_ptr<clang::CFG>* cfg;
  FileSystem fs;
  CFGBlockHandler cfgBlockHandler;
  vector<Path> paths;
  Incident* incident;
  vector<string> returnValues;
  vector<const clang::CFGBlock*> incidentBlocks;
};

class MyCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  MyCallback() {}
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
    Context *context = context->getInstance();
    context->setContext(Result);

    const auto* Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");


    if (incidentType.compare("RETURN") == 0) {
      SymbolTable* se = se->getInstance();
      se->loadState();
      se->loadParameters(functionName);
      vector<string> params;
      for (clang::FunctionDecl::param_const_iterator I = Function->param_begin(), E = Function->param_end(); I != E; ++I) {
        string name = (*I)->getNameAsString();
        params.push_back(name);
      }
      se->setType("RETURN");
      se->setParams(params);
    }

    const auto cfg = clang::CFG::buildCFG(Function,
                                        Function->getBody(),
                                        Result.Context,
                                        clang::CFG::BuildOptions());

    CFGHandler cfgHandler(cfg);

    cfgHandler.findIncidents();
    cfgHandler.collectConstraints();
  }
};

class MyConsumer : public clang::ASTConsumer {
public:
  explicit MyConsumer() : handler() {
//    const auto matching_node = clang::ast_matchers::functionDecl(clang::ast_matchers::isExpansionInMainFile()).bind("fn");
    const auto matching_node = clang::ast_matchers::functionDecl(clang::ast_matchers::hasName(functionName)).bind("fn");

    match_finder.addMatcher(matching_node, &handler);
  }

  void HandleTranslationUnit(clang::ASTContext& ctx) {
    match_finder.matchAST(ctx);
  }
private:
  MyCallback handler;
  clang::ast_matchers::MatchFinder match_finder;
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:

  std::unique_ptr <clang::ASTConsumer>
  virtual CreateASTConsumer(clang::CompilerInstance &, llvm::StringRef) override {
    return std::make_unique<MyConsumer>();
  }
};

int executeAction(int argc, const char **argv) {

  auto CFGCategory = llvm::cl::OptionCategory("CFG");
  clang::tooling::CommonOptionsParser OptionsParser(argc, argv, CFGCategory);
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

  return Tool.run(clang::tooling::newFrontendActionFactory<MyFrontendAction>().get());
}

SymbolTable *SymbolTable::instance = 0;
Context *Context::instance = 0;

int main(int argc, const char **argv) {

  functionName = *(&argv[3]);
  incident = *(&argv[4]);
  incidentType = *(&argv[5]);

  executeAction(argc, argv);

  SymbolTable* se = se->getInstance();
  se->saveState();

  return 0;
}
