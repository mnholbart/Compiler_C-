
#ifndef util_h
#define util_h

#include <string>
#include "globals.h"
#include "symbolTable.h"

//Struct to contain error information
//They will be stored in an error buffer and printed at the end
struct error {
	int lineno;
	char * msg;
};
void printToken(char *tokenString);
TreeNode *newStmtNode(StmtKind kind);
TreeNode *newExpNode(ExpKind kind);
TreeNode *newDeclNode(DeclKind kind);
void printTree (TreeNode * tree, int spaces, int siblingIndex);
void printAnTree (TreeNode * tree, int spaces, int siblingIndex, bool printMem);
void printSpaces(int spaces);
void printSymbolTableNode(void * data);
void printNode(TreeNode * t);
void printANode(TreeNode * t, bool printMem);
SymbolTable GetSymbolTable();
void printDeclNode(TreeNode * t);
void printStmtNode(TreeNode * t);
void printExpNode(TreeNode * t);
void scopeAndType(TreeNode *t);
void printError(int i, int lineno, char *str1, char *str2, char *str3, double duble, int linenoTarget);
void printSymbolTable();
TreeNode * CreateIOFunctions(TreeNode * root); 
char * typeToCharP(DeclType t);
void getExpectedTypes(const char* s, bool isBinary, bool &singleSidedErrors, DeclType &lhs, DeclType &rhs, DeclType &rt);
void startScopeAndType(TreeNode * t, int &numerrors, int &numwarnings);
void scopeTypeChildren(TreeNode * t);
void scopeTypeChildrenNoSibling(TreeNode * t);
void declNode(TreeNode * t);
void stmtNode(TreeNode * t);
void expNode(TreeNode * t);
void printErrorBuffer();
void processChildren(TreeNode * t);

//SymbolTable symbolTable;
//DeclType functionReturnType;
//bool newScope = true;

#endif


