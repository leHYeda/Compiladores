# Makefile - Calculadora Avancada

TARGET = calcAvancada
CC = gcc

# Dependencia do lib math necessária por conta da funcao B_sqrt, etc
LIBS = -lm

BISON_Y = calcAvancada.y
BISON_C = calcAvancada.tab.c
BISON_H = calcAvancada.tab.h

FLEX_L  = calcAvancada.l
FLEX_C  = calcAvancada.lex.c

SRC_C   = calcAvancada.c

all: $(TARGET)

$(TARGET): $(BISON_C) $(FLEX_C) $(SRC_C)
	$(CC) -o $(TARGET) $(BISON_C) $(FLEX_C) $(SRC_C) $(LIBS)

$(BISON_C) $(BISON_H): $(BISON_Y)
	bison -d $(BISON_Y)

$(FLEX_C): $(FLEX_L) $(BISON_H)
	flex -o $(FLEX_C) $(FLEX_L)

clean:
	rm -f $(BISON_C) $(BISON_H) $(FLEX_C) $(TARGET)