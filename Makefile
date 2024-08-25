cc = gcc
cflags = -std=c11 -g -pthread -Wall -Wextra
src = src/main.c src/utils.c src/ring_buf.c
obj = $(src:.c=.o)
target = glab

all: $(target)

$(target): $(obj)
	$(cc) $(cflags) -o $@ $^

%.o: %.c
	$(cc) $(cflags) -c $< -o $@

clean:
	rm -f $(obj) $(target)

run: $(target)
	./$(target)