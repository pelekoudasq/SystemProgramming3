CC		= gcc
CFLAGS	= -g3 -Wall -pthread
SERVER	= myhttpd
CRAWLER	= mycrawler
SOBJS	= myhttpd.o serverThread.o auxFun.o queue.o
COBJS	= mycrawler.o crawlThread.o auxFun.o

.PHONY : all clean
all: $(SERVER) $(CRAWLER)

$(SERVER): $(SOBJS)
	$(CC) $(CFLAGS) -o $@ $(SOBJS)

$(CRAWLER): $(COBJS)
	$(CC) $(CFLAGS) -o $@ $(COBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(SOBJS) $(COBJS) $(SERVER) $(CRAWLER)

server: $(SERVER)
	./myhttpd -p 8001 -c 8002 -t 5 -d root_dir

crawler: $(CRAWLER)
	./mycrawler -h localhost -p 8001 -c 8003 -t 5 -d save_dir starting_URL
