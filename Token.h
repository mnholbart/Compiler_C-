#ifndef TOKEN_H
#define TOKEN_H

typedef struct token {
	union {
		int iValue;
		char *sValue;
		char cValue;
	} type;

	int lineno;
	int sLength;
	char *rawString;

} Token;

#endif
