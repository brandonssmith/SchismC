# HolyC Windows Port Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = 

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c) \
          $(wildcard $(SRCDIR)/lexer/*.c) \
          $(wildcard $(SRCDIR)/parser/*.c) \
          $(wildcard $(SRCDIR)/codegen/*.c) \
          $(wildcard $(SRCDIR)/runtime/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/schismc.exe

# Default target
all: $(TARGET)

# Create directories
$(OBJDIR):
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\lexer mkdir $(OBJDIR)\lexer
	if not exist $(OBJDIR)\parser mkdir $(OBJDIR)\parser
	if not exist $(OBJDIR)\codegen mkdir $(OBJDIR)\codegen
	if not exist $(OBJDIR)\runtime mkdir $(OBJDIR)\runtime

$(BINDIR):
	if not exist $(BINDIR) mkdir $(BINDIR)

# Build executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build artifacts
clean:
	if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	if exist $(BINDIR) rmdir /s /q $(BINDIR)

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	@$(TARGET) tests/hello.hc
	@$(TARGET) tests/variables.hc
	@$(TARGET) tests/functions.hc

# Install (copy to system path)
install: $(TARGET)
	@echo "Installing schismc to C:\Windows\System32..."
	@copy $(TARGET) C:\Windows\System32\

# Uninstall
uninstall:
	@echo "Removing schismc from C:\Windows\System32..."
	@del C:\Windows\System32\schismc.exe

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the compiler (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  test     - Run test programs"
	@echo "  install  - Install to system path"
	@echo "  uninstall- Remove from system path"
	@echo "  help     - Show this help"

.PHONY: all clean test install uninstall help
