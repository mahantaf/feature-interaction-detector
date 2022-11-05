//
// Created by mahan on 3/31/21.
//

#include "CFGHandler.h"

CFGHandler::CFGHandler(const unique_ptr<clang::CFG>& cfg) {
    this->fs = FileSystem();
    this->cfgBlockHandler = CFGBlockHandler();
    Context* context = context->getInstance();
    this->incident = context->getIncident();
    if (cfg)
        this->cfg = &cfg;
}

bool CFGHandler::hasIncident() {
    return this->incidentBlocks.size() > 0;
}

void CFGHandler::findIncidents() {
    for (const auto* blk : **(this->cfg)) {
        for (clang::CFGBlock::const_iterator I = blk->begin(), E = blk->end(); I != E; ++I) {
            clang::CFGElement El = *I;
            if (auto SE = El.getAs<clang::CFGStmt>()) {
                const clang::Stmt *stmt = SE->getStmt();
                if (this->incident->hasIncident(stmt, this->returnValues)) {
                    this->incidentBlocks.push_back(blk);
                    this->incidentStatements.push_back(stmt);
                }
            }
        }
        blk->dump();
    }
}

void CFGHandler::findChildStatementIncident(const clang::Stmt* stmt, bool& varInFuncLock, bool& varInFuncTypeLock) {
    VarInFuncIncident* varInFuncIncident = dynamic_cast<VarInFuncIncident*>(this->incident);
    vector<const clang::Stmt*> incidentValues;
    int result = varInFuncIncident->hasIncidentExtend(stmt, incidentValues);
    if (result != -1) {
        this->fs.writeVarInFuncParameters(varInFuncIncident->getParams(), varInFuncLock);
        this->fs.writeVarInFuncParametersType(varInFuncIncident->getParamTypes(), varInFuncTypeLock);
        this->fs.writeFunctionFilePath(varInFuncIncident->getFilePath());
    }
}

void CFGHandler::findStatementIncident(const clang::Stmt* stmt, int& varInFuncCounter, string& varInFuncCount) {

    VarInFuncIncident* varInFuncIncident = dynamic_cast<VarInFuncIncident*>(this->incident);

    vector<const clang::Stmt*> incidentValues;
    int result = varInFuncIncident->hasIncidentExtend(stmt, incidentValues);

    if (result != -1) {

        if (to_string(varInFuncCounter).compare(varInFuncCount) != 0) {
            varInFuncCounter++;
            return;
        }
        varInFuncCounter++;

        bool _if = result == 1;
        Context *context = context->getInstance();
        vector<vector<string>> initialConstraintsList = context->getInitialConstraintsList();

        if (initialConstraintsList.size()) {
          SymbolTable *st = st->getInstance();
          map<string, pair<set<string>, string>> tableCopy = st->getTable();
          cout << "Incident values size:  " << incidentValues.size() << endl;
            for (const clang::Stmt *statement : incidentValues) {
              this->paths.clear();
              for (vector<string> initialConstraints : initialConstraintsList) {
                st->setTable(tableCopy);
                vector<string> constraints = initialConstraints;
                string constraint = this->cfgBlockHandler.getIfStmtCondition(statement, _if);
                constraints.push_back(constraint);
                this->paths.push_back(Path(constraints));
              }
            }
        } else {
            for (const clang::Stmt* statement : incidentValues) {
                this->paths.clear();
                vector<string> constraints;
                string constraint = this->cfgBlockHandler.getIfStmtCondition(statement, _if);
                constraints.push_back(constraint);
                this->paths.push_back(Path(constraints));
            }
        }
        cout << "Path size: " << this->paths.size();
        this->writePaths();
    }
}

void CFGHandler::bottomUpTraverse(const clang::CFGBlock* startBlock, unsigned int previousBlockId, vector<string>& constraints, bool previousCollect) {
    cout << "Visiting Block " << startBlock->getBlockID() << endl;
    if (startBlock->getBlockID() == this->getEntryBlockId()) {
        return this->paths.push_back(Path(constraints));
    }
    string terminatorCondition = "";
    if (previousBlockId != 0) {
        terminatorCondition = this->cfgBlockHandler.getTerminatorBlockCondition(startBlock, previousBlockId, previousCollect);
        if (terminatorCondition.compare(""))
            constraints.push_back(terminatorCondition);
    }
    vector<vector<string>> constraintsList = this->cfgBlockHandler.getBlockCondition(startBlock, constraints);
    SymbolTable *st = st->getInstance();
    map<string, pair<set<string>, string>> tableCopy = st->getTable();
    for (clang::CFGBlock::const_pred_iterator I = startBlock->pred_begin(), E = startBlock->pred_end(); I != E; I++) {
        bool collect = (previousBlockId == 0) || (terminatorCondition.compare("") != 0) || (!(constraintsList.size() == 1 && constraintsList[0].size() == constraints.size()));
        for (vector<string> c: constraintsList) {
            vector<string> constraintsCopy = c;
            st->setTable(tableCopy);
            this->bottomUpTraverse((*I).getReachableBlock(), startBlock->getBlockID(), constraintsCopy, collect);
        }
    }
}

vector<vector<string>> CFGHandler::readConstraintsList() {
    return this->fs.readFunctionConstraints();
}

void CFGHandler::writeConstraintsList(vector<vector<string>> constraintsList) {
  for (vector<string> constraints : constraintsList) {
    this->fs.writeMainConstraints(constraints);
  }
}

void CFGHandler::clearConstraintsList() {
    this->fs.clearMainConstraintsFile();
}

void CFGHandler::collectConstraints() {
    if (this->incident->getType().compare("FUNCTION") == 0) {
        cout << "In collecting constraints" << endl;
        FunctionIncident* functionIncident = dynamic_cast<FunctionIncident*>(this->incident);
        this->fs.writeFunctionParameters(functionIncident->getParams());
        this->fs.writeFunctionParametersType(functionIncident->getParamTypes());
        this->fs.writeFunctionFilePath(functionIncident->getFilePath());
        return;
    }
    vector<vector<string>> initialConstraintsList;
    if (this->incident->getType().compare("RETURN") != 0) {
        initialConstraintsList = this->fs.readFunctionConstraints();
        if (this->incidentBlocks.size()) { // If there is an incident in this command then clear the file o.w keep it.
          this->fs.clearMainConstraintsFile();
        }
    } else {
        this->fs.clearWriteConstarintsFile();
    }
    if (initialConstraintsList.size()) {
        SymbolTable *st = st->getInstance();
        map<string, pair<set<string>, string>>tableCopy = st->getTable();
        for (vector<string> initialConstraints : initialConstraintsList) {
            for (vector<const clang::CFGBlock*>::iterator blk = this->incidentBlocks.begin(); blk != this->incidentBlocks.end(); ++blk) {
                st->setTable(tableCopy);
                cout << "Incident Block: " << (*blk)->getBlockID() << endl;

                this->paths.clear();
                vector <string> pathConstraints = initialConstraints;

                this->bottomUpTraverse((*blk), 0, pathConstraints, false);
                this->writePaths();
            }
        }
    } else {
        SymbolTable *st = st->getInstance();
        map<string, pair<set<string>, string>>tableCopy = st->getTable();
        for (vector<const clang::CFGBlock *>::iterator blk = this->incidentBlocks.begin(); blk != this->incidentBlocks.end(); ++blk) {
            st->setTable(tableCopy);
            cout << "Incident Block: " << (*blk)->getBlockID() << endl;

            this->paths.clear();
            vector <string> pathConstraints;

            this->bottomUpTraverse((*blk), 0, pathConstraints, false);
            this->writePaths();
        }
    }
}

void CFGHandler::writePaths() {
    cout << "Constrains:" << endl;
    Context* context = context->getInstance();
    this->removeDuplicatePaths();
    for (Path path: this->paths) {
        vector<string> constraints = path.getConstraints();
        if (context->getIncident()->getType().compare("RETURN") == 0)
            this->fs.writeReturnConstraints(constraints);
        else if (context->getIncident()->getType().compare("VARWRITE") == 0)
            this->fs.writeVarWriteConstraints(constraints);
        else
            this->fs.writeMainConstraints(constraints);

        cout << "Sub Path: ";
        for (string s: constraints)
            cout << s << " & ";
        cout << endl;
    }
}

void CFGHandler::writeVarWritePaths() {
    vector<vector<string>> varWritePaths = this->fs.readVarWriteConstraints();
    this->fs.clearMainConstraintsFile();
    for (vector<string> constraints: varWritePaths)
        this->fs.writeMainConstraints(constraints);
    this->fs.clearVarWriteConstraintsFile();
}

unsigned int CFGHandler::getEntryBlockId() {
    return (*(this->cfg))->getEntry().getBlockID();
}

vector<const clang::CFGBlock*> CFGHandler::getIncidentBlocks() {
    return this->incidentBlocks;
}

vector<string> CFGHandler::getReturnValues() {
    return this->returnValues;
}

void CFGHandler::removeDuplicatePaths() {
    for (long unsigned int i = 0; i < this->paths.size(); i++) {
        for (long unsigned int j = i + 1; j < this->paths.size(); j++) {
            if (this->paths[i].compare(this->paths[j])) {
                this->paths.erase(this->paths.begin() + j);
                j--;
            }
        }
    }
}