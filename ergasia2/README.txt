Ioannis Pelekoudas
AM: 1115201500128

System Programming
Project 2

Overview:

Compile by typing make in the terminal.
The executable file (jobExecutor) takes two flags, -w, number of workers, and -d, name of text file with the directories.
Both of these MUST exist, no default value to number of workers, cannot be zero though.
The program reads the given file containing the paths and adds them to a map struct.
Right after that, the program creates numWorkers*2 fifo pipes -two for each worker, read and write- at the tmp folder of the system.
Time to fork, hold fifos and child process ids.
If the return value of fork is 0, child, call worker() function, for better code abstraction.
Otherwise, is jobExecutor's operation code.
JE creates ./log directory
Both jobExecutor and the workers, create the pipe names and open them from their side. (O_NONBLOCK)
Path sharing begins, with round-robin logic. For every worker, until paths are done.
For every path, worker opens the directory. For every file in the directory creates a map and adds the map to its trie.
During this process, worker counts the characters, words and lines.
Write "stop" string to all workers to communicate the end of path sharing.
After a worker has received the "stop" string, calls the waitForSignal() function.
waitForSignal() function puts the process in pause() state until it receives a SIGIO signal or a SIGTERM signal.
jobExecutor waits for input string from the user until he gets /exit.
>If JE gets a /wc, for every worker sends a SIGIO signal and writes "wc" to the pipe. Worker sends the stats to JE. JE prints the total results.
>If JE gets a /maxcount or /mincount with a keyword, sends SIGIO, writes "max" or "min" and the keyword.
Every worker searches the word, saves the posting list of the word, classifies it by file that exists and sorts the results before sending the highest or lowest to JE.
JE holds only the highest or lowest from all workers and prints the results.
>If JE gets /exit, sends a SIGTERM signal to all workers. Every worker closes its fifos and frees all their structures before it successfully exits.
JE closes all fifos from its side and frees all the structures before it exits.

Assumptions:

--> Every worker can finish with every path it gets before the sleep(5) expires.

--> NO SEARCH FUNC AVAILABLE

--> BASH ONLY Total number of keywords searched: AVAILABLE
