//
// Created by mahan on 3/28/21.
//

#ifndef TRANSPILER_H
#define TRANSPILER_H

#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <set>

using namespace std;

class Transpiler {
public:
    Transpiler() {}

    static vector<string> split(string s, char delimiter);

    void replaceVariables(string& stmt, set<pair<string, string>>& vars);

    void replaceDeclaration(string& statement);

    void replaceReturnStatement(string& statement);

    string replaceFunctionCallByReturnValue(string& statement, string& functionName, string& returnValue);

    void replaceCompoundAssignment(string& statement);

    void replaceStaticSingleAssignment(string& statement, set<pair<string, string>>& operands, set<pair<string, string>>& duplicate);

    void replaceStatement(string& statement, string& statementClass, set<pair<string, string>> operands);

    string replaceFunction(string& statement, string& statementClass, string& functionName, string& returnValue, set<pair<string, string>> operands);
};


#endif //TRANSPILER_H
