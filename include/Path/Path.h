//
// Created by mahan on 3/31/21.
//

#ifndef PATH_H
#define PATH_H

#include <vector>
#include <string>

using namespace std;

class Path {
public:
    Path(vector<string> constraints);

    bool compare(Path path);

    vector<string> getConstraints();
private:
    vector<string> constraints;
};


#endif //PATH_H
