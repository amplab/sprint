CC := g++
AR := ar

SRCDIR := src
BLDDIR := build
BINDIR := bin
LIBDIR := lib

TARGET := $(LIBDIR)/libds.a

SOURCES := $(shell find $(SRCDIR) -type f -name *.cc)
OBJECTS := $(patsubst $(SRCDIR)/%,$(BLDDIR)/%,$(SOURCES:.cc=.o))
CFLAGS := -O3 -std=c++11 -Wall -g
LIB :=
INC := -I include -I external/ds-lib/include

all: ds-lib $(TARGET)

ds-lib:
	@echo "Making ds-lib..."
	@echo " cd external/ds-lib/; make" 

$(TARGET): $(OBJECTS)
	@echo "Creating static library..."
	@mkdir -p $(LIBDIR)
	@echo "  $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^

$(BLDDIR)/%.o: $(SRCDIR)/%.cc
	@mkdir -p $(BLDDIR)
	@echo "  $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

bench: $(TARGET)
	@echo "Making benchmarks..."
	@echo "  cd bench/; make"; cd bench/; make
clean:
	@echo "Cleaning..."; 
	@echo "  $(RM) -r $(BLDDIR) $(TARGET)"; $(RM) -r $(BLDDIR) $(TARGET)
	@echo "  cd bench/; make clean"; cd bench/; make clean

.PHONY: clean
