//
// Created by Mahan on 11/11/20.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>

using namespace std;


vector<string> split(string str, char splitter) {
    vector<string> words;
    string word = "";
    for (auto x : str) {
        if (x == splitter) {
            words.push_back(word);
            word = "";
        }
        else {
            word = word + x;
        }
    }
    words.push_back(word);
    return words;
}

void readRawCommands() {
    ifstream infile("Result.txt");
    string line;


    while (getline(infile, line)) {
        vector<string> splitedLine = split(line, ' ');
        if (splitedLine[0].compare("from") === 0) {

        }
    }
}

class FileSystem {
public:
    FileSystem() {
        this->folder = "temp/";
        this->symbolFile = "symbols.txt";
        this->constraintFile = "constraints.txt";
        this->functionParametersFile = "params.txt";
        this->functionParameterTypesFile = "param_types.txt";
    }

    void clearFile(string name) {
        string file;
        if (name.compare("constraints") == 0) {
            file = this->constraintFile;
        } else if (name.compare("params") == 0) {
            file = this->folder + this->functionParametersFile;
        } else if (name.compare("paramTypes") == 0) {
            file = this->folder + this->functionParameterTypesFile;
        } else if (name.compare("symbols") == 0) {
            file = this->folder + this->symbolFile;
        }
        std::ofstream ofs;
        ofs.open(file, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    vector<string> readFunctionParameters() {
        ifstream infile(this->folder + this->functionParametersFile);
        string line;
        vector<string> params;

        while (getline(infile, line) && line.compare("---------------"))
            params.push_back(line);
        return params;
    }

    vector<string> readFunctionParameterTypes() {
        ifstream infile(this->folder + this->functionParameterTypesFile);
        string line;
        vector<string> paramTypes;

        while (getline(infile, line) && line.compare("---------------"))
            paramTypes.push_back(line);
        return paramTypes;
    }

    vector<vector<string>> readVarInFuncParameters() {
        ifstream infile(this->folder + this->functionParametersFile);
        string line;
        vector<vector<string>> parametersList;
        vector<string> parameters;

        while (getline(infile, line)) {
//            cout << line << endl;
            if (line.compare("---------------") == 0) {
                parametersList.push_back(parameters);
                parameters.clear();
            } else {
                parameters.push_back(line);
            }
        }
        return parametersList;
    }

    vector<vector<string>> readVarInFuncParameterTypes() {
        ifstream infile(this->folder + this->functionParameterTypesFile);
        string line;
        vector<vector<string>> parameterTypesList;
        vector<string> parameterTypes;

        while (getline(infile, line)) {
            cout << line << endl;
            if (line.compare("---------------") == 0) {
                parameterTypesList.push_back(parameterTypes);
                parameterTypes.clear();
            } else {
                parameterTypes.push_back(line);
            }
        }
        return parameterTypesList;
    }

    vector<vector<string>> readFunctionConstraints() {
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

    void writeFunctionParameters(vector<string> params) {

        string fileName = this->folder + this->functionParametersFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(params, file);
    }

    void writeFunctionParametersType(vector<string> paramTypes) {
        string fileName = this->folder + this->functionParameterTypesFile;
        ofstream file (fileName, ofstream::out | ofstream::trunc);
        return this->writeConstraints(paramTypes, file);
    }

    void writeMainConstraints(vector<vector<string>>& constraints) {
        ofstream file (this->constraintFile, ofstream::out | ofstream::trunc);
        if (file.is_open()) {
            for (vector<string> constraint : constraints) {
                for (string s: constraint)
                    file << s << endl;
                file << "---------------\n";
            }
        }
        file.close();
    }

protected:
    void writeConstraints(vector<string>& constraints, ofstream& file) {
        if (file.is_open()) {
            for (string s: constraints)
                file << s << endl;
            file << "---------------\n";
            file.close();
        }
    }
private:
    string folder;
    string symbolFile;
    string constraintFile;
    string functionParametersFile;
    string functionParameterTypesFile;
};

class Command {
public:

    Command(string functionName, string incident, string command) {
        this->functionName = functionName;
        this->incident = incident;
        this->command = command;
    }

    void setParams(vector<string> params) {
        this->params = params;
    }

    void setParamTypes(vector<string> paramTypes) {
        this->paramTypes = paramTypes;
    }

    void print() {
        cout << this->functionName << endl;
        cout << this->command << endl;
        cout << this->incident << endl;
        cout << "Params:" << endl;
        for (string p : this->params)
            cout << p << endl;
        cout << "ParamTypes:" << endl;
        for (string pt: this->paramTypes)
            cout << pt << endl;
        cout << "-----------------\n";
        if (subCommands.size()) {
            cout << "Sub Commands:\n";
            for (Command c : subCommands)
                c.print();
        }
    }

    vector<string> params;
    vector<string> paramTypes;
    string command;
    string functionName;
    string incident;
};

vector<string> parseCommand(string str) {
    vector<string> commands;
    string word = "";
    for (auto x : str) {
        if (x == ' ') {
            commands.push_back(word);
            word = "";
        }
        else {
            word = word + x;
        }
    }
    commands.push_back(word);
    return commands;
}


FileSystem fs = FileSystem();

void runCommand(Command& command, string fileName, string isRoot, int clCount) {

    fs.clearFile("params");
    fs.clearFile("paramTypes");

    if (command.params.size()) {
        fs.writeFunctionParameters(command.params);
        fs.writeFunctionParametersType(command.paramTypes);
    }

    string run = "./cfg " + fileName + " -- " + command.functionName + " " + command.incident + " " + command.command + " " + isRoot + " " + to_string(clCount);
    cout << run << endl;
    system(run.c_str());
}

int main(int argc, const char **argv) {

//    string fileName = "tests/multiple/test.cpp";
//    string command = "bar WRITE c VARWRITE h";
//    string command = "foo CALL bar WRITE c VARINFUNC isTrue WRITE c";


//    string fileName = "autonomoose_core/shuttle_manager/src/shuttle_manager_nodelet.cpp";
//    string command = "ShuttleConfirmationCb WRITE current_state VARINFFUNC processNextShuttleRequest WRITE current_state";

    string fileName = "autonomoose_core/route_publisher/src/route_publisher_nodelet.cpp";
    string command = "refGPSCb WRITE first_init_path VARWRITE update_needed";
//    string command = "refGPSCb WRITE ref_gps_flag_ VARINFFUNC updatePath CALL fillPathsFromRoutePlan WRITE route_plan";

    vector<string> commands = parseCommand(command);

    vector<vector<Command>> commandLists;
    vector<Command> cl;

    commandLists.push_back(cl);

    // Pass 1: Fill metadata

    bool call = false;
    fs.clearFile("symbols");
    fs.clearFile("constraints");
    for (int i = 0; i + 2 < commands.size(); i = i + 2) {
        string functionName = commands[i];
        string command = commands[i + 1];
        string incident = commands[i + 2];

        Command c = Command(functionName, incident, command);

        if (call) {
            call = false;
            vector <string> params = fs.readFunctionParameters();
            vector <string> paramTypes = fs.readFunctionParameterTypes();
            c.setParams(params);
            c.setParamTypes(paramTypes);
        }

        if (command.compare("CALL") == 0) {
            call = true;
            string run = "./cfg " + fileName + " -- " + functionName + " " + incident + " FUNCTION 0";
            cout << "Executing: " << run << endl;
            system(run.c_str());
        } else if (command.compare("VARINFFUNC") == 0) {

            for (int i = 0; i < commandLists.size(); i++) {
                commandLists[i].push_back(c);
            }

            string run = "./cfg " + fileName + " -- " + functionName + " " + incident + " VARINFUNCEXTEND 0";
            cout << "Executing: " << run << endl;
            system(run.c_str());

            string nextFunctionName = commands[i + 2];
            string nextCommand = commands[i + 3];
            string nextIncident = commands[i + 4];

            vector <vector<string>> varInFuncParameters = fs.readVarInFuncParameters();
            vector <vector<string>> varInFuncParameterTypes = fs.readVarInFuncParameterTypes();

            vector<vector<Command>> newCommandLists;

            for (int i = 0; i < varInFuncParameters.size(); ++i) {

                vector<vector<Command>> copyCommandLists = commandLists;

                Command subCommand(nextFunctionName, nextIncident, nextCommand);
                subCommand.setParams(varInFuncParameters[i]);
                subCommand.setParamTypes(varInFuncParameterTypes[i]);

                for (int i = 0; i < copyCommandLists.size(); i++) {
                    copyCommandLists[i].push_back(subCommand);
                }

                newCommandLists.insert(newCommandLists.end(), copyCommandLists.begin(), copyCommandLists.end());
            }
            commandLists = newCommandLists;
            i = i + 2;
            continue;
        }

        for (int i = 0; i < commandLists.size(); i++) {
            commandLists[i].push_back(c);
        }
    }

    for (vector<Command> cl: commandLists) {
        cout << "----------COMMAND LIST----------\n";
        for (Command c : cl)
            c.print();
    }
    // Pass 2: Traverse and run the commands
    cout << "Starting pass 2:\n";

    vector<vector<string>> constraintList;
    int i = 0;
    for (vector<Command> commandList: commandLists) {
        cout << "Running command list: " << i << endl;

        fs.clearFile("symbols");
        fs.clearFile("constraints");

        for (vector<Command>::reverse_iterator it = commandList.rbegin(); it != commandList.rend(); ++it) {

            Command currentCommand = (*it);

            string isRoot = "";

            if (it != --commandList.rend())
                isRoot += "0";
            else
                isRoot += "1";

            runCommand(currentCommand, fileName, isRoot, i);
        }
        vector<vector<string>> newConstraintList = fs.readFunctionConstraints();
        constraintList.insert(constraintList.begin(), newConstraintList.begin(), newConstraintList.end());
        i++;
    }
    fs.writeMainConstraints(constraintList);
}


int x = 2;

void H() {
  return;
}

void I() {
  if (x > 2) {
    H();
  }
}

int G(int c) {
  x = c * 2;
  return x;
}

void F() {
  int a = 2;
  int b = G(a);
}































