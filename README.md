### Ioannis Pelekoudas
### AM: 1115201500128

# System Programming
## Project 3

#### June 2018



Overview:

SERVER:

Server(myhttpd.c) kicks off with a command with the following format:
```bash
./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir
```
where:
serving port is the port from where server gets HTTP/1.1 requests,
command port is the port from where server gets requests like STATS and SHUTDOWN,
number of threads is the number of the threads that server will create in order to serve the request appearing at the serving port,
and root dir is the directory that includes all the pages that webcreator.sh created.

Server "splits up" using fork in order to listen to both serving and command port at the same time.
This is the reason why we are using shared memory for the queue. (myhttpd.c: 62)

After the fork, the child process calls the serverProc function(server.c)
serverProc initializes the queue and creates, binds and starts to listen to the socket with port the serving port.
After that, it creates the serving threads and begins a loop in which it accepts connections and pushes the new sockets into the queue.
This happens by locking the queue mutex, checking if the queue is not full, pushing, and sending the signal that the queue now is not empty.
The loop is waiting for a signal, and if it gets a SIGTERM, the process handles it, canceling the threads and freeing all the structures.
serverThread function/routine that begins for every thread after its initialization, tries to pop from the queue while it is not empty.
after it gets the socket fd, sends a signal that the queue now is surely not full and begins the request process by calling the getRequest function.
getRequest this function calls the recv_get function where it reads every line of the GET request and fills up the request structure.
In this project we only care about the name, so we just consume the other lines.
Now that getRequest has the name of the page, it tries to access it. Depending on the returns of the access and open syscalls,
the function fills up the status and message strings. If the file exist, we bring in to the memory and update the server stats.
With this two strings ready and after constructin the date time (local not GMT), the server thread builds the header and sends it to the client before the answer. 
All structs freed and fds closed, and the thread tries to get next request from queue.

Back to the parent process, server opens up a socket to listen from the command port.
It can receive STATS and SHUTDOWN commands. In STATS it returns to the client the running time (cpu time), the pages and bytes served.
If it gets SHUTDOWN, it terminates the child process serverProc, waits for it and exits successfully.



CRAWLER:

Crawler(mycrawler.c) kicks off with a command with this format:
```bash
./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL
```
where:
host is the computer where server runs,
port is the one that server listens to,
command port is the port from where crawler gets requests like STATS and SHUTDOWN but also SEARCH which *IS NOT IMPLEMENTED*,
number of threads is the number of the threads that crawler will create in order to crawl the pages and request more from server,
save_dir is the direcory that crawler will create and save the pages there,
and starting URL is the known to crawler URL and the one that will start the crawling process from.

First, crawler creates the save_dir directorym recursively if there are subfolders included in the given path.
Then, adds to the list the given starting URL, and creates the threads, with the threadFun as thread routine(crawlThread.c).

threadFun, after checking if it is empty, removes a page from the list pagesToAdd, adds it to the pagesAdded list, adds one to workers
-usefull for the crawler later on- calls the get function, workers minus one, and from the beginning again.
get function request to connect to the server. If the connection succeeds it constructs the GET HTTP/1.1 request and sends it via the socket to the server.
After that, waits for the answer from the server using the function readAnswerFromServer.
readAnswerFromServer gets the first line from the header sent from the server and examines the code. If it is one of the accepted one, goes ahead
and gets the length of the Content from fourth header line. Then, after consuming the rest of the header lines -not needed in this project- it gets the content.
if the code is not 200 (OK), it prints out the message, else, it processes the page returned using the function contentProcessing.
contentProcessing in the beginning builds off the correct path of the file that is going to be the page returned from server.
It opens the file, and writes to it the page. After that, for every line in that string checks if it is a link.
It properly gets the address and, if it is not in the two lists (already added or already in the to add list), adds it to pagesToAdd list.
After everything is done successfully, thread keeps trying to remove pages from the list.

Back to crawler's main thread, it creates a socket binds to it and starts listening from the command port given.
After that, it starts accepting requests/commands to this port.
It can receive STATS, in which case, like the server, it returns to the client the running time (cpu time), the pages and bytes served.
In the case of SEARCH, crawler checks if the crawling process is done or not. It checks if the pagesToAdd list is empty 
and if all the workers do not work at the same time. This check takes place only until it succeeds thanks to the ready flag.
If thats the case, we execute the small script bashls which creates the pathsfile that jobExecutor needs in project2.
After that we fork, with the help of two pipes we connect stdins and stdouts of parent and child and finally we execute jobExecutor to the child.
Crawler everytime it gets a search requests sends it to stdout via printf and jobExecutor receives it from stdin.

*DISCLAIMER: Search does not work in jobExecutor side, so it just returns "Search not implemented"*

Finally, in case of SHUTDOWN, crawlers sends via stdin /exit to jobExecutor in order to exit successfully.
It closes all fds, cancels all threads and destroys all structures.



WEBCREATOR:

webcreator.sh bash script gets arguements in this exact way:
```bash
./webcreator.sh root_directory text_file w p
```
where:
root_directory is the directory where all sites and their pages will be stored after their own creation,
text_file is a text file from where the creator will get its data,
w is the number of web sites that webcreator will create,
and p is the number of pages per web site.

webcreator checks if all the arguements are correct, it checks if the file is at least 10000 lines and purges the directory content if ots not empty.
After that, the directories of every site are created, and their content, the pages, all empty.
Now, for every page we hard code the html tags in the beginning, put the apropriate lines and the apropriate links according to the qualifications of the project
and hard code the closing html tags

webcreator.sh bash script is relatively slow because it copies line by line from the text_file until the link

Finally, we check if all pages have an incoming link.



Thanks,

Ioannis Pelekoudas, 05-May-2018
