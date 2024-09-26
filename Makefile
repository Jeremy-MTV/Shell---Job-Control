.PHONY: force

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -O -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wno-parentheses \
		 -Wcast-align -Wconversion -Wformat \
		 -Wformat-nonliteral -Wmissing-braces -Wuninitialized \
		 -Wmissing-declarations  -Winline \
		 -Wmissing-prototypes -Wredundant-decls \
		 -Wformat-security -pedantic -lreadline -lhistory

# Source files
SRCS = src/main.c src/parser.c src/prompt.c src/builtin.c src/execute.c src/job.c src/redirections.c src/command.c

# Executable name
TARGET = jsh

# Build target
$(TARGET): force
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS)

# Clean up
clean:
	rm -f $(TARGET)
