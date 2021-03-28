//
// Created by mahan on 3/28/21.
//

#include "SymbolTable.h"

SymbolTable* SymbolTable::getInstance() {
    if (!instance)
        instance = new SymbolTable;
    return instance;
}

string SymbolTable::addVariableSymbol(string var, string type) {
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
            var = var + "_" + this->functionName;
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

map<string, pair<set<string>, string>> SymbolTable::getTable() {
    return this->symbolTable;
}

void SymbolTable::setTable(map<string, pair<set<string>, string>> table) {
    this->symbolTable = table;
}

void SymbolTable::setType(string type) {
    this->type = type;
}

void SymbolTable::setParams(vector<string> params) {
    this->params = params;
}

void SymbolTable::setPassParams(vector<string> passParams) {
    this->passParams = passParams;
}

void SymbolTable::setFunctionName(string functionName) {
    this->functionName = functionName;
}

void SymbolTable::saveState() {
    this->fs.writeSymbolTable(this->symbolTable);
}

void SymbolTable::saveParameters(string functionName, vector<string> parameters) {
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

void SymbolTable::saveParameterTypes(string functionName, vector<string> parameters, vector<string> parameterTypes) {
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

void SymbolTable::loadParameterTypes(string functionName) {
    this->passParamTypes = this->fs.readParameterTypes(functionName);
}

void SymbolTable::loadParameters(string functionName) {
    this->passParams = this->fs.readParameters(functionName);
}

void SymbolTable::loadFunctionParameterTypes() {
    this->passParamTypes = this->fs.readFunctionParameterTypes();
}

void SymbolTable::loadFunctionParameters() {
    this->passParams = this->fs.readFunctionParameters();
}

void SymbolTable::loadState() {
    this->symbolTable = this->fs.readSymbolTable();
}

void SymbolTable::print() {
    for (map<string, pair<set<string>, string>>::const_iterator it = symbolTable.begin(); it != symbolTable.end(); ++it) {
        cout << it->first << ": (";
        pair<set<string>, string> symbols = it->second;
        for (string s: symbols.first) {
            cout << s << ' ';
        }
        cout << ") " << symbols.second << endl;
    }
}

string SymbolTable::insertNewSymbol(string variable) {
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

string SymbolTable::getVariableLastSymbol(string variable) {
    set<string> variableSymbols = this->getVariableSymbols(variable);
    return *--variableSymbols.end();
}

set<string> SymbolTable::getVariableSymbols(string variable) {
    return this->symbolTable[variable].first;
}

bool SymbolTable::isMemberVariable(string variable) {
    if (variable.find("this->") != string::npos)
        return true;
    return false;
}

bool SymbolTable::isLiteral(string variableType) {
    if (variableType.find("Literal") != string::npos)
        return true;
    return false;
}

int SymbolTable::isInParams(string variable) {
    for (int i = 0; i < params.size(); ++i)
        if (params[i].compare(variable) == 0)
            return i;
    return -1;
}

void SymbolTable::setVariableSymbol(string variable, string symbol) {
    set<string> variableSymbols = this->getVariableSymbols(variable);
    variableSymbols.insert(symbol);
    this->symbolTable[variable].first = variableSymbols;
}

void SymbolTable::setVariableSymbols(string variable, set<string> symbols) {
    this->symbolTable[variable].first = symbols;
}

SymbolTable::SymbolTable() {
    this->fs = FileSystem();
    map<string, pair<set<string>, string>> st;
    symbol = "s";
    symbolTable = st;
}
