CC=gcc
FLAGS=-Wall -std=gnu99 -O3
LIBS=

NAME=mlinker

.PHONY:
	clean all

${NAME}: ${NAME}.c
	${CC} ${FLAGS} -o ${NAME} ${NAME}.c ${LIBS}

clean:
	rm ${NAME}
