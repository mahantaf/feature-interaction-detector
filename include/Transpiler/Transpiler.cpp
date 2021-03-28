//
// Created by mahan on 3/28/21.
//

#include "Transpiler.h"

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

vector<string> Transpiler::split(string s, char delimiter) {
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

void Transpiler::replaceVariables(string& stmt, set<pair<string, string>>& vars) {
    for (pair<string, string> p: vars) {
        regex e(p.first + "((?=\\W)|$)");
        stmt = regex_replace(stmt, e, p.second);
    }
    for (pair<string, string> p: vars) {
        regex em("->");
        stmt = regex_replace(stmt, em, "_");
    }
}

void Transpiler::replaceDeclaration(string& statement) {
    regex e("^\\S*\\s");
    statement = regex_replace(statement, e, "");
    regex sem(";");
    statement = regex_replace(statement, sem, "");
}

void Transpiler::replaceReturnStatement(string& statement) {
    regex e("return\\s");
    statement = regex_replace(statement, e, "");
}

string Transpiler::replaceFunctionCallByReturnValue(string& statement, string& functionName, string& returnValue) {
    regex e("(this->)*" + functionName + "\\((.*)\\)");
    return regex_replace(statement, e, returnValue);
}

void Transpiler::replaceCompoundAssignment(string& statement) {
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

void Transpiler::replaceStaticSingleAssignment(
        string& statement,
        set<pair<string, string>>& operands,
        set<pair<string, string>>& duplicate
) {

    vector<string> splitStatement = this->split(statement, '=');
    string lhs = splitStatement[0], rhs = splitStatement[1];

    this->replaceVariables(lhs, operands);
    this->replaceVariables(rhs, duplicate);

    statement = lhs + "=" + rhs;
}

void Transpiler::replaceStatement(string& statement, string& statementClass, set<pair<string, string>>& operands) {
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

string Transpiler::replaceFunction(
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
