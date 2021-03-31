//
// Created by Mahan Tafreshipour on 3/29/21.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

#include <string>
#include <vector>

using namespace std;

class Incident;

class Context {
public:

  static Context *getInstance();

  clang::ast_matchers::MatchFinder::MatchResult* getContext();

  vector<vector<string>> getInitialConstraintsList();

  bool getConstraintsListSet();

  string getCurrentFunction();

  Incident* getIncident();

  string getFilePath();

  void setContext(clang::ast_matchers::MatchFinder::MatchResult context);

  void setInitialConstraintsList(vector<vector<string>> initialConstraintsList);

  void setCurrentFunction(string currentFunction);

  void setIncident(Incident* incident);

  void setFilePath(string filePath);


private:
  Context();
  static Context *instance;
  clang::ast_matchers::MatchFinder::MatchResult* context;
  vector<vector<string>> initialConstraintsList;
  bool constraintsListSet;
  string currentFunction;
  Incident* incident;
  string filePath;
};


#endif //CONTEXT_H
