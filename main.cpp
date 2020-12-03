//
// Created by Mahan on 11/11/20.
//

#include <iostream>
#include <string>
#include <stdlib.h>
#include <thread>

using namespace std;

int main(int argc, const char **argv) {
  system("./cfg test.cpp -- foo bar CALL");
  system("./cfg test.cpp -- bar c WRITE");
}

