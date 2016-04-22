#ifndef GENCODE_H
#define GENCODE_H

#include <string.h>
#include <cstring>
#include "globals.h"
#include "emitCode.h"
#include "symbolTable.h"

void generateCode(char * inFile, char * outFile, TreeNode * root, SymbolTable symbolTable);
void emitHeader(char * inFile);
void emitIOLibrary(TreeNode * root);
void emitTree(TreeNode * t);
void emitDecl(TreeNode * t);
void emitStmt(TreeNode * t);
void emitExp(TreeNode * t);
void emitInit(TreeNode * t);
void emitGlobalsAndStatics(TreeNode * t);
void emitChildren(TreeNode * t);
void emitConstant(TreeNode * t);

#endif
