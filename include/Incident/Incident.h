//
// Created by Mahan Tafreshipour on 3/29/21.
//

#ifndef INCIDENT_H
#define INCIDENT_H

#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>

#include <llvm/Support/Casting.h>

#include <set>
#include <string>
#include <vector>
#include <iostream>

#include "../Context/Context.h"
#include "../StatementHandler/StatementHandler.h"

using namespace std;
using namespace llvm;

class Incident {
public:
  Incident(string incident, string type);

  virtual bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) = 0;

  virtual void print() = 0;

  string getType();

protected:
  string incident;
  string type;
};

class CallIncident : public Incident {
public:
  CallIncident(string incident);

  bool hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues);

  void print();
};

class WriteIncident: public Incident {
public:
  WriteIncident(string incident);

  bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues);

  void print();
};

class ReturnIncident: public Incident {
public:
  ReturnIncident(string incident);

  bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues);

  void print();

private:

};

class FunctionIncident : public Incident {
public:
  FunctionIncident(string incident);

  vector<string> getParams();

  vector<string> getParamTypes();

  void setParamsAndParamTypes(const clang::Stmt* stmt);

  bool hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues);

  void print();

private:
  vector<string> params;
  vector<string> paramTypes;
};

class VarInFuncIncident: public Incident {
public:
  VarInFuncIncident(string incident);

  vector<string> getParams();

  vector<string> getParamTypes();

  bool hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues);

  int hasIncidentExtend(const clang::Stmt *stmt, vector<const clang::Stmt*> &incidentValues);

  bool hasCall(const clang::Stmt* stmt);

  bool hasVariable(set<string>& variables);

  void getConditionVariables(const clang::Stmt* stmt, set<string>& variables);

  void print();
private:
  vector<string> params;
  vector<string> paramTypes;
};

class VarWriteIncident: public Incident {
public:
  VarWriteIncident(string incident);

  bool hasIncident(const clang::Stmt *stmt, vector<string> &incidentValues);

  bool hasIncidentExtend(const clang::Stmt* stmt);

  bool hasVariable(string variable, set<string>& variables);

  void getBinaryOperatorRHSVariables(const clang::Stmt* stmt, set<string>& variables);

  void getDeclStatementRHSVariables(const clang::Stmt* stmt, set<string>& variables);

  string getBinaryOperatorLHSVariable(const clang::Stmt* stmt);

  string getDeclStatementLHSVariable(const clang::Stmt* stmt);

  void print();
};


#endif //INCIDENT_H
