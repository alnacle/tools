
NAME = gex
SRC = $(NAME).c
OBJ = $(SRC:.c=.o)

INCS = -I. -I/usr/include
CFLAGS = -std=c99 -pedantic -Wall -O0 ${INCS}
LDFLAGS = -L/usr/lib -lc

all: $(NAME)

.c.o:
	@${CC} -c ${CFLAGS} $<

${NAME}: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f ${NAME} ${OBJ} 

debug: CFLAGS += -DDEBUG -g
debug: all

.PHONY: all clean debug
