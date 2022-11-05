//
// Created by Mahan Tafreshipour on 3/29/21.
//

#include "StatementHandler.h"


string getStatementString(const clang::Stmt* stmt) {
  Context *context = context->getInstance();
  clang::SourceRange range = stmt->getSourceRange();
  const clang::SourceManager *SM = context->getContext()->SourceManager;
  llvm::StringRef ref = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), *SM,
                                                    clang::LangOptions());

  return ref.str();
}

string getFunctionFileName(const clang::FunctionDecl* functionDecl) {
  Context *context = context->getInstance();
  clang::SourceLocation sourceLocation = functionDecl->getBeginLoc();
  const clang::SourceManager *SM = context->getContext()->SourceManager;
  llvm::StringRef ref = SM->getFilename(sourceLocation);
  return ref.str();
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

bool hasFunctionCall(const clang::Stmt* stmt, string stmtClass, vector<string>& names, vector<vector<string>>& paramNames, vector<vector<string>>& paramTypes) {
//    cout << getStatementString(stmt) << " " << stmtClass << endl;
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

//        const clang::Stmt *callee = callExpr->getCallee();
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
//            cout << "VarDecl: " << varDecl->getNameAsString() << " has init: " << varDecl->hasInit() << endl;
            if (varDecl && varDecl->hasInit()) {
//                cout << "Has init:\n";
                const clang::Stmt* rhs = varDecl->getInit();
                return hasFunctionCall(rhs, rhs->getStmtClassName(), names, paramNames, paramTypes);
            }
        }
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

    if (thenStatement && hasReturn(thenStatement))
        return 1;
    if (elseStatement && hasReturn(elseStatement))
        return 0;
    return -1;
}

bool hasElse(const clang::IfStmt *ifStmt) {
    const clang::Stmt *elseStatement = ifStmt->getElse();
    if (elseStatement)
        return true;
    return false;
}

void getStmtOperands(const clang::Stmt* stmt, set<pair<string, string>>& operands, string type) {
    Context* context = context->getInstance();
    const string stmtClass(stmt->getStmtClassName());
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
            Incident* incident = context->getIncident();
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
            Incident* incident = context->getIncident();
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
                // FIXME: Do not check statement class here
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

                Incident* incident = context->getIncident();
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
    else if (stmtClass.compare("ReturnStmt") == 0 && context->getIncident()->getType().compare("RETURN") == 0) {
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