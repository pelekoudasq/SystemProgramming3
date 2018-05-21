CFLAGS	= -g3 -Wall
SERVER	= myhttpd
CRAWLER	= mycrawler
JOBEXE  = ergasia2/jobExecutor
SOBJS	= myhttpd.o serverThread.o auxFun.o queue.o server.o
COBJS	= mycrawler.o crawlThread.o auxFun.o list.o
JOBJS   = ergasia2/jobExecutor.o ergasia2/paths.o ergasia2/worker.o ergasia2/textmap.o ergasia2/postinglist.o ergasia2/trie.o
LDFLAGS = -g3 -pthread

.PHONY : all clean
all: $(SERVER) $(CRAWLER) $(JOBEXE)

$(SERVER): $(SOBJS)

$(CRAWLER): $(COBJS)

$(JOBEXE): $(JOBJS)

clean:
	rm -f $(SOBJS) $(COBJS) $(JOBJS) $(SERVER) $(CRAWLER) $(JOBEXE)

server: $(SERVER)
	./myhttpd -p 8001 -c 8002 -t 5 -d sites

crawler: $(CRAWLER)
	./mycrawler -h localhost -p 8001 -c 8003 -t 5 -d save_dir /site1/page1_14210.html
