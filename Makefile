LIB_SRCS = overrides.c messageDispatch.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

CFLAGS += -Wall
LDFLAGS= -fPIC 

default: liblistener.so

%.o: %.c
	$(CC) $(CFLAGS) -fPIC  -o $@ -c $^

%.o: %.cpp
	$(CC) $(CPPFLAGS) -fPIC -o $@ -c $^


liblistener.so: $(LIB_OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^ -ldl 

clean:
	rm -f *.o liblistener.so
