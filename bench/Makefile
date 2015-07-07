CC := g++
AR := ar

SRCDIR := src
INCDIR := include
BLDDIR := ../build/bench
BINDIR := ../bin
LIBDIR := ../lib

TARGET_ST := $(BINDIR)/stbench

OBJ_ST := $(BLDDIR)/suffix_tree_bench.o
CFLAGS := -O3 -std=c++11 -Wall -g
LIB := -L $(LIBDIR) -lds
INC := -I $(INCDIR) -I ../include

all: stbench

stbench: $(TARGET_ST)

$(TARGET_ST): $(OBJ_ST)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo "  $(CC) $^ -o $(TARGET_ST) $(LIB)"; $(CC) $^ -o $(TARGET_ST) $(LIB)

$(BLDDIR)/%.o: $(SRCDIR)/%.cc
	@mkdir -p $(BLDDIR)
	@echo "  $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo "  $(RM) -r $(BLDDIR) $(TARGET_ST)"; $(RM) -r $(BLDDIR) $(TARGET_ST)

.PHONY: clean
