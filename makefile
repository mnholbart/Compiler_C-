BIN  = c-
CC   = g++
SRCS = $(BIN).y $(BIN).l util.c util.h globals.h Token.h symbolTable.h symbolTable.cpp genCode.h genCode.cpp emitCode.cpp emitCode.h
OBJS = lex.yy.o $(BIN).tab.o util.o symbolTable.o genCode.o emitCode.o
LIBS = -lm 

$(BIN): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) $(LIBS) -o $(BIN)

$(BIN).tab.h $(BIN).tab.c: $(BIN).y
	bison -v -t -d $(BIN).y   # -d supplies defines file, -v supplies output

lex.yy.c: $(BIN).l $(BIN).tab.h
	flex $(BIN).l  # -d debug

util.o: util.c util.h globals.h
	$(CC) $(CCFLAGS) -c util.c

symbolTable.o: symbolTable.h symbolTable.cpp 
	$(CC) $(CCFLAGS) -c symbolTable.cpp

genCode.o: genCode.h genCode.cpp
	$(CC) $(CCFLAGS) -c genCode.cpp

emitCode.o: emitCode.cpp emitCode.h
	$(CC) $(CCFLAGS) -c emitCode.cpp

all:    
	touch $(SRCS)
	make

clean:
	rm -f $(OBJS) $(BIN) lex.yy.c $(BIN).tab.h $(BIN).tab.c $(BIN).tar *~ *.output

tar:
	tar -cvf $(BIN).tar $(SRCS) makefile
