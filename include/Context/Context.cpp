//
// Created by Mahan Tafreshipour on 3/29/21.
//

#include "Context.h"
#include "../Incident/Incident.h"

Context* Context::getInstance() {
  if (!instance)
    instance = new Context;
  return instance;
}

clang::ast_matchers::MatchFinder::MatchResult* Context::getContext() {
  return this->context;
}

vector<vector<string>> Context::getInitialConstraintsList() {
  return this->initialConstraintsList;
}

bool Context::getConstraintsListSet() {
  return this->constraintsListSet;
}

string Context::getCurrentFunction() {
  return this->currentFunction;
}

Incident* Context::getIncident() {
    return this->incident;
}

string Context::getFilePath() {
    return this->filePath;
}

void Context::setContext(clang::ast_matchers::MatchFinder::MatchResult context) {
    this->context = &context;
}

void Context::setInitialConstraintsList(vector<vector<string>> initialConstraintsList) {
  this->initialConstraintsList = initialConstraintsList;
  this->constraintsListSet = true;
}

void Context::setCurrentFunction(string currentFunction) {
  this->currentFunction = currentFunction;
}

void Context::setIncident(Incident* incident) {
    this->incident = incident;
}

void Context::setFilePath(string filePath) {
    this->filePath = filePath;
}

Context::Context() { this->constraintsListSet = false; }
