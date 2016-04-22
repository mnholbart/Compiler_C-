#include <algorithm>
#include <vector>
#include <iostream>
#include "util.h"

//Error handling
char charBuffer[256]; //Buffer to prevent anything from being lost by not being flushed
int numberErrors = 0; //Number of errors found
int numberWarnings = 0; //Number warnings found
std::vector<error> errorBuffer; //Our error buffer to print at the end instead of as we go

//Global flags for loops and scopes
bool enterScope = true; //Used by FuncK and CompK so we know when a CompK is a new scope
bool foundReturn = false; //Was a return ever found during the current FuncK
bool inLoop = false; //Check if we are in a foreach or while loop
int loopDepth = 1; //What depth of loop we are currently in

//Global function stuff
TreeNode * currFunction = NULL; //Current function we are in to check return types and stuff

//Our symbol table
SymbolTable symbolTable;

//Memory
int localOffset = 0;
int globalOffset = 0;
int numParams = 0;

//Function called by c-.y that starts the scope and type processChildren
//Also returns the number of errors and warnings
void startScopeAndType(TreeNode * t, int &numerrors, int &numwarnings) {
	//Scope and type the root node
	scopeTypeChildren(t);

	//Check if main function exists
	TreeNode * mainLookup = (TreeNode*)symbolTable.lookup("main");
	if (mainLookup == NULL) printError(36, 0, NULL, NULL, NULL, 0, 0);
	
	//Print all our buffer errors and set the referenced error numbers
	printErrorBuffer();
	
	numerrors += numberErrors;
	numwarnings += numberWarnings;
}

//Split the scope and type based on what type of node it is 
void scopeTypeChildren(TreeNode * t) {
	//Check for NULL here so we don't have to do it throughout the scope and type functions
	if (t == NULL) return;
//	t->memSize = 0; t->memOffset = 0;
	switch (t->nodekind) {
		case DeclK: declNode(t); break;
		case StmtK: stmtNode(t); break;
		case ExpK: expNode(t); break;
	}
	scopeTypeChildren(t->sibling);
}

void scopeTypeChildrenNoSibling(TreeNode * t) {
	//Check for NULL here so we don't have to do it throughout the scope and type functions
	if (t == NULL) return;
//	t->memSize = 0; t->memOffset = 0;
	switch (t->nodekind) {
		case DeclK: declNode(t); break;
		case StmtK: stmtNode(t); break;
		case ExpK: expNode(t); break;
	}
//	scopeTypeChildren(t->sibling);
}

//Scope and type function for statement nodes
void stmtNode(TreeNode * t) {
	//Bools to check which if any child throws an error, defaulted to false
	//Anytime we need to check a child for error need to check ciThrowErr 
	bool c0throwErr, c1throwErr, c2throwErr;
	c0throwErr = c1throwErr = c2throwErr = false;
	
	bool child0IsArray, child1IsArray;
	child0IsArray = child1IsArray = false;
	
	//Used to check if we have changed depth for the loop, set to true and update depth here
	//Recheck the depth in the While and Foreach switch statements
	if (t->kind.stmt == ForeachK || t->kind.stmt == WhileK) {
		if (!inLoop) 
			loopDepth = symbolTable.depth();
		inLoop = true;
	}	
	
	//Check cithrowErr for all 3 children
	if (t->kind.stmt != CompK) {
		scopeTypeChildren(t->child[0]);
		if (t->child[0] != NULL && t->child[0]->type == Void 
			&& !(t->child[0]->nodekind == ExpK && t->child[0]->kind.exp == CallK))
			c0throwErr = true;
		scopeTypeChildren(t->child[1]);
		if (t->child[1] != NULL && t->child[1]->type == Void 
			&& !(t->child[1]->nodekind == ExpK && t->child[1]->kind.exp == CallK))
			c1throwErr = true;
		scopeTypeChildren(t->child[2]);
		if (t->child[2] != NULL && t->child[2]->type == Void 
			&& !(t->child[2]->nodekind == ExpK && t->child[2]->kind.exp == CallK))
			c2throwErr = true;

		if (t->child[0] != NULL && t->child[0]->isArray) child0IsArray = true;
		if (t->child[1] != NULL && t->child[1]->isArray) child1IsArray = true;
	}

	//c0throwErr = c1throwErr = c2throwErr = false;

	//Get all the children now
	TreeNode* child0 = t->child[0];
	TreeNode* child1 = t->child[1];

	//child0IsArray = !(child0 != NULL && child0->child[0] != NULL);
	//child1IsArray = !(child1 != NULL && child1->child[0] != NULL);
	if (child0 != NULL && child0->child[0] != NULL) child0IsArray = false;
	if (child1 != NULL && child1->child[0] != NULL) child1IsArray = false;

	switch (t->kind.stmt) {
		case IfK:
			//Error 10 Expecting Boolean test condition in %s statement but got type %s.
			//If child 0 isn't a bool like we are expecting
			if (!c0throwErr && child0->type != Bool) printError(10, t->lineno, t->attr.name,
				typeToCharP(child0->type), NULL, 0, 0);
				
			//Error 9 Cannot use array as test condition in %s statement.
			//Can't use an array 
			if (child0IsArray) printError(9, t->lineno, t->attr.name, NULL, NULL, 0,0);
		break;
		case ForeachK:
			//Error 30 In foreach statement the variable to the left of 'in' must not be an array.
			//If child0 is an array which it can't be
			if (!c0throwErr && child0IsArray) 
				printError(30, t->lineno, NULL, NULL, NULL, 0,0);
		
			//Error 29 If not an array, foreach requires rhs of 'in' be of type int but it is type %s.
			//RHS or child1 isn't type int
			if (!c1throwErr && !child1IsArray && child1->type != Int)
				printError(29, t->lineno, typeToCharP(child1->type),
				NULL, NULL, 0, 0);
				
			//Error 28 If not an array, foreach requires lhs of 'in' be of type int but it is type %s
			//LHS or child0 isn't type int
			if (!c0throwErr && !child1IsArray && child0->type != Int) 
				printError(28, t->lineno, typeToCharP(child0->type),
				NULL, NULL, 0, 0);
			
		
			//Error 27 Foreach requires operands of 'in' be the same type but lhs is type %s and rhs array is type %s.
			//RHS array of type X must match lhs type X
			if ((!c0throwErr || !c1throwErr) && child1IsArray
				&& child0->type != child1->type) printError(27, t->lineno,
				typeToCharP(child0->type), typeToCharP(child1->type), NULL, 0,0);
				
			//If we haven't gone up in depth, we are now leaving the loop, if we reenter a loop
			//It will be set again at the start of stmtNode();
			if (loopDepth == symbolTable.depth()) inLoop = false;
		break;
		case WhileK:
			//Error 10 Expecting Boolean test condition in %s statement but got type %s
			//Expected boolean and didn't get one
			if (!c0throwErr && child0->type != Bool) printError(10, t->lineno, t->attr.name, 
				typeToCharP(child0->type), NULL, 0, 0);
			
			//Error 9 Cannot use array as test condition in %s statement.
			//Can't use an array 
			if (child0IsArray) printError(9, t->lineno, t->attr.name, NULL, NULL, 0,0);
			
			//If we haven't gone up in depth, we are now leaving the loop, if we reenter a loop
			//It will be set again at the start of stmtNode();
			if (loopDepth == symbolTable.depth()) inLoop = false;
		break;
		case CompK: {
			//Need to remember localOffset so we can go back
			int compoundSize = localOffset;
			bool newScope = enterScope;
			//FuncK will set enterScope to false so if we last had a function, ignore this compK
			if (newScope) symbolTable.enter("comp"); //Current scope is a new scope
			else enterScope = true; //Our next scope will be a new scope

			processChildren(t);
			if (newScope) symbolTable.leave();

			//Revert local offset as we leave scope
			t->memSize = localOffset;
			localOffset = compoundSize;
		}
		break;
		case ReturnK:
			processChildren(t);	
			//If we are returning something
			if (t->child[0] != NULL) {

				//printf("%s %s  %s %s \n", currFunction->attr.name, typeToCharP(currFunction->type) , t->attr.name, typeToCharP(t->child[0]->type));
				//
				if (currFunction == NULL) break;
					
				//Error 13 Function '%s' at line %d is expecting to return type %s but got %s
				//If function isnt void, child isnt void (returning something) and the type being returned
				//doesn't match the function return type 
				if (currFunction->type != Void && t->child[0]->type != Void 
				&& currFunction->type != t->child[0]->type) 
					printError(13, t->lineno,
					currFunction->attr.name, typeToCharP(currFunction->type), 
					typeToCharP(t->child[0]->type), 0, currFunction->lineno);
				
				//Error 12 Function '%s' at line %d is expecting no return value, but return has return value.
				//Since child 0 isnt null, we are returning something and shouldn't be
				if (currFunction->type == Void) printError(12, t->lineno, 
					currFunction->attr.name, NULL, NULL, 0, currFunction->lineno);
				

			}
			//Error 11 Cannot return an array.
			//Trying to return an array 
			else if (t->isArray) printError(11, t->lineno, NULL, NULL, NULL, 0, 0);
			//Error 14 Function '%s' at line %d is expecting to return type %s but return has no return value.
			//If our function expects a return of type X, and we aren't returning anything
			if (currFunction->type != Void && t->child[0] == NULL) printError(14, t->lineno,
				currFunction->attr.name, typeToCharP(currFunction->type), NULL, 0, 
				currFunction->lineno);
				
			foundReturn = true;
		break;
		case BreakK:
			//Error 16 Cannot have a break statement outside of loop.
			//If we aren't in a while or foreach loop then throw error 
			if (!inLoop) 
				printError(16, t->lineno, NULL, NULL, NULL, 0, 0);
		break;
	}
}

//Function to handle expression nodes
void expNode(TreeNode * t) {
	//isString isArray isUnary isIndexed all self explanatory
	bool isLHSString, isRHSString, isUnary, isLHSArray, isRHSArray, isLHSIndexed, isRHSIndexed;
	isLHSString = isRHSString = isUnary = isLHSArray = isRHSArray = isLHSIndexed = isRHSIndexed = false;

	//types belong to our lhs rhs return, what the expected lhs rhs types are based on getExpectedTypes()
	DeclType LHS, RHS, retType, expectedLHS, expectedRHS;
	LHS = RHS = retType = expectedLHS = expectedRHS = retType = Undefined;

	TreeNode * found = NULL;
	TreeNode * lhNode = NULL;
	TreeNode * rhNode = NULL;

	switch(t->kind.exp) {
		case AssignK: //Fallthrough to OpK
		case OpK: 
			processChildren(t);
			
			//Get information for the child 0
			if (t->child[0] != NULL) {
				lhNode = t->child[0]; //lhNode is the child 0
				LHS = lhNode->type; //LH type is child 0 type
				isLHSArray = lhNode->isArray; //LHSArray is the child 0 isArray
				
				//If child 0 has a child then its indexed and no longer an array
				if (lhNode->child[0] != NULL) {
					isLHSArray = false;
					isLHSIndexed = true;
				}
				
				if (lhNode->nodekind == ExpK && lhNode->kind.exp == CallK) 
					isLHSArray = false;
				if (lhNode->nodekind == ExpK && lhNode->kind.exp == ConstK) 
					isLHSString = true;
				
				isUnary = true;
			}

			//Get information for child 1
			if (t->child[1] != NULL) {
				rhNode = t->child[1];  //rhNode is child 1
				RHS = rhNode->type; //RH type is child 1 type
				isRHSArray = rhNode->isArray; //RHArray is child 1 isArray
				
				//If child 1 h as a child its indexed and no longer an array
				if (rhNode->child[0] != NULL) {
					isRHSArray = false;
					isRHSIndexed = true;
				}
				
				if (rhNode->nodekind == ExpK && rhNode->kind.exp == CallK) 
					isRHSArray = false;
				if (rhNode->nodekind == ExpK && rhNode->kind.exp == ConstK) 
					isRHSString = true;
				
				isUnary = false;
			}

			//left error right error error handling stuff
			bool leftErr, rightErr, unaryError;
			leftErr = rightErr = unaryError = false;

			//Checks if LHS or RHS is undeclared, so leftErr rightErr = leftDeclared rightDeclared
			if (LHS == Void && !(lhNode->nodekind == ExpK && lhNode->kind.exp == CallK)) 
				leftErr = true;
			if (RHS == Void && !(rhNode->nodekind == ExpK && rhNode->kind.exp == CallK)) 
				rightErr = true;
			
			getExpectedTypes(t->attr.name, isUnary, unaryError, expectedLHS, expectedRHS, retType);
			//printf("%s %d \n", t->attr.name, t->lineno);

			//If unary expression without errors
			if (isUnary && !leftErr) {
				//Error 8 Unary '%s' requires an operand of type %s but was given %s.
				//If our type doesn't match the expected type for the Op
				if (LHS != expectedLHS && expectedLHS != Undefined) printError(8, t->lineno,
					t->attr.name, typeToCharP(expectedLHS), typeToCharP(LHS), 0, 0);
				
				//Error 6 The operation '%s' does not work with arrays.
				//Special case for * to check for arrays, here we dont want an array
				if (isLHSArray && strcmp(t->attr.name, "*") != 0) printError(6, t->lineno, 
					t->attr.name, NULL, NULL, 0,0);
					
				//Error 7 The operation '%s' only works with arrays.
				//Special case for * to check for arrays, here we want an array
				else if (!isLHSArray && strcmp(t->attr.name, "*") == 0) printError(7, t->lineno, 
					t->attr.name, NULL, NULL, 0,0);
				 
			} 
			//If binary expression
			else {
				//Error 2 '%s' requires operands of the same type but lhs is type %s and rhs is %s.
				//If our LHS and RHS don't match 
				if (!unaryError)
					if (LHS != RHS && !leftErr && !rightErr) printError(2, t->lineno, t->attr.name, 
						typeToCharP(LHS), typeToCharP(RHS), 0, 0);

				//Make sure we have an expected RHS and LHS type (not undefined)
				if (!(expectedLHS == Undefined || expectedRHS == Undefined)) {
					//special case > < >= <= to handle alphabetical or numerical
					if (expectedLHS == IntChar || expectedRHS == IntChar) {
						//Error 38 '%s' requires operands of type char or int but lhs is of type %s
						//Check that LHS is int or char else error
						if (!leftErr && LHS != Int && LHS != Char) printError(38, t->lineno, t->attr.name,
							typeToCharP(LHS), NULL, 0, 0);
					
						//Error 37 '%s' requires operands of type char or int but rhs is of type %s
						//Check that RHS is int or char else error
						if (!rightErr && RHS != Int && RHS != Char) printError(37, t->lineno, t->attr.name,
							typeToCharP(RHS), NULL, 0, 0);

					} else {

						//printf("%s %d \n", t->attr.name, t->lineno);
						//Error 3 '%s' requires operands of type %s but lhs is of type %s.
						//Check that LHS is the correct type
						if (!leftErr &&  LHS != expectedLHS) printError(3, t->lineno, 
							t->attr.name,typeToCharP(expectedLHS), 
							typeToCharP(LHS), 0, 0);
							
						//Error 4 '%s' requires operands of type %s but rhs is of type %s.
						//Check that RHS is the correct type
						if (!rightErr && RHS != expectedRHS) printError(4, t->lineno, 
							t->attr.name,typeToCharP(expectedRHS), 
							typeToCharP(RHS), 0, 0);
					} 
				}
				
				if (isLHSArray || isRHSArray) {
					//Error 6 The operation '%s' does not work with arrays
					//Undefined incompatible type with the arrays
					if (expectedLHS != Undefined)
						printError(6, t->lineno, t->attr.name, NULL, NULL, 0, 0);
					//Error 5 '%s' requires that if one operand is an array so must the other operand.
					//If one is an array and the other is not then error
					else if ((isLHSArray && !isRHSArray) || (!isLHSArray && isRHSArray)) 
						printError(5, t->lineno, t->attr.name, NULL, NULL, 0, 0);
				}
			}

			if (retType == Undefined)
				t->type = LHS;
			else t->type = retType;
		break;
		case ConstK:
			//Just process children
			processChildren(t);
		break;
		case IdK:
			//Since we are using an ID, make sure that it exists somewhere
			found = (TreeNode*)symbolTable.lookup(t->attr.name);
			//Error 1 Symbol '%s' is not defined.
			//Didn't find the ID anywhere 
			if (found == NULL) {
				printError(1, t->lineno, t->attr.name, NULL, NULL, 0, 0);
				break;
			}
			
			t->type = found->type;
			t->isArray = found->isArray;
			t->memSize = found->memSize;
			t->memOffset = found->memOffset;
			t->isGlobal = found->isGlobal;
			t->isStatic = found->isStatic;

			//Error 18 Cannot use function '%s' as a simple variable
			//This is IdK and we found a function
			if(found->kind.decl == FuncK) {
				printError(18, t->lineno, t->attr.name, NULL, NULL, 0,0);
				break;
			}

			if (t->child[0] == NULL)
				break;
				
			processChildren(t);
			
			if (t->child[0]->type == Void && !(t->child[0]->nodekind == ExpK 
				&& t->child[0]->kind.exp == CallK)) break;
			
			//Error 20 Array '%s' should be indexed by type int but got %s
			//Tried to index with a type other than int (child 0 must be int)
			if (t->isArray && t->child[0]->type != Int) 
				printError(20, t->lineno, t->attr.name, typeToCharP(t->child[0]->type), NULL, 0,0);
			
			//Error 19 Array index is the unindexed array '%s'.
			if (t->isArray && t->child[0]->isArray && t->child[0]->child[0] == NULL)
				printError(19, t->lineno, t->child[0]->attr.name, NULL, NULL, 0, 0);
		
			//Error 21 Cannot index nonarray '%s'.
			//If t is not an array
			if (!t->isArray) 
				 printError(21, t->lineno, t->attr.name, NULL, NULL, 0,0);
			
		break;
		case CallK:

		//	processChildren(t);
			//Lookup any existing functions
			found = (TreeNode*)symbolTable.lookup(t->attr.name);
		
			//Still need to run children if it isn't found	
			if (found == NULL) {
				 printError(1, t->lineno, t->attr.name, NULL, NULL, 0,0);
				 processChildren(t);
			}
			
			if (found != NULL) {
				t->type = found->type;
				t->memSize = found->memSize;
				t->memOffset = found->memOffset;

				//Error 17 '%s' is a simple variable and cannot be called.
				//If what we found is not a function and we are trying to Call it
				if (found->kind.decl != FuncK) printError(17, t->lineno, t->attr.name, NULL,
					NULL, 0, 0);
				
				//Parameters of the function call
				TreeNode * callParams = t->child[0];
				//Parameters of function declaration
				TreeNode * declParams = found->child[0];
				bool declArray = false; //Is Declaration param an array
				bool callArray = false; //Is call param an array
				int index = 1; //Index of the param we are comparing, used in error 23 and 24
	
				//Go through every param one at a time
				//Check that each param matches or throw an error
				//Once we reach the end of one param list, check that it is the end of the other 
				while (declParams != NULL && callParams != NULL) {
					//scope type without siblings because only want to run the one were on
					scopeTypeChildrenNoSibling(callParams);
					//scopeTypeChildren(callParams);
					//Skip void parameters
					callParams->isParam = true;
					if (callParams->kind.exp == IdK && callParams->type == Void) {
						declParams = declParams->sibling; //Traversal
						callParams = callParams->sibling;
						callArray = declArray = false; //Reset
						index++; //index inc
						continue;
					}
					
					//Check if declParam is an array
					if (declParams->isArray) {
						declArray = true; //Array
						if(declParams->child[0] != NULL) declArray = false; //Indexed, not array
					} else declArray = false; 
					//Check if callParam is array
					if (callParams->isArray) {
						callArray = true; //Array
						if (callParams->child[0] != NULL) callArray = false; //Indexed, not array
					} else callArray = false;
					//Error 22 Expecting type %s in parameter %d of call to '%s' defined on line %d but got %s.
					//Type mismatch between callParam and declParam
					if (declParams->type != callParams->type) printError(22, t->lineno,
						typeToCharP(declParams->type), t->attr.name, 
						typeToCharP(callParams->type), index, found->lineno);
						
					//Error 23 Expecting array in parameter %d of call to '%s' defined on line %d.
					//Didn't get an array where we need one
					if (declArray && !callArray) printError(23, t->lineno, t->attr.name, NULL,
						NULL, index, found->lineno);
						
					//Error 24 Not expecting array in parameter %d of call to '%s' defined on line %d
					//Got an array when we didn't need one
					if (!declArray && callArray) printError(24, t->lineno, t->attr.name, NULL,
						NULL, index, found->lineno);
				
					//Iterate to next param increment index
					callParams = callParams->sibling;
					declParams = declParams->sibling;
					index++;
				}

				bool tooFew = false;
				bool tooMany = false;				

				if (callParams == NULL && declParams != NULL)
					tooFew = true;
				else if (callParams != NULL && declParams == NULL)
					tooMany = true;
	
				//Error 25 Too few parameters passed for function '%s' defined on line %d.
				//We hit null on callParams but not declParams, so don't have enough params
				if (tooFew) printError(25, t->lineno, t->attr.name, NULL, NULL, 0, found->lineno);
				//if (callParams == NULL && declParams != NULL) printError(25, t->lineno,
				//	t->attr.name, NULL, NULL, 0, found->lineno);
				
				//Error 26 Too many parameters passed for function '%s' defined on line %d
				//We hit null on declParams but not callParams, so we have too many params
				if (tooMany) printError(26, t->lineno, t->attr.name, NULL, NULL, 0, found->lineno);
				//if (callParams != NULL && declParams == NULL) printError(26, t->lineno,
				//	t->attr.name, NULL, NULL, 0, found->lineno); 
			
				while (callParams != NULL) {
					//callParams = callParams->sibling;
					scopeTypeChildrenNoSibling(callParams);
					callParams = callParams->sibling;
				}
			}
		break;
	}
}

//Scope and type function for a declaration type of node
void declNode(TreeNode * t) {
	//At the start of declaration, check if there is already something declared beforehand
	TreeNode * prevInitialized;
	if (t->kind.decl != VarK && !symbolTable.insert(t->attr.name, t)) {
		prevInitialized = (TreeNode*)symbolTable.lookup(t->attr.name);
		printError(0, t->lineno, t->attr.name, NULL, NULL, 0, prevInitialized->lineno);
	}

	if (symbolTable.depth() == 1)
		t->isGlobal = true;
	else t->isGlobal = false;

	switch (t->kind.decl) {
		case ParamK:
			//Process parameters
			processChildren(t);
			//1 more param to the counter
			numParams--;
			//Default 1 size
			t->memSize = 1;
			//Update current offset and move to next offset
			t->memOffset = localOffset;
			localOffset--;
		break;
		case VarK:
			processChildren(t);

			if (t->child[0] != NULL) {
				//Check if we have an ID, because we need to lookup the previous declaration of it
				if (t->child[0]->nodekind == ExpK 
				&& (t->kind.exp == IdK && t->child[0]->kind.exp == CallK)) 
					prevInitialized = (TreeNode*)symbolTable.lookup(t->child[0]->attr.name);
				else prevInitialized = t->child[0];
			
				//Error 34 Initializer for variable '%s' is not a constant expression.
				if (prevInitialized->nodekind == ExpK 
				&& (prevInitialized->kind.exp == IdK || prevInitialized->kind.exp == CallK)) {
					printError(34, t->lineno, t->attr.name, NULL, NULL, 0, 0);	
				} else {
					//Error 31 Array '%s' must be of type char to be initialized, but is of type %s.
					//If we need an array, and it isn't a char then its invalid
					if (prevInitialized->isArray && t->isArray && t->type != Char) 
						printError (31, t->lineno, t->attr.name, 
							typeToCharP(t->type),NULL,0,0);
					
					//Error 35 Variable '%s' is of type %s but is being initialized with an expression of type %s.
					//If we have non matching types of declaration and initialization
					if (prevInitialized->isArray && !t->isArray && prevInitialized->type == Char 
						&& prevInitialized->type != t->type)
						printError(35, t->lineno, t->attr.name, typeToCharP(t->type),
							typeToCharP(prevInitialized->type), 0, 0);
								

					//Error 33 Initializer for nonarray variable '%s' of type %s cannot be initialized with an array
					//If we are trying to initialize a non array into a var that is an array
					if (prevInitialized->isArray && !t->isArray) 
						printError(33, t->lineno, t->attr.name, 
								typeToCharP(t->type),NULL,0,0); 
					
					//Error 32 Initializer for array variable '%s' must be a string, but is of nonarray type %s
					//If we weren't an array, and are trying to initiate with an array
					if (!prevInitialized->isArray && t->isArray) printError(32, t->lineno,
						 t->attr.name, typeToCharP(prevInitialized->type), NULL, 0, 0);
					 
					//Error 35 Variable '%s' is of type %s but is being initialized with an expression of type %s.
					//Same error as above, but for non Char
					if (!prevInitialized->isArray && !t->isArray
						&& prevInitialized->type != t->type) 
						printError(35, t->lineno, t->attr.name, typeToCharP(t->type), 
						typeToCharP(prevInitialized->type), 0, 0);
					
				}
			}

			//Error 0 Symbol '%s' is already defined at line %d
			//Check for variable redeclaration since we didn't do it above for VarK
			if (!symbolTable.insert(t->attr.name, t)) {
				TreeNode * exists = (TreeNode*)symbolTable.lookup(t->attr.name);	
				printError(0, t->lineno, t->attr.name, NULL, NULL, 0, exists->lineno);
			}

			//Memory = size of array + 1 to hold the length
			if (t->isArray) t->memSize = t->arrayLength + 1;
			else t->memSize = 1;

			//Global and static managed the same way
			//only difference is whether it uses local or global offsets
			if (t->isGlobal || t->isStatic) {
				if (t->isArray) t->memOffset = globalOffset - 1;
				else t->memOffset = globalOffset;
				globalOffset -= t->memSize;
			} else {
				if (t->isArray) t->memOffset = localOffset - 1;
				else t->memOffset = localOffset;
				localOffset -= t->memSize;
			}

		break;
		case FuncK:
			//Entering a new function 
			currFunction = t;
			symbolTable.enter(t->attr.name);

			//Sized of function is -2 + params
			//Params are handled in ParamK
			localOffset = -2;
	
			//Safety measure to prevent CompK node from registering a new scope
			enterScope = false;

			numParams = -2;
			processChildren(t);

			//printf("%s %s \n", t->attr.name, typeToCharP(t->type));
			//printf("%s %i\n", currFunction->attr.name, currFunction->lineno);

			//Error 15 Expecting to return type %s but function '%s' has no return statement
			//Check if we found a Return type node if the function expects one
			if (foundReturn) foundReturn = false;
			else if (t->type != Void && t->lineno >= 0) printError(15, t->lineno, typeToCharP(t->type),
				t->attr.name, NULL, 0,  0);
			
			//Leave the current function 
			symbolTable.leave();
			currFunction = NULL;

			//Update memSize with numParams
			t->memSize = numParams;;
			t->memOffset = 0;
		break;	
	}
}

//Scope and type all the children
//Null children will be handled in scopeTypeChildren()
void processChildren(TreeNode * t) {
	for (int i=0; i<3; i++) 
		scopeTypeChildren(t->child[i]);
}


//Print all the errors we found
void printErrorBuffer() {
	for (int i = 0; i<errorBuffer.size(); i++) {
		printf("%s", errorBuffer[i].msg);
	}
}

//Used to add an error to the error buffer
void printError(int i, int lineno, char *str1, char *str2, char *str3, double duble, int linenoTarget) {

	switch (i) {
	case  0: sprintf(charBuffer, "ERROR(%d): Symbol '%s' is already defined at line %d.\n", lineno, str1, linenoTarget); numberErrors++; break;
	case  1: sprintf(charBuffer, "ERROR(%d): Symbol '%s' is not defined.\n", lineno, str1); numberErrors++; break;
	case  2: sprintf(charBuffer, "ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n", lineno, str1, str2, str3); numberErrors++; break;
	case  3: sprintf(charBuffer, "ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n", lineno, str1, str2, str3); numberErrors++; break;
	case  4: sprintf(charBuffer, "ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n", lineno, str1, str2, str3); numberErrors++; break;
	case  5: sprintf(charBuffer, "ERROR(%d): '%s' requires that if one operand is an array so must the other operand.\n", lineno, str1); numberErrors++; break;
	case  6: sprintf(charBuffer, "ERROR(%d): The operation '%s' does not work with arrays.\n", lineno, str1); numberErrors++; break;
	case  7: sprintf(charBuffer, "ERROR(%d): The operation '%s' only works with arrays.\n", lineno, str1); numberErrors++; break;
	case  8: sprintf(charBuffer, "ERROR(%d): Unary '%s' requires an operand of type %s but was given %s.\n", lineno, str1, str2, str3); numberErrors++; break;
	case  9: sprintf(charBuffer, "ERROR(%d): Cannot use array as test condition in %s statement.\n", lineno, str1); numberErrors++; break;
	case 10: sprintf(charBuffer, "ERROR(%d): Expecting Boolean test condition in %s statement but got type %s.\n", lineno, str1, str2); numberErrors++; break;
	case 11: sprintf(charBuffer, "ERROR(%d): Cannot return an array.\n", lineno); numberErrors++; break;
	case 12: sprintf(charBuffer, "ERROR(%d): Function '%s' at line %d is expecting no return value, but return has return value.\n", lineno, str1, linenoTarget); numberErrors++; break;
	case 13: sprintf(charBuffer, "ERROR(%d): Function '%s' at line %d is expecting to return type %s but got %s.\n", lineno, str1, linenoTarget, str2, str3); numberErrors++; break;
	case 14: sprintf(charBuffer, "ERROR(%d): Function '%s' at line %d is expecting to return type %s but return has no return value.\n", lineno, str1, linenoTarget, str2); numberErrors++; break;
	case 15: sprintf(charBuffer, "WARNING(%d): Expecting to return type %s but function '%s' has no return statement.\n", lineno, str1, str2); numberWarnings++; break;
	case 16: sprintf(charBuffer, "ERROR(%d): Cannot have a break statement outside of loop.\n", lineno); numberErrors++; break;
	case 17: sprintf(charBuffer, "ERROR(%d): '%s' is a simple variable and cannot be called.\n", lineno, str1); numberErrors++; break;
	case 18: sprintf(charBuffer, "ERROR(%d): Cannot use function '%s' as a simple variable.\n", lineno, str1); numberErrors++; break;
	case 19: sprintf(charBuffer, "ERROR(%d): Array index is the unindexed array '%s'.\n", lineno, str1); numberErrors++; break;
	case 20: sprintf(charBuffer, "ERROR(%d): Array '%s' should be indexed by type int but got %s.\n", lineno, str1, str2); numberErrors++; break;
	case 21: sprintf(charBuffer, "ERROR(%d): Cannot index nonarray '%s'.\n", lineno, str1); numberErrors++; break;
	case 22: sprintf(charBuffer, "ERROR(%d): Expecting type %s in parameter %d of call to '%s' defined on line %d but got %s.\n", lineno, str1, (int)duble, str2, linenoTarget, str3); numberErrors++; break;
	case 23: sprintf(charBuffer, "ERROR(%d): Expecting array in parameter %d of call to '%s' defined on line %d.\n", lineno, (int)duble, str1, linenoTarget); numberErrors++; break;
	case 24: sprintf(charBuffer, "ERROR(%d): Not expecting array in parameter %d of call to '%s' defined on line %d.\n", lineno, (int)duble, str1, linenoTarget); numberErrors++; break;
	case 25: sprintf(charBuffer, "ERROR(%d): Too few parameters passed for function '%s' defined on line %d.\n", lineno, str1, linenoTarget); numberErrors++; break;
	case 26: sprintf(charBuffer, "ERROR(%d): Too many parameters passed for function '%s' defined on line %d.\n", lineno, str1, linenoTarget); numberErrors++; break;
	case 27: sprintf(charBuffer, "ERROR(%d): Foreach requires operands of 'in' be the same type but lhs is type %s and rhs array is type %s.\n", lineno, str1, str2); numberErrors++; break;
	case 28: sprintf(charBuffer, "ERROR(%d): If not an array, foreach requires lhs of 'in' be of type int but it is type %s.\n", lineno, str1); numberErrors++; break;
	case 29: sprintf(charBuffer, "ERROR(%d): If not an array, foreach requires rhs of 'in' be of type int but it is type %s.\n", lineno, str1); numberErrors++; break;
	case 30: sprintf(charBuffer, "ERROR(%d): In foreach statement the variable to the left of 'in' must not be an array.\n",lineno); numberErrors++; break;
	case 31: sprintf(charBuffer, "ERROR(%d): Array '%s' must be of type char to be initialized, but is of type %s.\n", lineno, str1, str2); numberErrors++; break;
	case 32: sprintf(charBuffer, "ERROR(%d): Initializer for array variable '%s' must be a string, but is of nonarray type %s.\n", lineno, str1, str2); numberErrors++; break;
	case 33: sprintf(charBuffer, "ERROR(%d): Initializer for nonarray variable '%s' of type %s cannot be initialized with an array.\n", lineno, str1, str2); numberErrors++; break;
	case 34: sprintf(charBuffer, "ERROR(%d): Initializer for variable '%s' is not a constant expression.\n", lineno, str1); numberErrors++; break;
	case 35: sprintf(charBuffer, "ERROR(%d): Variable '%s' is of type %s but is being initialized with an expression of type %s.\n", lineno, str1, str2, str3); numberErrors++; break;
	case 36: sprintf(charBuffer, "ERROR(LINKER): Procedure main is not defined.\n"); numberErrors++; break;
	case 37: sprintf(charBuffer, "ERROR(%d): '%s' requires operands of type char or int but rhs is of type %s.\n", lineno, str1, str2); numberErrors++; break;
	case 38: sprintf(charBuffer, "ERROR(%d): '%s' requires operands of type char or int but lhs is of type %s.\n", lineno, str1, str2); numberErrors++; break;
	}

	//Add the error message to the error buffer
	error e; //new error
	e.lineno = lineno; //set its line number
	e.msg = strdup(charBuffer); //get all the chars from the char buffer
	errorBuffer.push_back(e); //Add the error to the errorBuffer
}

//Get the expected lhs and rhs and return type based on the op kind
void getExpectedTypes(const char* s, bool isUnary, bool &unaryError, 
	DeclType &lhs, DeclType &rhs, DeclType &rt) {
	std::string op(s);
	unaryError = false;

	//not commenting everything, all should be self explanatory following basic coding practice
	
	if (isUnary) {
		if (op == "!") lhs = rhs = rt = Bool;
		if (op == "*") { lhs = rhs = Undefined; rt = Int; }
		if (op == "?" || op == "-" || op == "--" || op == "++") lhs = rhs = rt = Int;
	} else {
		if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "+="
			|| op == "-=" || op == "*=" || op == "/=") {
				lhs = rhs = rt = Int;
				unaryError = true;
			}
		if (op == ">" || op == "<" || op == ">=" || op == "<=") {
			lhs = rhs = IntChar; //Intchar to handle int or char for comparison allows alphabetical
			rt = Bool;
		}
		if (op == "==" || op == "!=") {
			lhs = rhs = Undefined;
			rt = Bool;
		}
		if (op == "&" || op == "|") {
			lhs = rhs = rt = Bool;
			unaryError = true;
		}
		if (op == "=") {
			lhs = rhs = rt = Undefined;
		}

	}
}

SymbolTable GetSymbolTable() {
	return symbolTable;
}

//Get a string based on a type
char * typeToCharP(DeclType t) {
	switch (t) {
	case Int: return strdup ("int");
	case Bool: return strdup ("bool");
	case Char: return strdup ("char");
	case Void: return strdup ("void");
	case String: return strdup ("char");
	default: return strdup("[ERROR]"); 
	}
}

TreeNode * CreateIOFunctions(TreeNode * root) {
	//output outputb outputc inputa inputb inputc outnl name
	//void void void int bool char void return type
	//int bool char void void void void params
	

	std::string names[] = { "output", "outputb", "outputc", "input", "inputb", "inputc", "outnl", "name" };
	DeclType returnTypes[] = { Void, Void, Void, Int, Bool, Char, Void, Void };
	DeclType paramTypes[] = { Bool, Char, Void, Void, Void, Void, Void, Void }; 

	//All of this is throwing a segfault on heckendorns compiler, works fine on mine but idk
	/*
	TreeNode* r;
	TreeNode* temp;
	for (int i=0; i<8; i++) {
		TreeNode *IONode = newDeclNode(FuncK);
		if (temp != NULL) 
			temp->sibling = IONode;		
		else r = IONode;

		IONode->lineno = -1;
		IONode->type = returnTypes[i];		
		IONode->attr.name = strdup(names[i].c_str());

		if (IONode->type != Void) {
			IONode->child[0] = newDeclNode(ParamK);
			IONode->child[0]->type = paramTypes[i];
			IONode->child[0]->lineno = -1;
			IONode->child[0]->attr.name = strdup("*dummy*");
		}
		temp = IONode;	
	}

	temp->sibling = root;
	*/

	TreeNode *node3 = newDeclNode(FuncK);
	node3->lineno = -1;
	node3->type = Int;
	node3->attr.name = strdup("input");
	node3->memSize = -2; node3->memOffset = 0;
	
	TreeNode *node = newDeclNode(FuncK);
	node->lineno = -1;
	node->type = Void;
	node->attr.name = strdup("output");
	TreeNode *child = newDeclNode(ParamK);
	child->type = Int;
	child->lineno = -1;
	child->attr.name = strdup("*dummy*");
	node->child[0] = child;
	node->memSize = -3; node->memOffset = 0;
	child->memSize = 1; child->memOffset = -2;

	TreeNode *node4 = newDeclNode(FuncK);
	node4->lineno = -1;
	node4->type = Bool;
	node4->attr.name = strdup("inputb");
	node4->memSize = -2; node4->memOffset = 0;

	TreeNode *node1 = newDeclNode(FuncK);
	node1->lineno = -1;
	node1->type = Void;
	node1->attr.name = strdup("outputb");
	TreeNode *child1 = newDeclNode(ParamK);
	child1->type = Bool;
	child1->lineno = -1;
	child1->attr.name = strdup("*dummy*");
	node1->child[0] = child1;
	node1->memSize = -3; node1->memOffset = 0;
	child->memSize = 1; child->memOffset = 2;

	TreeNode *node5 = newDeclNode(FuncK);
	node5->lineno = -1;
	node5->type = Char;
	node5->attr.name = strdup("inputc");
	node5->memSize = -2; node5->memOffset = 0;

	TreeNode *node2 = newDeclNode(FuncK);
	node2->lineno = -1;
	node2->type = Void;
	node2->attr.name = strdup("outputc");
	TreeNode *child2 = newDeclNode(ParamK);
	child2->type = Char;
	child2->lineno = -1;
	child2->attr.name = strdup("*dummy*");
	node2->child[0] = child2;	
	node2->memSize = -3; node2->memOffset = 0;
	child2->memSize = 1; child2->memOffset = 2;

	TreeNode *node6 = newDeclNode(FuncK);
	node6->lineno = -1;
	node6->type = Void;
	node6->attr.name = strdup("outnl");
	node6->memSize = -2; node6->memOffset = 0;

	node3->sibling = node;
	node->sibling = node4;
	node4->sibling = node1;
	node1->sibling = node5;
	node5->sibling = node2;
	node2->sibling = node6;
	node6->sibling = root;

	return node3;
}

void printToken( char *tokenString ) {

} 

TreeNode *newStmtNode(StmtKind kind) {
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));

	int i;
	for (i=0; i<3; i++) t->child[i] = NULL;
	t-> sibling = NULL;
	t->nodekind = StmtK;
	t->kind.stmt = kind;
	t->lineno = yylineno;

	return t;
}

TreeNode *newExpNode(ExpKind kind) {
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));

	int i;
	for (i=0; i<3; i++) t->child[i] = NULL;
	t->sibling = NULL;
	t->nodekind = ExpK;
	t->kind.exp = kind;
	t->lineno = yylineno;

	return t;
}

TreeNode *newDeclNode(DeclKind kind) {
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));

	int i;
	for (i=0; i<3; i++) t->child[i] = NULL;
	t->sibling = NULL;
	t->nodekind = DeclK;
	t->kind.decl = kind;
	t->lineno = yylineno;

	return t;
}

void printAnTree (TreeNode * tree, int spaces, int siblingIndex, bool printMem) {

	if (spaces == -1) {
		printANode(tree, printMem);
		printf(" [line: %i]\n", tree->lineno);
		spaces++;
	}

	for (int i=0; i<3; i++) {
		if (tree->child[i] != NULL) {
			spaces += 4;
			printSpaces(spaces);
		
			printf("Child: %i ", i);
			printANode(tree->child[i], printMem);
			printf(" [line: %i]\n", tree->child[i]->lineno);			

			printAnTree(tree->child[i], spaces, 0, printMem);
			spaces -= 4;
		}
	}

	if (tree->sibling != NULL) {
		printSpaces(spaces+1);

		printf("Sibling: %i ", ++siblingIndex);
		printANode(tree->sibling, printMem);
		printf(" [line: %i]\n", tree->sibling->lineno);
	
		printAnTree(tree->sibling, spaces, siblingIndex, printMem);
	} 
}

void printTree (TreeNode * tree, int spaces, int siblingIndex) {	

	if (spaces == -1) {
		printNode(tree);
		printf(" [line: %i]\n", tree->lineno);
		spaces++;
	}

	for (int i=0; i<3; i++)
		if (tree->child[i] != NULL) {
			spaces += 4;
			printSpaces(spaces);
		
			printf("Child: %i ", i);
			printNode(tree->child[i]);
			printf(" [line: %i]\n", tree->child[i]->lineno);
		
			printTree(tree->child[i], spaces, 0);
			spaces -= 4;
		}

	if (tree->sibling != NULL) {
		printSpaces(spaces+1);

		printf("Sibling: %i ", ++siblingIndex);
		printNode(tree->sibling);
		printf(" [line: %i]\n", tree->sibling->lineno);

		printTree(tree->sibling, spaces, siblingIndex);
	}
}

void printANode(TreeNode * t, bool printMem) {
	//if (!printMem) 
	//{
		printNode(t);
	//
	//	if (t->nodekind == ExpK) 
	//		printf(" Type: %s", typeToCharP(t->type));
	//}
	//else {
	//	printNode(t);

	//	if (t->nodekind == DeclK && t->kind.decl == VarK) 
	//		printf(" Type: %s", typeToCharP(t->type));

	if (t->nodekind == ExpK) {
		if (t->isArray && t->child[0] == NULL)
			printf(" Type: is array of %s", typeToCharP(t->type));
		else printf(" Type: %s", typeToCharP(t->type));
	}
	
	if (t->nodekind == StmtK && t->kind.stmt == CompK) {
		printf(" with size %i at end of it's declarations", t->memSize);
	} else if (t->nodekind == DeclK && t->kind.decl == ParamK) {
		printf(" allocated as Parameter of size %i and data location %i", t->memSize, t->memOffset);
	} else if (t->nodekind == DeclK && t->kind.decl == VarK) {
		printf(" allocated as %s%s of size %i and data location %i", 
			t->isGlobal ? "Global" : "Local", t->isStatic ? "Static" : "", 
			t->memSize, t->memOffset);
	} else if (t->nodekind == DeclK && t->kind.decl == FuncK) {
		printf(" allocated as %s%s of size %i and exec location %i",
			t->isGlobal ? "Global" : "Local", t->isStatic ? "Static" : "",
			t->memSize, t->memOffset);
	}	
}

void printNode(TreeNode * t) {

	switch (t->nodekind) {
	case (StmtK): printStmtNode(t); break;
	case (ExpK): printExpNode(t); break;
	case (DeclK): printDeclNode(t); break;
	}
}

void printStmtNode(TreeNode * t) {

	switch(t->kind.stmt) {
	case (IfK): printf("If"); break;
	case (ForeachK): printf("Foreach"); break;
	case (WhileK): printf("While"); break;
	case (CompK): printf("Compound"); break;
	case (ReturnK): printf("Return"); break;
	case (BreakK): printf("Break"); break;
	}

}

void printExpNode(TreeNode * t) {

	switch(t->kind.exp) {
	case (OpK): printf("Op: %s", t->attr.name); break;
	case (ConstK): {
		switch (t->type) {
		case (Int): printf("Const: %i", t->attr.iValue); break;
		case (Char):
			if (!t->isArray)  printf("Const: \'%c\'", t->attr.cValue); 
			if (t->isArray)  printf("Const: \"%s\"", t->attr.sValue); 
		break;
		case (Bool): printf("Const: %s", t->attr.iValue == 1 ? "true" : "false"); break;
	} break; }

	case (IdK): printf("Id: %s", t->attr.name); break;
	case (SimpleK): printf("Simple: %s", t->attr.name); break;
	case (CallK): printf("Call: %s", t->attr.name); break;
	case (AssignK): printf("Assign: %s", t->attr.name); break;
	}

}

void printDeclNode(TreeNode * t) {

	switch(t->kind.decl) {
	case (ParamK): printf("Param %s %s", t->attr.name, t->isArray ? "is array of " : ""); break;
	//case (VarK): printf("Var %s ", t->attr.name); break;
	case (VarK): printf("Var %s %s", t->attr.name, t->isArray ? "is array of " : ""); break;
	case (FuncK): printf("Func %s returns type ", t->attr.name); break;
	}

	switch(t->type) {
	case (Void): printf("void"); break;
	case (Int): printf("int"); break;
	case (Bool): printf("bool"); break;
	case (Char): printf("char"); break;
	}

}

void printSpaces(int spaces) {

	char const * iString = "|   ";

	for (int i=0; i<spaces; i++) {
			
		printf("%c", iString[i%4]);
	}
}

