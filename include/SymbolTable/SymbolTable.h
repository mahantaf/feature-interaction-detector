//
// Created by mahan on 3/28/21.
//

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "../FileSystem/FileSystem.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

class SymbolTable {
public:
    static SymbolTable *getInstance();

    string addVariableSymbol(string var, string type);

    map<string, pair<set<string>, string>> getTable();

    void setTable(map<string, pair<set<string>, string>> table);

    void setType(string type);

    void setParams(vector<string> params);

    void setPassParams(vector<string> passParams);

    void setFunctionName(string functionName);

    void saveState();

    void saveParameters(string functionName, vector<string> parameters);

    void saveParameterTypes(string functionName, vector<string> parameters, vector<string> parameterTypes);

    void loadParameterTypes(string functionName);

    void loadParameters(string functionName);

    void loadFunctionParameterTypes();

    void loadFunctionParameters();

    void loadState();

    void print();

protected:
    string insertNewSymbol(string variable);

    string getVariableLastSymbol(string variable);

    set<string> getVariableSymbols(string variable);

    bool isMemberVariable(string variable);

    bool isLiteral(string variableType);

    int isInParams(string variable);

    void setVariableSymbol(string variable, string symbol);

    void setVariableSymbols(string variable, set<string> symbols);
private:
    SymbolTable();
    FileSystem fs;
    string type;
    string functionName;
    vector<string> params;
    vector<string> passParams;
    vector<string> passParamTypes;
    static SymbolTable *instance;
    string symbol;
    map<string, pair<set<string>, string>> symbolTable;
};


#endif // SYMBOLTABLE_H
