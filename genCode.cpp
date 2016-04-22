#include "genCode.h"

extern int globalOffset;
extern int localOffset;
SymbolTable table;
bool bReturnComplete = false;
bool isReturn = false;
FILE * code;
int compoundSize = 0;
int compC = 2;

void generateCode(char * inFile, char * outFile, TreeNode * root, SymbolTable symbolTable) {
	//printf("%s\n", outFile == NULL ? "null" : outFile);

	if (outFile == NULL) {
		string s(inFile);
		s = s.substr(0, s.length()-2);
		s += "tm";
		outFile = (char*)s.c_str();
	} else if (strcmp(outFile, "-") == 0) {
		outFile = NULL;
	}

	table = symbolTable; //need symbol table to get our mem stuff and func/call lookups
	if (outFile == NULL) {
		code = stdout;
	} else code = fopen(outFile, "w");

	emitHeader(inFile); //output header
	emitIOLibrary(root); //output IO functions
	TreeNode * t = root;
	for (int i=0; i<7; i++) t = t->sibling;
	emitTree(t); //Skip past IO functions and code gen the rest of the tree
	emitInit(root);	//output init stuff
}

void emitHeader(char * inFile) {
	emitComment("C- compiler version C-F15");
	emitComment("Built: December 1, 2015");
	emitComment("Author: Morgan N. Holbart");
	emitComment("File compiled:  " + (string)inFile);
	emitSkip(1);
}

void emitTree(TreeNode * t) {
	if (t == NULL) return; //easier than null checking everywhere else
	switch(t->nodekind) { 
		case ExpK: emitExp(t); break;
		case DeclK: emitDecl(t); break;
		case StmtK: emitStmt(t); break;
	}
	emitTree(t->sibling);
}

void emitInit(TreeNode * t) {
	TreeNode * main = (TreeNode*)table.lookup("main");
	if (main == NULL) {
		printf("Symbol Table lookup failed\n");
		return;
	}	 
	int restore = emitSkip(0);
	emitBackup(0);
	emitRM("LDA", PC, restore - 1, PC, "Jump to init [backpatch]"); 
	emitSkip(restore - 1);
	emitComment("INIT");
	emitRM("LD", GP, 0, 0, "Set the global pointer"); 
	emitRM("LDA", FP, globalOffset, GP, "set first frame at end of globals");
	emitRM("ST", FP, 0, FP, "store old fp (point to self)");
	emitComment("INIT GLOBALS AND STATICS"); 
	emitGlobalsAndStatics(t); 
	emitComment("END INIT GLOBALS AND STATICS"); 
	emitRM("LDA", AC, FP, PC, "Return address in ac"); 
	emitRM("LDA", PC, main->emitLoc - emitSkip(0), PC, "Jump to main"); 
	emitRO("HALT", 0,0,0, "DONE!");
	emitComment("END INIT");
}

int paramCount = 0;
bool lhArray = false;
void emitExp(TreeNode * t) {
	//register to use
	int r;
	string name;
	//Use LDA, use LD if the call param is a func param
	string loadType = "LDA";
	//Check if its a func param by checking memSize
	if (t->memSize == 1) loadType = "LD";

	//Check if param and print a comment for param index
	if (t->isParam) {
		int paramIndex = paramCount + 1; //+1 to start at 1 instead of 0
		//http://stackoverflow.com/questions/9655202/how-to-convert-integer-to-string-in-c
		//got method here
		char pi[10];
		sprintf(pi, "%i", paramIndex); 
		emitComment("\t\t\tLoad param " + (string)pi);
	}

	TreeNode * LHS = t->child[0];
	TreeNode * RHS = t->child[1];

	switch (t->kind.exp) {
		case AssignK: 
			emitComment("EXPRESSION");

			//If its an array thats indexed save index first
			lhArray = false;
			if (t->child[0]->isArray && t->child[0]->child[0] != NULL) {
				emitTree(t->child[0]->child[0]);
				emitRM("ST", AC, compoundSize, FP, "Save index");
			}
			lhArray = false;
			emitTree(t->child[1]);
			lhArray = true;
			emitTree(t->child[0]);

		break;
		case OpK: 
			name = t->attr.name;
			emitTree(t->child[0]);
			if (RHS != NULL) {
				emitRM("ST", 3, 0, FP, "Save left side");
				emitTree(t->child[1]);
				emitRM("LD", AC1, 0, FP, "Load left into ac1");
				if (name == "+") 
					emitRM("ADD", AC, AC1, AC, "Op +");
				else if (name == "-") 
					emitRM("SUB", AC, AC1, AC, "Op -");
				else if (name == "/") 
					emitRM("DIV", AC, AC1, AC, "Op /");
				else if (name == "*") 
					emitRM("MUL", AC, AC1, AC, "Op *");
				else if (name == "|") 
					emitRM("OR", AC, AC1, AC, "Op OR");
				else if (name == "&") 
					emitRM("AND", AC, AC1, AC, "Op AND");
				else if (name == "<") 
					emitRM("TLT", AC, AC1, AC, "Op <");
				else if (name == "<=") 
					emitRM("TLE", AC, AC1, AC, "Op <=");
				else if (name == ">") 
					emitRM("TGT", AC, AC1, AC, "Op >");
				else if (name == ">=") 
					emitRM("TGE", AC, AC1, AC, "Op >=");
				else if (name == "!=") 
					emitRM("TNE", AC, AC1, AC, "Op !=");
				else if (name == "==")
					emitRM("TEQ", AC, AC1, AC, "Op ==");
				else if (name == "%") {

				}
			} else {
				if (name == "*") {
					emitRM("LD", AC, 1, AC, "Load array size");
				} else if (name == "?") {
					emitRM("RND", AC, AC, 6, "Op ?");
				} else if (name == "!") {
					emitRM("LDC", AC1, 1, 6, "Load 1");
					emitRM("XOR", AC, AC, AC1, "Op NOT");
				} else if (name == "-") {
					emitRM("LDC", AC1, 0, 6, "Load 0");
					emitRM("SUB", AC, AC1, AC, "Up unary -");
				}
			}

			//emitChildren(t);
			//Special case for indexed array with a *
			//if (strcmp(t->attr.name, "*") == 0) {
			//	emitRM("LD", AC, 1, AC, "Load array size");
			//}
		break;
		case ConstK: 
			emitConstant(t);
		break;
		case IdK: 
			name = t->attr.name;

			//Set register to use
			if (t->isStatic || t->isGlobal)
				r = 0;
			else r = 1;

			if (t->isParam || isReturn) {
				if (t->isArray) {
					if (t->child[0] != NULL) emitTree(t->child[0]);
					emitRM(loadType, t->child[0] != NULL ? AC1 : AC, t->memOffset, r, 
						"Load address of base of array " + name);
					if (t->child[0] != NULL) {	
						emitRO("SUB", AC, AC1, AC, "Compute offset of value");
						emitRM("LD", AC, 0, AC, "Load the value");
					}
				} else emitRM(loadType, AC, t->memOffset, r, "Load variable " + name);
				//Store parameters and increment counter
				if (t->isParam) {
					emitRM("ST", AC,compoundSize-2-paramCount + compC, FP, "Store parameter ");
					paramCount++;	
				}
				return;
			} 
			//Storing
			if (lhArray && t->isArray) {	
				if (t->child[0] != NULL) 
					emitRM("LD", AC1, compoundSize, FP, "Restore index ");
				emitRM(loadType, t->child[0] != NULL ? AC2 : AC, t->memOffset, r, 
					"Load address of base of array " + name);
				if (t->child[0] != NULL) {
					emitRO("SUB", AC2, AC2, AC1, "Compute offset of value ");
					emitRM("ST", AC, 0, AC2, "Store variable " + name);
				}
			} else if (lhArray && !t->isArray) 
				emitRM("ST", AC, t->memOffset, r, "Store variable " + name);
			 
			//Loading
			if (!lhArray && t->isArray) {
				if (t->child[0] != NULL) emitTree(t->child[0]);
				emitRM(loadType, t->child[0] != NULL ? AC1 : AC, t->memOffset, r
						, "Load address of base of array " + name);
				if (t->child[0] != NULL) {
					emitRO("SUB", AC, AC1, AC, "Compute offset of value");
					emitRM("LD", AC, 0, AC, "Load the value");
				}
			} else if (!lhArray && !t->isArray) 
				emitRM("LD", AC, t->memOffset, r, "Load variable " + name);
			

		break;
		case CallK:
			//Lookup the function
			TreeNode * lookup = (TreeNode*)table.lookup(t->attr.name);
			if (lookup == NULL) {
				printf("Symbol table lookup failed\n");
				return;
			}
			
			int comps = compoundSize;
			compC -= 2;
			emitComment("EXPRESSION");
			name = t->attr.name;
			emitComment("\t\t\tBegin call to " + name);
			emitRM("ST", FP, comps + compC, FP, "Store old fp in ghost frame");
			
			paramCount = 0;
			int parc = paramCount;
			emitTree(t->child[0]);
			paramCount = parc;
			compoundSize = comps;
			emitComment("\t\t\tJump to " + name);
			emitRM("LDA", FP, compoundSize + compC, FP, "Load address of new frame");
			emitRM("LDA", AC, FP, PC, "Return address in ac");
			emitRM("LDA", PC, lookup->emitLoc - emitSkip(0), PC, "CALL " + name);
			emitRM("LDA", AC, 0, RT, "Save the result in ac");
			emitComment("\t\t\tEnd call to " + name);
			compC += 2;
		break;
	}

	//Store parameter and increment param counter
	if (t->isParam) {
		emitRM("ST", AC, compoundSize - 2 - paramCount + compC, FP, "Store parameter");
		paramCount++;	
	}
}

int ifLocation, ifLocation2;
int whileLocation, whileLocation2;
void emitStmt(TreeNode * t) {
	switch(t->kind.stmt) {
		case IfK:
			emitComment("IF"); 
			//emitChildren(t);
			emitTree(t->child[0]);
			ifLocation = emitSkip(1);
			emitComment("THEN");
			emitComment("EXPRESSION");
			emitTree(t->child[1]);
			ifLocation2 = emitSkip(0);
			emitBackup(ifLocation);
			if (t->child[1] != NULL) {
				emitComment("ELSE");
				emitComment("EXPRESSION");
				ifLocation = emitSkip(1);
				emitTree(t->child[2]);
				ifLocation2 = emitSkip(0);
				emitBackup(ifLocation);
				emitRM("LDA", PC, ifLocation2 - emitSkip(0) - 1, PC,
					 "Jump around the ELSE [backpatch]");
				emitRestore();
			}
			emitComment("ENDIF");			
		break;
		case WhileK:
			whileLocation = emitSkip(0);
			emitComment("WHILE");
			emitTree(t->child[0]);
			//emitChildren(t);	
			emitRM("JNZ", AC, 1, PC, "Jump to while part");
			whileLocation2 = emitSkip(1);
			emitComment("DO");
			emitTree(t->child[1]);
			emitRM("LDA", PC, whileLocation - emitSkip(0) - 1, PC, "go to beginning of loop");
			whileLocation = emitSkip(0);
			emitBackup(whileLocation2);
			emitRM("LDA", PC, whileLocation - whileLocation2 - 1, PC, "Jump past loop [backpatch]");
			emitRestore();
			emitComment("ENDWHILE");
		break;
		case ForeachK:
			emitComment("FOR");
			emitChildren(t);
			emitComment("ENDFOR");
		break;
		case BreakK:
			emitComment("BREAK");
			emitRM("LDA", PC, 0, AC, "Return");
			//emitChildren(t);
		break;
		case CompK:
			//Mem size already calculated
			compoundSize = t->memSize;
			emitComment("COMPOUND");
			emitChildren(t);
			emitComment("END COMPOUND");
		break;
		case ReturnK:
			emitComment("RETURN");
			isReturn = true;
			emitTree(t->child[0]);
			isReturn = false;
			if (t->child[0] != NULL)
				emitRM("LDA", RT, 0, AC, "copy result to rt register");
			emitRM("LD", AC, -1, FP, "Load return address");
			emitRM("LD", FP, 0, FP, "Adjust fp");
			emitRM("LDA", PC, 0, AC, "Return");
		break;
	}
}

void emitDecl(TreeNode * t) {
	string name;
	
	switch(t->kind.decl) {
		case VarK: 
			//Deal with these elsewhere
			if (t->isGlobal || t->isStatic)
				return;

			name = t->attr.name;	
			if (t->isArray) {
				emitRM("LDC", AC, t->memSize - 1, 6, "load size of array " + name);
				emitRM("ST", AC, t->memOffset + 1, FP, "save size of array " + name);
			} else if ( t->child[0] != NULL) {
				emitConstant(t->child[0]);
				emitRM("ST", AC, t->memOffset, FP, "Store variable " + name);
			}
			
		break;
		case FuncK: 
			//Save location
			t->emitLoc = emitSkip(0) - 1;
			name = t->attr.name;
			emitComment("FUNCTION " + name);
			emitRM("ST", AC, -1, FP, "Store return address.");
			emitChildren(t);

			//Need to check here for existing return statements and only add this if 
			//a return statement isnt found			
			emitComment("Add standard closing in case there is no return statement");
			emitRM("LDC", RT, 0, 6, "Set return value to 0");
			emitRM("LD", AC, -1, FP, "Load return address");
			emitRM("LD", FP, 0, FP, "Adjust fp");
			emitRM("LDA", PC, 0, AC, "Return");

			emitComment("END FUNCTION " + name);
		break;
		case ParamK:
			emitChildren(t);
		break;
	}
}

void emitConstant(TreeNode * t) {
	if (t == NULL) return;
	if (t->nodekind != ExpK) return;
	if (t->nodekind == ExpK && t->kind.exp != ConstK) return;

	string s;
	switch (t->type) {
		case Int: 
		case Bool:
			s = t->type == Int ? "integer" : "Boolean";
			emitRM("LDC", AC, t->attr.iValue, 6, "Load " + s + " constant");
		break;
		case Char:
			if (!t->isArray) emitRM("LDC", AC, t->attr.cValue, 6, "Load char constant");
		break;
	}
}

void emitGlobalsAndStatics(TreeNode * t) {
	if (t == NULL) return;
	//Only dealing with VarK that are global or static here
	if (t->nodekind == DeclK && t->kind.decl == VarK
	&& (t->isGlobal || t->isStatic)) {
		string name(t->attr.name);
		if (t->isArray) {
			emitRM("LDC", AC, t->memSize - 1, 6, "load size of array " + name);
			emitRM("ST", AC, t->memOffset + 1, 0, "save size of array " + name);
		} else if (t->child[0] != NULL) {
			emitConstant(t->child[0]);
			emitRM("ST", AC, t->memOffset, GP, "Store variable " + name);
		} 
	}
	//Traverse children and siblings
	for (int i =0; i<3; i++) emitGlobalsAndStatics(t->child[i]);
	emitGlobalsAndStatics(t->sibling);	
}

void emitChildren(TreeNode * t) {
	for (int i =0; i <3; i++) emitTree(t->child[i]);
}

void emitIOLibrary(TreeNode* root) {
	for (int i = 0; i < 4; i++) {
		root->emitLoc = emitSkip(0) - 1;
		emitComment("FUNCTION " + (string)root->attr.name);
		emitRM("ST", 3, -1, 1, "");
		if (i == 0) emitRO("IN", 2, 2, 2, "");
		else if (i == 1) emitRO("INB", 2, 2, 2, "");
		else if (i == 2) emitRO("INC", 2, 2, 2, "");
		else if (i == 3) emitRO("OUTNL", 3, 3, 3, "");
		emitRM("LD", 3, -1, 1, "");
		emitRM("LD", 1, 0, 1, "");
		emitRM("LDA", 7, 0, 3, "");
		emitComment("END FUNCTION " + (string)root->attr.name);

		if (i == 3) break;
		root = root->sibling;
		root->emitLoc = emitSkip(0) - 1;
	
		emitComment("FUNCTION " + (string)root->attr.name);	
		emitRM("ST", 3, -1, 1, "");
		emitRM("LD", 3, -2, 1, "");
		string s = "OUT";
		if (i == 1) s += "B";
		else if (i == 2) s += "C";
		emitRO(s, 3, 3, 3, "");
		emitRM("LDC", 2, 0, 6, "");
		emitRM("LD", 3, -1, 1, "");
		emitRM("LD", 1, 0, 1, "");
		emitRM("LDA", 7, 0, 3, "");
		emitComment("END FUNCTION " + (string)root->attr.name);

		root = root->sibling;
	}	
}
