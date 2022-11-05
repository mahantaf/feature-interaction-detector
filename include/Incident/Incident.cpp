//
// Created by Mahan Tafreshipour on 3/29/21.
//

#include "Incident.h"
#include "../Context/Context.h"

Incident::Incident(string incident, string type) : incident(incident), type(type) {}

string Incident::getType() {
  return this->type;
}

// -------------------CallIncident-------------------
CallIncident::CallIncident(string incident) : Incident(incident, "CALL") {}

bool CallIncident::hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues) {
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

void CallIncident::print() {
  cout << "Call Incident" << endl;
}

// -------------------WriteIncident-------------------
WriteIncident::WriteIncident(string incident) : Incident(incident, "WRITE") {}

bool WriteIncident::hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
  const string stmtClass(stmt->getStmtClassName());
  if (stmtClass.compare("BinaryOperator") == 0) {
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

void WriteIncident::print() {
  cout << "Write Incident" << endl;
}

// -------------------ReturnIncident-------------------
ReturnIncident::ReturnIncident(string incident) : Incident(incident, "RETURN") {}

bool ReturnIncident::hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
  const string stmtClass(stmt->getStmtClassName());
  if (stmtClass.compare("ReturnStmt") == 0) {
    return true;
  }
  return false;
}

void ReturnIncident::print() {
  cout << "Return Incident" << endl;
}

// -------------------FunctionIncident-------------------
FunctionIncident::FunctionIncident(string incident) : Incident(incident, "FUNCTION") {}

vector<string> FunctionIncident::getParams() {
  return this->params;
}

vector<string> FunctionIncident::getParamTypes() {
  return this->paramTypes;
}

string FunctionIncident::getFilePath() {
  return this->filePath;
}

void FunctionIncident::setParamsAndParamTypes(const clang::Stmt* stmt) {

  const clang::CallExpr* callExpr = cast<clang::CallExpr>(stmt);
  const clang::FunctionDecl *functionDecl = cast<clang::CallExpr>(stmt)->getDirectCallee();

  if (functionDecl)
    this->filePath = getFunctionFileName(functionDecl);

  const clang::Expr* const* args = callExpr->getArgs();
  for (unsigned int i = 0; i < callExpr->getNumArgs(); ++i) {
    this->params.push_back(getStatementString(args[i]));
    this->paramTypes.push_back(args[i]->getStmtClassName());
  }

}

bool FunctionIncident::hasIncident(const clang::Stmt* stmt, vector<string>& incidentValues) {
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
        this->filePath = getFunctionFileName(functionDecl);
        return true;
      }
    }
  }
  return false;
}

void FunctionIncident::print() {

}

// -------------------VarInFuncIncident-------------------
VarInFuncIncident::VarInFuncIncident(string incident) : Incident(incident, "VARINFFUNC") {}

vector<string> VarInFuncIncident::getParams() {
  return this->params;
}

vector<string> VarInFuncIncident::getParamTypes() {
  return this->paramTypes;
}

string VarInFuncIncident::getFilePath() {
  return this->filePath;
}

bool VarInFuncIncident::hasIncident(const clang::Stmt *stmt, vector <string> &incidentValues) {
  return true;
}

int VarInFuncIncident::hasIncidentExtend(const clang::Stmt *stmt, vector<const clang::Stmt*> &incidentValues) {
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
}

bool VarInFuncIncident::hasCall(const clang::Stmt* stmt) {
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
    this->filePath = functionIncident.getFilePath();
    return true;
  }
  return false;
}

bool VarInFuncIncident::hasVariable(set<string>& variables) {
  Context* context = context->getInstance();
  string functionName = context->getCurrentFunction();
  for (string variable: variables)
    if (variable.compare(functionName) == 0)
      return true;
  return false;
}

void VarInFuncIncident::getConditionVariables(const clang::Stmt* stmt, set<string>& variables) {
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

void VarInFuncIncident::print() {
  cout << "VarInFunc Incident" << endl;
}

// -------------------VarWriteIncident-------------------
VarWriteIncident::VarWriteIncident(string incident) : Incident(incident, "VARWRITE") {}

bool VarWriteIncident::hasIncident(const clang::Stmt *stmt, vector<string> &incidentValues) {
  Context* context = context->getInstance();
  const string stmtClass(stmt->getStmtClassName());
  string lhsVar = "";
  string functionName = context->getCurrentFunction();
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
  return lhsVar.compare(this->incident) == 0 && hasVariable(functionName, variables);
}

bool VarWriteIncident::hasIncidentExtend(const clang::Stmt* stmt) {
  Context* context = context->getInstance();
  const string stmtClass(stmt->getStmtClassName());
  string functionName = context->getCurrentFunction();
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

bool VarWriteIncident::hasVariable(string variable, set<string>& variables) {
  for (string var : variables)
    if (var.compare(variable) == 0)
      return true;
  return false;
}

void VarWriteIncident::getBinaryOperatorRHSVariables(const clang::Stmt* stmt, set<string>& variables) {
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

void VarWriteIncident::getDeclStatementRHSVariables(const clang::Stmt* stmt, set<string>& variables) {
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

string VarWriteIncident::getBinaryOperatorLHSVariable(const clang::Stmt* stmt) {
  const string stmtClass(stmt->getStmtClassName());
  if (stmtClass.compare("ImplicitCastExpr") == 0) {
    const clang::Stmt *subExpr = cast<clang::ImplicitCastExpr>(stmt)->getSubExpr();
    return getBinaryOperatorLHSVariable(subExpr);
  } else if (stmtClass.compare("MemberExpr") == 0) {
    return cast<clang::MemberExpr>(stmt)->getMemberNameInfo().getAsString();
  } else if (stmtClass.compare("DeclRefExpr") == 0) {
    return getStatementString(stmt);
  }
  return "";
}

string VarWriteIncident::getDeclStatementLHSVariable(const clang::Stmt* stmt) {
  const clang::DeclStmt *declStmt = cast<clang::DeclStmt>(stmt);
  const clang::Decl* declaration = declStmt->getSingleDecl();
  if (declaration) {
    const clang::VarDecl *varDecl = cast<clang::VarDecl>(declaration);
    if (varDecl && varDecl->hasInit())
      return varDecl->getNameAsString();
    return "";
  }
  return "";
}

void VarWriteIncident::print() {
  cout << "VarWrite Incident" << endl;
}
