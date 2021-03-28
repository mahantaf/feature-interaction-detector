//
// Created by mahan on 3/28/21.
//

#include "FileSystem.h"

FileSystem::FileSystem() {
    this->folder = "/home/mahan/Projects/Thesis/feature-interaction-detector/temp/";
    this->constraintFile = "/home/mahan/Projects/Thesis/feature-interaction-detector/constraints.txt";
    this->returnConstraintFile = "return_constraints.txt";
    this->varWriteConstraintFile = "var_write_constraints.txt";
    this->symbolTableFile = "symbols.txt";
    this->parametersFile = "_params.txt";
    this->parameterTypesFile = "_param_types.txt";
    this->functionParametersFile = "params.txt";
    this->functionParameterTypesFile = "param_types.txt";
}

void FileSystem::clearMainConstraintsFile() {
    std::ofstream ofs;
    ofs.open(this->constraintFile, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

void FileSystem::clearWriteConstarintsFile() {
    std::ofstream ofs;
    ofs.open(this->folder + this->returnConstraintFile, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

void FileSystem::clearVarWriteConstraintsFile() {
    std::ofstream ofs;
    ofs.open(this->folder + this->varWriteConstraintFile, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

map<string, pair<set<string>, string>> FileSystem::readSymbolTable() {
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

vector<string> FileSystem::readParameterTypes(string functionName) {
    ifstream infile(this->folder + functionName + this->parameterTypesFile);
    string line;
    vector<string> paramTypes;

    while (getline(infile, line) && line.compare("---------------"))
        paramTypes.push_back(line);
    return paramTypes;
}

vector<string> FileSystem::readParameters(string functionName) {
    ifstream infile(this->folder + functionName + this->parametersFile);
    string line;
    vector<string> params;

    while (getline(infile, line) && line.compare("---------------"))
        params.push_back(line);
    return params;
}

vector<string> FileSystem::readFunctionParameterTypes() {
    ifstream infile(this->folder + this->functionParameterTypesFile);
    string line;
    vector<string> paramTypes;

    while (getline(infile, line) && line.compare("---------------"))
        paramTypes.push_back(line);
    return paramTypes;
}

vector<string> FileSystem::readFunctionParameters() {
    ifstream infile(this->folder + this->functionParametersFile);
    string line;
    vector<string> params;

    while (getline(infile, line) && line.compare("---------------"))
        params.push_back(line);
    return params;
}

vector<vector<string>> FileSystem::readFunctionConstraints() {
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

vector<vector<string>> FileSystem::readVarWriteConstraints() {
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

vector<vector<string>> FileSystem::readReturnConstraints(vector<string>& returnValues) {
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

void FileSystem::writeSymbolTable(map<string, pair<set<string>, string>>& symbolTable) {
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

void FileSystem::writeParametersType(string functionName, vector<string> paramTypes) {
    string fileName = this->folder + functionName + this->parameterTypesFile;
    ofstream file (fileName, ofstream::out | ofstream::trunc);
    return this->writeConstraints(paramTypes, file);
}

void FileSystem::writeFunctionParametersType(vector<string> paramTypes) {
    string fileName = this->folder + this->functionParameterTypesFile;
    ofstream file (fileName, ofstream::out | ofstream::trunc);
    return this->writeConstraints(paramTypes, file);
}

void FileSystem::writeVarInFuncParametersType(vector<string> paramTypes, bool varInFuncTypeLock) {
    string fileName = this->folder + this->functionParameterTypesFile;
    if (varInFuncTypeLock) {
        ofstream file(fileName, ios_base::app);
        return this->writeConstraints(paramTypes, file);
    } else {
        varInFuncTypeLock = true;
        ofstream file(fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(paramTypes, file);
    }
}

void FileSystem::writeParameters(string functionName, vector<string> params) {
    string fileName = this->folder + functionName + this->parametersFile;
    ofstream file (fileName, ofstream::out | ofstream::trunc);
    return this->writeConstraints(params, file);
}

void FileSystem::writeFunctionParameters(vector<string> params) {
    string fileName = this->folder + this->functionParametersFile;
    ofstream file (fileName, ofstream::out | ofstream::trunc);
    return this->writeConstraints(params, file);
}

void FileSystem::writeVarInFuncParameters(vector<string> params, bool varInFuncLock) {
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

void FileSystem::writeMainConstraints(vector<string>& constraints) {
    ofstream file (this->constraintFile, ios_base::app);
    return this->writeConstraints(constraints, file);
}

void FileSystem::writeVarWriteConstraints(vector<string>& constraints) {
    ofstream file (this->folder + this->varWriteConstraintFile, ios_base::app);
    return this->writeConstraints(constraints, file);
}

void FileSystem::writeReturnConstraints(vector<string>& constraints) {
    ofstream file;
    file.open(this->folder + this->returnConstraintFile, ios_base::app);
    return this->writeConstraints(constraints, file);
}

void FileSystem::writeConstraints(vector<string>& constraints, ofstream& file) {
    if (file.is_open()) {
        for (string s: constraints)
            file << s << endl;
        file << "---------------\n";
        file.close();
    }
}

