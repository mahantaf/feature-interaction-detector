//
// Created by mahan on 3/31/21.
//

#include "Path.h"

Path::Path(vector<string> constraints) {
    this->constraints = constraints;
}

bool Path::compare(Path path) {
    vector<string> pathConstraints = path.getConstraints();
    if (pathConstraints.size() != this->constraints.size())
        return false;

    for (long unsigned int i = 0; i < this->constraints.size(); i++)
        if (this->constraints[i].compare(pathConstraints[i]) != 0)
            return false;
    return true;
}

vector<string> Path::getConstraints() {
    return this->constraints;
}