.PHONY: tests deftest

CC=gcc

ODIR=obj
SDIR=src
BDIR=bin
HDIR=inc
TDIR=tests

LIBS=-lconfig
CFLAGS = -g -I$(HDIR) -Wall $(xml2-config --cflags) $(xml2-config --libs)

DEPS = $(wildcard $(HDIR)/*.h)

SRC = $(wildcard $(SDIR)/*.c)
TESTS_SRC = $(wildcard $(TDIR)/*.c)

OBJ = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRC))
TESTS_OBJ = $(patsubst $(TDIR)/%.c,$(ODIR)/%.o,$(TESTS_SRC))
OBJS = $(wildcard $(ODIR)/*.o)

$(BDIR)/thermocouples: $(OBJ)
	$(CC) -o $@ $(ODIR)/*.o $(LIBS) $(CFLAGS)
	
tests: CFLAGS += -D_TEST_ 
tests: deftest $(TESTS_OBJ) $(OBJ)
	$(CC) -o $(BDIR)/tests $(ODIR)/*.o $(LIBS) $(CFLAGS)

$(ODIR)/test_%.o: $(TDIR)/test_%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(ODIR)/* $(BDIR)/*
