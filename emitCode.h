#ifndef EMIT_CODE_H__
#define EMIT_CODE_H__

#include <string>

using namespace std;

//
//  Special register defines for optional use in calling the 
//  routines below.
//

#define GP   0	//  The global pointer
#define FP   1	//  The local frame pointer
#define RT   2	//  Return value
#define AC   3  //  Accumulator
#define AC1  4  //  Accumulator
#define AC2  5  //  Accumulator
#define AC3  6  //  Accumulator
#define PC   7	//  The program counter

//
//  No comment please...
//

#define NO_COMMENT (char *)""


//
//  We always trace the code
//
#define TraceCode   1

//
//  The following functions were borrowed from Tiny compiler code generator
//
int emitSkip(int howMany);
void emitBackup(int loc);

//void emitComment(char *c);
void emitComment(char *c, char *cc);
void emitComment(string c);
void emitRestore();
void emitGoto(int d, int s, char *c);
void emitGoto(int d, int s, char *c, char *cc);
void emitGotoAbs(int a, char *c);
void emitGotoAbs(int a, char *c, char *cc);

//void emitRM(char *op, int r, int d, int s, char *c);
void emitRM(char *op, int r, int d, int s, char *c, char *cc);
void emitRM(string op, int r, int d, int s, string c);

void emitRMAbs(char *op, int r, int a, char *c);
void emitRMAbs(char *op, int r, int a, char *c, char *cc);

//void emitRO(char *op, int r, int s, int t, char *c);
void emitRO(char *op, int r, int s, int t, char *c, char *cc);
void emitRO(string op, int r, int s, int t, string c);

void backPatchAJumpToHere(int addr, char *comment);
void backPatchAJumpToHere(char *cmd, int reg, int addr, char *comment);

void emitLit(char *s);

#endif
