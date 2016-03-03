OBJS=main.o vtable.o get_sensor.o
NAME=vtable
HFILES=vtable.h
NAME_TEST=fill_db
CFLAGS+=-Wall -Werror -Wformat
CFLAGS+=-g
CFLAGS+=-DUSE_MMAP
LDFLAGS=-lsqlite3

all:$(NAME) $(NAME_TEST)

%.o: $(HFILES)
$(NAME):$(OBJS)  
	$(CC) $(LDFLAGS) $^  -o $@

$(NAME_TEST): $(NAME_TEST).o get_sensor.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(NAME) $(NAME_TEST) $(NAME_TEST).o
rebuild: clean all
