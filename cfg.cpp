//
// Created by Mahan on 11/6/20.
//

// Clang includes
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/Inclusions/HeaderIncludes.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>

// LLVM includes
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <map>
#include <set>
#include <stdlib.h>
#include <experimental/filesystem>
#include <filesystem>
#include <thread>
#include <exception>

#include "include/Path/Path.h"
#include "include/Context/Context.h"
#include "include/Incident/Incident.h"
#include "include/FileSystem/FileSystem.h"
#include "include/Transpiler/Transpiler.h"
#include "include/SymbolTable/SymbolTable.h"
#include "include/StatementHandler/StatementHandler.h"
#include "include/CFGBlockHandler/CFGBlockHandler.h"
#include "include/CFGHandler/CFGHandler.h"

using namespace std;
using namespace llvm;
//namespace fs =  std::experimental::filesystem;

string filePath, functionName, incident, incidentType, root, varInFuncCount;

int varInFuncCounter = 0;
bool varInFuncLock = false;
bool varInFuncTypeLock = false;


Incident* createIncident() {
    if (incidentType.compare("CALL") == 0) {
        return new CallIncident(incident);
    }
    if (incidentType.compare("WRITE") == 0) {
        return new WriteIncident(incident);
    }
    if (incidentType.compare("RETURN") == 0) {
        return new ReturnIncident(incident);
    }
    if (incidentType.compare("FUNCTION") == 0) {
        return new FunctionIncident(incident);
    }
    if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {
        return new VarInFuncIncident(incident);
    }
    if (incidentType.compare("VARWRITE") == 0) {
        return new VarWriteIncident(incident);
    }
    return nullptr;
}

class VarWriteCallBack : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    VarWriteCallBack() {}

    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        Context *context = context->getInstance();
        context->setContext(Result);
        const auto *Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
        if (Function->isThisDeclarationADefinition() && Function->hasBody()) {
            const clang::Stmt *funcBody = Function->getBody();
            clang::SourceLocation SL = funcBody->getBeginLoc();
            const clang::SourceManager *SM = Result.SourceManager;
            if (SM->isInMainFile(SL)) {

                SymbolTable *se = se->getInstance();
                se->setFunctionName(Function->getNameInfo().getAsString());
                se->loadState();

                vector <string> params;
                for (clang::FunctionDecl::param_const_iterator I = Function->param_begin(), E = Function->param_end();
                     I != E; ++I) {
                    string name = (*I)->getNameAsString();
                    params.push_back(name);
                }
                se->setParams(params);
                se->setType("FUNCTION");

                const auto cfg = clang::CFG::buildCFG(Function, Function->getBody(), Result.Context,
                                                      clang::CFG::BuildOptions());

                CFGHandler cfgHandler(cfg);
                cfgHandler.findIncidents();
                cfgHandler.collectConstraints();
            }

        }
    }
};

class MyCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    MyCallback() {}
    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
        Context *context = context->getInstance();
        context->setContext(Result);

        if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {

            CFGHandler cfgHandler(nullptr);
            if (!context->getConstraintsListSet()) {
                context->setInitialConstraintsList(cfgHandler.readConstraintsList());
                cfgHandler.clearConstraintsList();
            }

            SymbolTable *se = se->getInstance();
            se->setType("FUNCTION");

            const auto *If = Result.Nodes.getNodeAs<clang::IfStmt>("if");
            clang::SourceLocation SL = If->getBeginLoc();
            const clang::SourceManager *SM = Result.SourceManager;
            if (SM->isInMainFile(SL)) {
                CFGHandler cfgHandler(nullptr);
                if (incidentType.compare("VARINFFUNC") == 0)
                    cfgHandler.findStatementIncident(If, varInFuncCounter, varInFuncCount);
                else if (incidentType.compare("VARINFUNCEXTEND") == 0)
                    cfgHandler.findChildStatementIncident(If, varInFuncLock, varInFuncTypeLock);
                return;
            }
        } else {
            const auto* Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
            if (Function->isThisDeclarationADefinition()) {
                SymbolTable *se = se->getInstance();
//                se->loadState();
                vector <string> params;
                for (clang::FunctionDecl::param_const_iterator I = Function->param_begin(), E = Function->param_end();
                     I != E; ++I) {
                    string name = (*I)->getNameAsString();
                    params.push_back(name);
                }
                se->setParams(params);
                if (incidentType.compare("RETURN") == 0) {
                    se->loadParameters(functionName);
                    se->loadParameterTypes(functionName);
                    se->setType("RETURN");
                } else {
                    se->loadFunctionParameters();
                    se->loadFunctionParameterTypes();
                    if (root.compare("1"))
                        se->setType("FUNCTION");
                }

                const auto cfg = clang::CFG::buildCFG(Function,
                                                      Function->getBody(),
                                                      Result.Context,
                                                      clang::CFG::BuildOptions());
                CFGHandler cfgHandler(cfg);

                cfgHandler.findIncidents();
                cfgHandler.collectConstraints();
            }
        }
    }
};

class MyConsumer : public clang::ASTConsumer {
public:
    explicit MyConsumer() : handler(), varWriteCallBack(), cfgHandler(nullptr) {
        if (incidentType.compare("VARINFFUNC") == 0 || incidentType.compare("VARINFUNCEXTEND") == 0) {
            const auto matching_node = clang::ast_matchers::ifStmt().bind("if");
            match_finder.addMatcher(matching_node, &handler);
        } else if (incidentType.compare("VARWRITE") == 0) {
            const auto matching_node = clang::ast_matchers::functionDecl().bind("fn");
            match_finder.addMatcher(matching_node, &varWriteCallBack);
        } else {
            const auto matching_node = clang::ast_matchers::functionDecl(
                    clang::ast_matchers::hasName(functionName)).bind("fn");
            match_finder.addMatcher(matching_node, &handler);
        }
    }

    void HandleTranslationUnit(clang::ASTContext& ctx) {
        match_finder.matchAST(ctx);
        if (incidentType.compare("VARWRITE") == 0)
            this->cfgHandler.writeVarWritePaths();
    }
private:
    MyCallback handler;
    VarWriteCallBack varWriteCallBack;
    CFGHandler cfgHandler;
    clang::ast_matchers::MatchFinder match_finder;
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr <clang::ASTConsumer>
    virtual CreateASTConsumer(clang::CompilerInstance& CI, llvm::StringRef) override {
        // Following two lines are added in order to not rising error while not finding #include location
        clang::Preprocessor &pp = CI.getPreprocessor();
        pp.SetSuppressIncludeNotFoundError(true);
        clang::DiagnosticsEngine& de = pp.getDiagnostics();
        de.setErrorLimit(1000);
        de.setSuppressAllDiagnostics(true);
        return std::make_unique<MyConsumer>();
    }
};

int executeAction(int argc, const char** argv) {

    auto CFGCategory = llvm::cl::OptionCategory("CFG");
    clang::tooling::CommonOptionsParser OptionsParser(argc, argv, CFGCategory);
    clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    return Tool.run(clang::tooling::newFrontendActionFactory<MyFrontendAction>().get());
}

SymbolTable *SymbolTable::instance = 0;
Context *Context::instance = 0;

int main(int argc, const char **argv) {

    filePath = *(&argv[1]);
    functionName = *(&argv[3]);
    incident = *(&argv[4]);
    incidentType = *(&argv[5]);
    root = *(&argv[6]);

    if (argc == 8) {
        varInFuncCount = *(&argv[7]);
    }

    Context* context = context->getInstance();
    SymbolTable* se = se->getInstance();

    se->loadState();
    se->setFunctionName(functionName);
    context->setCurrentFunction(functionName);
    context->setIncident(createIncident());

//    string cwd = string(fs::current_path());
    string cwd = "/usr/local/Cellar/llvm/11.0.0/cfg2";

    context->setFilePath( cwd + "/" + filePath);

    executeAction(argc, argv);

    se->saveState();

    return 0;
}
