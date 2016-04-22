
#ifndef globals_h
#define globals_h

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern int yylineno;


typedef enum {StmtK, ExpK, DeclK} NodeKind;
typedef enum {IfK, ForeachK, WhileK, CompK, ReturnK, BreakK} StmtKind;
typedef enum {OpK, ConstK, IdK, SimpleK, CallK, AssignK} ExpKind;
typedef enum {ParamK, VarK, FuncK} DeclKind;
typedef enum {PlusK, MinusK, MultK, DivK, ModK, TernaryK } OpKind;
typedef enum {Void, Int, Bool, Char, String, Error, Undefined, IntChar} DeclType;
typedef enum {NUMC, BOOLC, CHARC, STRINGC} ConstType;


typedef struct treeNode
{
	struct treeNode *child[3];
	struct treeNode *sibling;

	ConstType consttype;
	DeclType type;
	int lineno;
	NodeKind nodekind;
	union
	{
		DeclKind decl;
		StmtKind stmt;
		ExpKind exp;
	} kind;

	union
	{
		OpKind op;
		int iValue;
		unsigned char cValue;
		char *sValue;
		char *name;
	} attr;
	bool isGlobal;
	bool isStatic;
	bool isArray;
	int arrayLength;
	int memSize;
	int memOffset;

	//code gen stuff
	int emitLoc;
	bool isParam;
} TreeNode;

#endif
