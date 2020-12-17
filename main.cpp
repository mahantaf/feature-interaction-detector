//
// Created by Mahan on 11/11/20.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>

using namespace std;

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
    }

    vector<string> params;
    vector<string> paramTypes;
    string command;
    string functionName;
    string incident;
    Command* next;
    Command* prev;
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

int main(int argc, const char **argv) {
//  system("./cfg test.cpp -- foo bar CALL");
//  system("./cfg test.cpp -- bar c WRITE");

    FileSystem fs = FileSystem();

    string fileName = "tests/single/test.cpp";
    string command = "foo CALL bar WRITE r";
//    string command = "bar WRITE r";

    vector<string> commands = parseCommand(command);
    vector<Command> commandList;

//    for (string c : commands)
//        cout << c << endl;

    // Pass 1: Fill metadata

    bool call = false;
    for (int i = 0; i + 2 < commands.size(); i = i + 2) {
        string functionName = commands[i];
        string command = commands[i + 1];
        string incident = commands[i + 2];
        if (command.compare("CALL") == 0) {
            call = true;
            string run = "./cfg " + fileName + " -- " + functionName + " " + incident + " FUNCTION 0";
            system(run.c_str());
            commandList.push_back(Command(functionName, incident, command));
        } else if (call) {
            call = false;
            vector <string> params = fs.readFunctionParameters();
            vector <string> paramTypes = fs.readFunctionParameterTypes();
            Command c = Command(functionName, incident, command);
            c.setParams(params);
            c.setParamTypes(paramTypes);
            commandList.push_back(c);
        } else {
            Command c = Command(functionName, incident, command);
            commandList.push_back(c);
        }
    }
//    for (Command c: commandList) {
//        c.print();
//    }

    // Pass 2: Traverse and run the commands
    cout << "Starting pass 2:\n";
    fs.clearFile("symbols");
    fs.clearFile("constraints");
    for (vector<Command>::reverse_iterator it = commandList.rbegin(); it != commandList.rend(); ++it) {
        Command currentCommand = (*it);
        fs.clearFile("params");
        fs.clearFile("paramTypes");
        if (currentCommand.params.size()) {
            fs.writeFunctionParameters(currentCommand.params);
            fs.writeFunctionParametersType(currentCommand.paramTypes);
        }
        string run = "./cfg " + fileName + " -- " + currentCommand.functionName + " " + currentCommand.incident + " " + currentCommand.command;
        if (it != --commandList.rend())
            run += " 0";
        else
            run += " 1";
        system(run.c_str());
    }
}

