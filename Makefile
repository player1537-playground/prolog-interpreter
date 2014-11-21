CC = gcc
STND = -ansi
CFLAGS = $(STND) -pedantic -g -Werror -Wall -Wextra -Wformat=2 -Wshadow -Wno-long-long \
		 -Wno-overlength-strings -Wno-format-nonliteral -Wcast-align \
		 -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
		 -Wmissing-include-dirs -Wswitch-default

SRC := prolog.c
EXE := $(SRC:.c=)

all: $(EXE)

$(EXE): $(SRC) mpc/mpc.c
	$(CC) $(CFLAGS) $^ -lm -o $@
