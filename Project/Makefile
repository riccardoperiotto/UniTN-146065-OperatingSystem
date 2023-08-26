#macro definitions
CC = gcc
CFLAGS = -std=gnu90
MKDIR_P = mkdir -p


SRCDIR := ./src
LIBDIR:= ./lib
BUILDDIR := ./build
BINDIR := ./bin
LIB:= $(BUILDDIR)/common.o $(BUILDDIR)/communication.o $(BUILDDIR)/graphic.o $(BUILDDIR)/list.o $(BUILDDIR)/process.o $(BUILDDIR)/reader.o

#color
RED=\033[0;31m
GREEN=\033[0;32m
YELLOW=\033[0;33m
MAGENTA=\033[0;35m
NC=\33[0m

#PHONY targets
.PHONY: directories clean

default: help

build: directories $(BINDIR)/home $(BINDIR)/controlUnit $(BINDIR)/hub $(BINDIR)/timer $(BINDIR)/bulb $(BINDIR)/fridge $(BINDIR)/window $(BINDIR)/manualMode
	@echo "$(GREEN)Project components compiled successfully.$(NC)"

help:
	@echo ""
	@echo "$(YELLOW)PROJECT INFORMATION:$(NC)"
	@echo "$(MAGENTA)GROUP MEMBERS:$(NC)"
	@echo "Battistotti Tommaso"
	@echo "Berlanda Leonardo"
	@echo "Farina Davide"
	@echo "Periotto Riccardo"
	@echo "$(MAGENTA)UBUNTU VERSION SUPPORTED:$(NC)"
	@echo "16.*"
	@echo "18.*"
	@echo "19.*"
	@echo "$(MAGENTA)MAKE COMMANDS$(NC)"
	@echo "Use 'make help' to see this information page."
	@echo "Use 'make build' to create both object and binary files."
	@echo "Use 'make clean' to delete those files."
	@echo "Use './run.sh' and './manualMode.sh' to run the executibles."
	@echo ""

#executible rules
$(BINDIR)/home: $(BUILDDIR)/home.o $(LIB)
	$(CC) $(BUILDDIR)/home.o $(LIB) -o $(BINDIR)/home $(CFLAGS)

$(BINDIR)/controlUnit: $(BUILDDIR)/controlUnit.o $(LIB)
	$(CC) $(BUILDDIR)/controlUnit.o $(LIB) -o $(BINDIR)/controlUnit $(CFLAGS)

$(BINDIR)/hub: $(BUILDDIR)/hub.o $(LIB)
	$(CC) $(BUILDDIR)/hub.o $(LIB) -o $(BINDIR)/hub $(CFLAGS)

$(BINDIR)/timer: $(BUILDDIR)/timer.o $(LIB)
	$(CC) $(BUILDDIR)/timer.o $(LIB) -o $(BINDIR)/timer $(CFLAGS)

$(BINDIR)/bulb: $(BUILDDIR)/bulb.o $(LIB)
	$(CC) $(BUILDDIR)/bulb.o $(LIB) -o $(BINDIR)/bulb $(CFLAGS)

$(BINDIR)/window: $(BUILDDIR)/window.o $(LIB)
	$(CC) $(BUILDDIR)/window.o $(LIB) -o $(BINDIR)/window $(CFLAGS)

$(BINDIR)/fridge: $(BUILDDIR)/fridge.o $(LIB)
	$(CC) $(BUILDDIR)/fridge.o $(LIB) -o $(BINDIR)/fridge $(CFLAGS)

$(BINDIR)/manualMode: $(BUILDDIR)/manualMode.o $(LIB)
	$(CC) $(BUILDDIR)/manualMode.o $(LIB) -o $(BINDIR)/manualMode $(CFLAGS)
	@echo "$(GREEN)Devices excetible files compiled successfully.$(NC)"
	@echo ""

#object rules for executible
$(BUILDDIR)/hub.o: $(SRCDIR)/hub.c
	$(CC) -c $(SRCDIR)/hub.c -o $(BUILDDIR)/hub.o $(CFLAGS)

$(BUILDDIR)/timer.o: $(SRCDIR)/timer.c
	$(CC) -c $(SRCDIR)/timer.c -o $(BUILDDIR)/timer.o $(CFLAGS)

$(BUILDDIR)/controlUnit.o: $(SRCDIR)/controlUnit.c
	$(CC) -c $(SRCDIR)/controlUnit.c -o $(BUILDDIR)/controlUnit.o $(CFLAGS)

$(BUILDDIR)/home.o: $(SRCDIR)/home.c
	$(CC) -c $(SRCDIR)/home.c -o $(BUILDDIR)/home.o $(CFLAGS)

$(BUILDDIR)/bulb.o: $(SRCDIR)/bulb.c
	$(CC) -c $(SRCDIR)/bulb.c -o $(BUILDDIR)/bulb.o $(CFLAGS)

$(BUILDDIR)/window.o: $(SRCDIR)/window.c
	$(CC) -c $(SRCDIR)/window.c -o $(BUILDDIR)/window.o $(CFLAGS)

$(BUILDDIR)/fridge.o: $(SRCDIR)/fridge.c
	$(CC) -c $(SRCDIR)/fridge.c -o $(BUILDDIR)/fridge.o $(CFLAGS)

$(BUILDDIR)/manualMode.o: $(SRCDIR)/manualMode.c
	$(CC) -c $(SRCDIR)/manualMode.c -o $(BUILDDIR)/manualMode.o $(CFLAGS)

#object rules for library
$(BUILDDIR)/common.o: $(LIBDIR)/common.c
	$(CC) -c $(LIBDIR)/common.c -o $(BUILDDIR)/common.o $(CFLAGS)

$(BUILDDIR)/communication.o: $(LIBDIR)/communication.c
	$(CC) -c $(LIBDIR)/communication.c -o $(BUILDDIR)/communication.o $(CFLAGS)

$(BUILDDIR)/list.o: $(LIBDIR)/list.c
	$(CC) -c $(LIBDIR)/list.c -o $(BUILDDIR)/list.o $(CFLAGS)

$(BUILDDIR)/reader.o: $(LIBDIR)/reader.c
	$(CC) -c $(LIBDIR)/reader.c -o $(BUILDDIR)/reader.o $(CFLAGS)

$(BUILDDIR)/process.o: $(LIBDIR)/process.c 
	$(CC) -c $(LIBDIR)/process.c -o $(BUILDDIR)/process.o $(CFLAGS)

$(BUILDDIR)/graphic.o: $(LIBDIR)/graphic.c 
	$(CC) -c $(LIBDIR)/graphic.c -o $(BUILDDIR)/graphic.o $(CFLAGS)
	@echo "$(GREEN)Library object created successfully.$(NC)"
	@echo ""

#directories rules
directories:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(BINDIR)	
	@echo "$(GREEN)Folders created$(NC)"
	@echo ""


#clean rules
clean:
	@rm -r -f $(BINDIR)
	@rm -r -f $(BUILDDIR)
	@echo "$(GREEN)Project cleaned$(NC)"

