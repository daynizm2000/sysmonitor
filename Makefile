SRCS=$(shell find . -name "*.c" -not -path "./libs/*")
GTK=$(shell pkg-config --cflags --libs gtk4)

all:
	$(MAKE) -C libs/mlib-memory/
	gcc -Wextra -Wall $(SRCS) libs/mlib-memory/*.a $(LIBS) $(GTK) -O2 -o sysm

clean:
	$(MAKE) -C libs/mlib-memory/ clean
	rm sysm

run:
	./sysm