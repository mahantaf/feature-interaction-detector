//
// Created by mahan on 3/28/21.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

class FileSystem {
public:
    FileSystem();

    void clearMainConstraintsFile();

    void clearWriteConstarintsFile();

    void clearVarWriteConstraintsFile();

    map<string, pair<set<string>, string>> readSymbolTable();

    vector<string> readParameterTypes(string functionName);

    vector<string> readParameters(string functionName);

    vector<string> readFunctionParameterTypes();

    vector<string> readFunctionParameters();

    vector<vector<string>> readFunctionConstraints();

    vector<vector<string>> readVarWriteConstraints();

    vector<vector<string>> readReturnConstraints(vector<string>& returnValues);

    void writeSymbolTable(map<string, pair<set<string>, string>>& symbolTable);

    void writeParametersType(string functionName, vector<string> paramTypes);

    void writeFunctionParametersType(vector<string> paramTypes);

    void writeVarInFuncParametersType(vector<string> paramTypes, bool& varInFuncTypeLock);

    void writeParameters(string functionName, vector<string> params);

    void writeFunctionParameters(vector<string> params);

    void writeVarInFuncParameters(vector<string> params, bool& varInFuncLock);

    void writeFunctionFilePath(string filePath);

    void writeMainConstraints(vector<string>& constraints);

    void writeVarWriteConstraints(vector<string>& constraints);

    void writeReturnConstraints(vector<string>& constraints);

protected:
    void writeConstraints(vector<string>& constraints, ofstream& file);
private:
    string folder;
    string constraintFile;
    string returnConstraintFile;
    string varWriteConstraintFile;
    string symbolTableFile;
    string parametersFile;
    string parameterTypesFile;
    string functionPathFile;
    string functionParametersFile;
    string functionParameterTypesFile;
};

#endif //FILESYSTEM_H
