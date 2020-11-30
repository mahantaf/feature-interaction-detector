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

using namespace std;

string functionName, incident, incidentType;

class StringHandler {
public:
  StringHandler(string str) : str(str) {};

  string replace(string rgx, string substitute) {
    regex e(rgx);
    return regex_replace(str, e, substitute);
  }

  vector<string> split(char delimiter) {
    vector <string> substrings;
    string word = "";
    for (auto x : str) {
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

private:
  string str;
};

class SymbolTable {
public:
  static SymbolTable *getInstance() {
    if (!instance)
      instance = new SymbolTable;
    return instance;
  }

  string addVariableSymbol(string var, string type) {

    string varSymbol = "";

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

  void print() {
    map<string, pair<set<string>, string>> st;
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
    set<string> variableSymbols = this->getVariableSymbols(variable);
    string s = this->symbol + to_string(variableSymbols.size()) + "_" + variable;
    variableSymbols.insert(s);
    this->setVariableSymbols(variable, variableSymbols);
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
  void setVariableType(string variable, string type) {
    this->symbolTable[variable].second = type;
  }
  void setVariableSymbols(string variable, set<string> symbols) {
    this->symbolTable[variable].first = symbols;
  }
private:
  SymbolTable() {
    map<string, pair<set<string>, string>> st;
    symbol = "s";
    symbolTable = st;
  }
  static SymbolTable *instance;
  string symbol;
  map<string, pair<set<string>, string>> symbolTable;
};

string removeSpaces(string str) {
  str.erase(remove(str.begin(), str.end(), ' '), str.end());
  str.erase(remove(str.begin(), str.end(), '\n'), str.end());
  return str;
}

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

string GetSourceLevelStmtString(const clang::Stmt* stmt, clang::ast_matchers::MatchFinder::MatchResult Result) {

  clang::SourceRange range = stmt->getSourceRange();
  const clang::SourceManager *SM = Result.SourceManager;
  llvm::StringRef ref = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), *SM,
                                                    clang::LangOptions());

  return ref.str();
}

int isInBlock(const clang::CFGBlock* block, unsigned int previousBlockId) {
  int i = 0;
  for (clang::CFGBlock::const_succ_iterator I = block->succ_begin(), E = block->succ_end(); I != E; I++) {
    if (((*I).getReachableBlock()->getBlockID() == previousBlockId) && i == 0)
      return 0;
    if (((*I).getReachableBlock()->getBlockID() == previousBlockId) && i == 1)
      return 1;
    i++;
  }
  return -1;
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

void replaceVariablesBySymbols(string& stmt, set<pair<string, string>>& vars) {
  for (pair<string, string> p: vars) {
    regex e(p.first + "((?=\\W)|$)");
    stmt = regex_replace(stmt, e, p.second);
  }
}

string addVariableSymbol(string var, string type, map<string, pair<set<string>, string>>& symbolTable) {
  set<string> varSymbols = symbolTable[var].first;
  string varSymbol;

  if (varSymbols.size() == 0) {
    if (type.compare("LVALUE")) {
      varSymbol = "s" + to_string(varSymbols.size()) + "_" + var;
      varSymbols.insert(varSymbol);
    } else {
      varSymbol = "";
    }
  } else {
    if (type.compare("LVALUE"))
      varSymbol = *--varSymbols.end();
    else {
      if (symbolTable[var].second.compare("LVALUE")) {
        varSymbol = *--varSymbols.end();
      } else {
        varSymbol = "s" + to_string(varSymbols.size()) + "_" + var;
        varSymbols.insert(varSymbol);
      }
    }
  }
  if (symbolTable[var].second.compare("LVALUE"))
    symbolTable[var].second = type;
  symbolTable[var].first = varSymbols;
  return varSymbol;
}

void getStmtOperands(
    const clang::Stmt* stmt,
    set<pair<string, string>>& operands,
    clang::ast_matchers::MatchFinder::MatchResult Result,
    map<string, pair<set<string>, string>>& symbolTable,
    string type
) {

  const string stmtClass(stmt->getStmtClassName());
  if (stmtClass.compare("BinaryOperator") == 0) {

    const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
    const clang::Stmt *lhs = binaryOperator->getLHS();
    const clang::Stmt *rhs = binaryOperator->getRHS();

    if (binaryOperator->isAssignmentOp()) {
      getStmtOperands(rhs, operands, Result, symbolTable, "RVALUE");
      getStmtOperands(lhs, operands, Result, symbolTable, "LVALUE");
    } else {

      getStmtOperands(rhs, operands, Result, symbolTable, type);
      getStmtOperands(lhs, operands, Result, symbolTable, type);
    }
  } else if (stmtClass.compare("ImplicitCastExpr") == 0 || stmtClass.compare("DeclRefExpr") == 0) {

    if (type.compare("")) {
      string var = GetSourceLevelStmtString(stmt, Result);

      SymbolTable *st = st->getInstance();
      string varSymbol = st->addVariableSymbol(var, type);

//      string varSymbol = addVariableSymbol(var, type, symbolTable);
//      cout << "Type: " << type << " Var: " << var << " VarSymbol: " << varSymbol << endl;
      if (varSymbol.compare("")) {
        pair <string, string> op(var, varSymbol);
        operands.insert(op);
      }
    }

  } else if (stmtClass.compare("ParenExpr") == 0) {
      const clang::ParenExpr *parenExpr = cast<clang::ParenExpr>(stmt);
      const clang::Stmt *subParen = parenExpr->getSubExpr();
      getStmtOperands(subParen, operands, Result, symbolTable, type);
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
//        string varSymbol = addVariableSymbol(var, "LVALUE", symbolTable);
        if (varSymbol.compare("")) {
          pair <string, string> op(var, varSymbol);
          operands.insert(op);
        }
        getStmtOperands(rhs, operands, Result, symbolTable, "RVALUE");
      }
    }
  }
}

const string GetTerminatorCondition(
    const clang::CFGBlock* cfgBlock,
    unsigned int previousBlockId,
    clang::ast_matchers::MatchFinder::MatchResult Result,
    map<string, pair<set<string>, string>>& symbolTable
    ) {
  const clang::Stmt *terminatorStmt = cfgBlock->getTerminatorStmt();
  if (terminatorStmt) {
    if (terminatorStmt->getStmtClass() == clang::Stmt::IfStmtClass) {

      set<pair<string, string>> operands;
      getStmtOperands(cfgBlock->getTerminatorCondition(), operands, Result, symbolTable, "COND");

      string stmt;
      if (isInBlock(cfgBlock, previousBlockId) == 0)
        stmt = GetSourceLevelStmtString(cfgBlock->getTerminatorCondition(), Result);
      else
        stmt = "!(" + GetSourceLevelStmtString(cfgBlock->getTerminatorCondition(), Result) + ")";

      replaceVariablesBySymbols(stmt, operands);
      return stmt;
    }
  }
  return "";
}

bool hasFunctionCall(
    const clang::Stmt* stmt,
    string stmtClass,
    vector<string>& names,
    clang::ast_matchers::MatchFinder::MatchResult Result
    ) {
  if (stmtClass.compare("BinaryOperator") == 0) {
    const clang::BinaryOperator *binaryOperator = cast<clang::BinaryOperator>(stmt);
    const clang::Stmt *lhs = binaryOperator->getLHS();
    const clang::Stmt *rhs = binaryOperator->getRHS();
    return hasFunctionCall(lhs, lhs->getStmtClassName(), names, Result) ||
           hasFunctionCall(rhs, rhs->getStmtClassName(), names, Result);
  } else if (stmtClass.compare("CallExpr") == 0) {
    const clang::Stmt *callee = cast<clang::CallExpr>(stmt)->getCallee();
    names.push_back(GetSourceLevelStmtString(callee, Result));
    return true;
  }
  return false;
}

vector<vector<string>> addFunctionConstraints(
    vector<string>& returnValues
    ) {
  string line;
  ifstream infile("return_constraints.txt");
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

vector<vector<string>> GetBlockCondition(
    const clang::CFGBlock* block,
    vector<string>& constraints,
    clang::ast_matchers::MatchFinder::MatchResult Result,
    map<string, pair<set<string>, string>>& symbolTable
) {
  vector<vector<string>> constraintsList;
  constraintsList.push_back(constraints);

  for (clang::CFGBlock::const_reverse_iterator I = block->rbegin(), E = block->rend(); I != E ; ++I) {
    clang::CFGElement El = *I;
    if (auto SE = El.getAs<clang::CFGStmt>()) {
      const clang::Stmt *stmt = SE->getStmt();
      const string stmtClass(stmt->getStmtClassName());


      set<pair<string, string>> operands;
      getStmtOperands(stmt, operands, Result, symbolTable, "");

      string statement = GetSourceLevelStmtString(stmt, Result);
      pair<string, string> statementClassPair(statement, stmtClass);

      vector<string> funcNames;
      if (hasFunctionCall(stmt, stmtClass, funcNames, Result) && stmtClass.compare("CallExpr")) {
        cout << "Function names: " << endl;
        for (string name: funcNames) {
          string sysCall = "./cfg test.cpp -- " + name + " c RETURN";
          system(sysCall.c_str());
          vector<string> returnValues;
          vector<vector<string>> functionConstraintsList = addFunctionConstraints(returnValues);

          int index = 0;
          for (string returnValue: returnValues) {
            if (stmtClass.compare("DeclStmt") == 0) {
              regex e("^\\S*");
              statement = regex_replace(statement, e, "");
            }
            regex e(name + "\\((.*)\\)");
            string ts = regex_replace(statement, e, returnValue);
            replaceVariablesBySymbols(ts, operands);
            functionConstraintsList[index].insert(functionConstraintsList[index].begin(), ts);
            index++;
          }

          vector<vector<string>> newConstraintsList;
          for (vector<string> c: constraintsList) {
            for (vector<string> fc: functionConstraintsList) {
              vector<string> newConstraints = c;
              newConstraints.insert(newConstraints.end(), fc.begin(), fc.end());
              newConstraintsList.push_back(newConstraints);
            }
          }
          constraintsList = newConstraintsList;
        }
      } else {
        if (stmtClass.compare("DeclStmt") == 0) {
          regex e("^\\S*");
          statement = regex_replace(statement, e, "");
        }
        if (operands.size()) {
          set<pair<string, string>> duplicate = findPairsWithSameFirst(operands);
          if (duplicate.size()) {

            vector<string> splitStatement = split(statement, '=');

            string lhs = splitStatement[0];
            string rhs = splitStatement[1];

            replaceVariablesBySymbols(lhs, duplicate);
            replaceVariablesBySymbols(rhs, operands);

            for (int i = 0; i < constraintsList.size(); i++) {
              constraintsList[i].push_back(lhs + "=" + rhs);
            }

          } else {
            replaceVariablesBySymbols(statement, operands);
            for (int i = 0; i < constraintsList.size(); i++) {
              constraintsList[i].push_back(statement);
            }
          }
        }
      }
    }
  }
  return constraintsList;
}

vector<string> copyVector(vector<string>& v) {
  vector<string> v2;
  for (int i = 0; i < v.size(); i++)
    v2.push_back(v[i]);
  return v2;
}

void BottomUpCFGTraverse(
    const clang::CFGBlock* startBlock,
    unsigned int beginBlockId,
    unsigned int previousBlockId,
    clang::ast_matchers::MatchFinder::MatchResult Result,
    vector<string>& constraints,
    vector<vector<string>>& paths,
    map<string, pair<set<string>, string>>& symbolTable
    ) {
  cout << "Visiting block " << startBlock->getBlockID() << endl;
  if (startBlock->getBlockID() == beginBlockId) {
    return paths.push_back(constraints);
  }
  if (previousBlockId != 0) {
    const string terminatorCondition = GetTerminatorCondition(startBlock, previousBlockId, Result, symbolTable);
    if (terminatorCondition.compare(""))
      constraints.push_back(terminatorCondition);
  }
  vector<vector<string>> constraintsList = GetBlockCondition(startBlock, constraints, Result, symbolTable);

  SymbolTable *st = st->getInstance();
  map<string, pair<set<string>, string>>tableCopy = st->getTable();
  for (clang::CFGBlock::const_pred_iterator I = startBlock->pred_begin(), E = startBlock->pred_end(); I != E; I++) {
    for (vector<string> c: constraintsList) {
      vector<string> constraintsCopy = copyVector(c);
      map<string, pair<set<string>, string>>symbolTableCopy = symbolTable;
      st->setTable(tableCopy);
      BottomUpCFGTraverse((*I).getReachableBlock(), beginBlockId, startBlock->getBlockID(), Result, constraintsCopy, paths, symbolTableCopy);
    }
  }
}

void WriteConstraintsToFile(vector<string>& constraints) {
  string fileName = incidentType.compare("RETURN") ? "constraints.txt" : "return_constraints.txt";
  ofstream constraintsFile (fileName, ios_base::app);
  if (constraintsFile.is_open()) {
    for (string s: constraints)
      constraintsFile << s << endl;
    constraintsFile << "---------------\n";
    constraintsFile.close();
  }
}

class MyCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  MyCallback() {}
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
    cout << "In callback run\n";
    const auto* Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
    const auto CFG = clang::CFG::buildCFG(Function,
                                        Function->getBody(),
                                        Result.Context,
                                        clang::CFG::BuildOptions());


    vector<const clang::CFGBlock*> incidentBlocks;
    vector<string> returnValues;

    for (const auto* blk : *CFG) {
      for (clang::CFGBlock::const_iterator I = blk->begin(), E = blk->end(); I != E ; ++I) {
        clang::CFGElement El = *I;
        if (auto SE = El.getAs<clang::CFGStmt>()) {
          const clang::Stmt *stmt = SE->getStmt();
          const string stmtClass(stmt->getStmtClassName());
          cout << GetSourceLevelStmtString(stmt, Result) << " stmt type: " << stmtClass << endl;
          if (incidentType.compare("CALL") == 0 && stmtClass.compare("CallExpr") == 0) {
            const clang::Stmt *callee = cast<clang::CallExpr>(stmt)->getCallee();
            string stmtString = GetSourceLevelStmtString(callee, Result);
            if (stmtString.compare(incident) == 0)
              incidentBlocks.push_back(blk);
          } else if (incidentType.compare("WRITE") == 0 && stmtClass.compare("BinaryOperator") == 0) {
            const clang::BinaryOperator* binaryOperator = cast<clang::BinaryOperator>(stmt);
            if (binaryOperator->isAssignmentOp()) {
              const clang::Stmt* lhs = binaryOperator->getLHS();
              string stmtString = GetSourceLevelStmtString(lhs, Result);
              if (stmtString.compare(incident) == 0)
                incidentBlocks.push_back(blk);
            }
          } else if (incidentType.compare("RETURN") == 0 && stmtClass.compare("ReturnStmt") == 0) {
            const clang::ReturnStmt* returnStmt = cast<clang::ReturnStmt>(stmt);
            const clang::Stmt* returnValue = returnStmt->getRetValue();
            returnValues.push_back(GetSourceLevelStmtString(returnValue, Result));
            incidentBlocks.push_back(blk);
          }
        }
      }
      blk->dump();
    }
    int i = 0;
    for (vector<const clang::CFGBlock*>::iterator blk = incidentBlocks.begin(); blk != incidentBlocks.end(); ++blk) {
      cout << "Incident block: " << (*blk)->getBlockID() << endl;

      vector<string> c;
      vector<vector<string>> paths;
      map<string, pair<set<string>, string>> symbolTable;

      if (incidentType.compare("RETURN") == 0)
        c.push_back(returnValues[i]);

      BottomUpCFGTraverse((*blk), CFG->getEntry().getBlockID(), 0, Result, c, paths, symbolTable);

      cout << "Constrains:" << endl;
      for (vector<string> constraints: paths) {
        WriteConstraintsToFile(constraints);
        cout << "Sub Path: ";
        for (string s: constraints)
          cout << s << " & ";
        cout << endl;
      }
      i++;
    }
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

SymbolTable *SymbolTable::instance = 0;

int main(int argc, const char **argv) {

  functionName = *(&argv[3]);
  incident = *(&argv[4]);
  incidentType = *(&argv[5]);

  auto CFGCategory = llvm::cl::OptionCategory("CFG");
  clang::tooling::CommonOptionsParser OptionsParser(argc, argv, CFGCategory);
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

  return Tool.run(clang::tooling::newFrontendActionFactory<MyFrontendAction>().get());
}
