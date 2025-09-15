# SchismC Makefile
# True assembly-based HolyC compiler for Windows
# Supports MASM + Windows SDK for PE generation

CC = gcc
VS_PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
ASM = $(VS_PATH)\ml64.exe
LINK = $(VS_PATH)\link.exe
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
ASMFLAGS = /c /Cp /Cx /W3 /nologo
LINKFLAGS = /SUBSYSTEM:CONSOLE /ENTRY:main
INCLUDES = -Iinclude
SRCDIR = src
OBJDIR = obj
BINDIR = bin
ASMDIR = asm

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c) \
          $(wildcard $(SRCDIR)/runtime/*.c) \
          $(SRCDIR)/frontend/lexer/lexer.c \
          $(SRCDIR)/frontend/parser/parser.c \
          $(SRCDIR)/frontend/type_checker/type_checker.c \
          $(wildcard $(SRCDIR)/middleend/intermediate/*.c) \
          $(wildcard $(SRCDIR)/backend/assembly/*.c) \
          $(wildcard $(SRCDIR)/backend/aot/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/schismc.exe

# Default target
all: $(TARGET)

# Create directories
$(OBJDIR):
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(OBJDIR)\frontend mkdir $(OBJDIR)\frontend
	@if not exist $(OBJDIR)\frontend\lexer mkdir $(OBJDIR)\frontend\lexer
	@if not exist $(OBJDIR)\frontend\parser mkdir $(OBJDIR)\frontend\parser
	@if not exist $(OBJDIR)\middleend mkdir $(OBJDIR)\middleend
	@if not exist $(OBJDIR)\middleend\intermediate mkdir $(OBJDIR)\middleend\intermediate
	@if not exist $(OBJDIR)\middleend\optimization mkdir $(OBJDIR)\middleend\optimization
	@if not exist $(OBJDIR)\backend mkdir $(OBJDIR)\backend
	@if not exist $(OBJDIR)\backend\codegen mkdir $(OBJDIR)\backend\codegen
	@if not exist $(OBJDIR)\backend\registers mkdir $(OBJDIR)\backend\registers
	@if not exist $(OBJDIR)\backend\assembly mkdir $(OBJDIR)\backend\assembly
	@if not exist $(OBJDIR)\runtime mkdir $(OBJDIR)\runtime

$(BINDIR):
	@if not exist $(BINDIR) mkdir $(BINDIR)

# Build object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@

# Clean build artifacts
clean:
	@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	@if exist $(BINDIR) rmdir /s /q $(BINDIR)

# Install (copy to system path)
install: $(TARGET)
	copy $(TARGET) C:\Windows\System32\

# Test target
test: $(TARGET)
	@echo "Running tests..."
	@if exist tests\*.hc for %%f in (tests\*.hc) do $(TARGET) %%f

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Help target
help:
	@echo "SchismC Build System"
	@echo "==================="
	@echo "Available targets:"
	@echo "  all      - Build the compiler (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release"
	@echo "  test     - Run test programs"
	@echo "  install  - Install to system path"
	@echo "  help     - Show this help"

.PHONY: all clean install test debug release help
