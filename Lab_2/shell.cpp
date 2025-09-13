/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int fd[2];
    pipe(fd);

    int save_stdout = dup(1);
    int save_stdin = dup(0);

    pid_t pid = fork();    		// Create child to run first command
    if (pid == 0){	    
    	dup2(fd[1], 1);			// In child, redirect output to write end of pipe
    	close(fd[0]); 			// Close the read end of the pipe on the child side.
    	execvp("ls", cmd1);		// In child, execute the command
    }

    pid = fork();			// Create another child to run second command
    if (pid == 0){
        dup2(fd[0], 0);			// In child, redirect input to the read end of the pipe
        close(fd[1]);			// Close the write end of the pipe on the child side.
        execvp("tr", cmd2);		// Execute the second command.
    }

    dup2(save_stdout, 1);		// Reset the input and output file descriptors of the parent.
    dup2(save_stdin, 0);
}
