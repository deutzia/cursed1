FLAGS = -Wall -Wextra -Wshadow -O0 -g
CFLAGS = ${FLAGS} -std=gnu11

all: converter

read_elf.o: read_elf.c read_elf.h Makefile
	gcc ${CFLAGS} read_elf.c -c -o read_elf.o

convert_elf.o: convert_elf.c convert_elf.h Makefile
	gcc ${CFLAGS} convert_elf.c -c -o convert_elf.o

write_elf.o: write_elf.c write_elf.h Makefile
	gcc ${CFLAGS} write_elf.c -c -o write_elf.o

handle_flist.o: handle_flist.c handle_flist.h Makefile
	gcc ${CFLAGS} handle_flist.c -c -o handle_flist.o

count.o: count.c count.h Makefile
	gcc ${CFLAGS} count.c -c -o count.o

main.o: main.c write_elf.h convert_elf.h read_elf.h count.h Makefile
	gcc ${CFLAGS} main.c -c -o main.o

converter: read_elf.o convert_elf.o write_elf.o main.o count.o handle_flist.o
	gcc ${CFLAGS}  main.o write_elf.o convert_elf.o read_elf.o count.o -o converter handle_flist.o

clean:
	rm -f *.o converter
