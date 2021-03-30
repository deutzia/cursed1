COMPILER = gcc
FLAGS = -Wall -Wextra -Wshadow -O0 -g
CFLAGS = ${FLAGS} -std=gnu11

SRCS = read_elf.c convert_elf.c write_elf.c handle_flist.c count.c main.c
OBJS = $(SRCS:.c=.o)

DEPDIR := .d
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
$(shell mkdir -p $(DEPDIR) >/dev/null)

all: main src/Main.hs
	stack build

%.o : %.c $(DEPDIR)/%.d
	$(COMPILER) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

main: $(OBJS)
	$(COMPILER) $(CFLAGS) $(OBJS) -o main

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

clean:
	rm -f $(OBJS) main
	rm -rd $(DEPDIR)
