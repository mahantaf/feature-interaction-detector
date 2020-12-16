//
// Created by Mahan on 11/11/20.
//

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>

using namespace std;

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

    string fileName = "autonomoose_core/shuttle_manager/src/shuttle_manager_nodelet.cpp";
    string command = "ShuttleConfirmationCb CALL processNextShuttleRequest WRITE \"this->current_state\"";
    vector<string> commands = parseCommand(command);

    for (string c : commands)
        cout << c << endl;

}

