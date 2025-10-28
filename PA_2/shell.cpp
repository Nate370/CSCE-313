#include <iostream>
#include <fcntl.h> //Not originally in file
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    char* dir_buf;
    size_t dir_size;
    char* curr_dir = NULL;
    //int og_stdout = dup(STDOUT_FILENO);
    int og_stdin = dup(STDIN_FILENO);
    char* args[256];
    size_t j;
    string file_name;
    int fd = -1;
    pid_t pid;
    vector<pid_t> pids;
    vector<pid_t> background_pids;
    int status;

    char* prev_dir;
    char* prev_dir_buf;
    dir_size = pathconf(".", _PC_PATH_MAX);
    if ((prev_dir_buf = (char*)malloc(dir_size)) != NULL){
        prev_dir = getcwd(prev_dir_buf, dir_size);
    }

    for (;;) {
        pids.clear();

        // need date/time, username, and absolute path to current dir
        time_t t;
        time(&t);
        std::string curr_time(ctime(&t));
        char* curr_user = getenv("USER");
        dir_size = pathconf(".", _PC_PATH_MAX);
        if ((dir_buf = (char*)malloc(dir_size)) != NULL){
            curr_dir = getcwd(dir_buf, dir_size);
        }
        cout << YELLOW << curr_time.substr(0, curr_time.size()-1) << " " << curr_user << ":" << curr_dir << "$" << NC << " ";
        free(dir_buf);

        bool backgrounds_done = false;
        while (!backgrounds_done){
            backgrounds_done = true;
            for (size_t k = 0; k < background_pids.size(); k++){
                status = 0;
                if (waitpid(background_pids.at(k), &status, WNOHANG) != 0){
                    if (status > 1) {  // exit if child didn't exec properly
                        cout << status << endl;
                        exit(status);
                    }
                    background_pids.erase(background_pids.begin()+k);
                    backgrounds_done = false;
                    break;
                }
            }
        }
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }
        size_t num_cmds = tknr.commands.size();

        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }
        
        if (tknr.commands.at(0)->args.at(0).compare("cd") == 0){
            std::string prev(prev_dir);
            free(prev_dir_buf);
            dir_size = pathconf(".", _PC_PATH_MAX);
            if ((prev_dir_buf = (char*)malloc(dir_size)) != NULL){
                prev_dir = getcwd(prev_dir_buf, dir_size);
            }
            if (tknr.commands.at(0)->args.at(1).compare("-") == 0){
                cout << prev << endl;
                chdir((char*) prev.c_str());
            }
            else{
                chdir((char*) tknr.commands.at(0)->args.at(1).c_str());
            }
            continue;
        }

        for (size_t i = 0; i < num_cmds; i++){
            int pipe_fd[2];
            pipe(pipe_fd);

            for (j = 0; j < tknr.commands.at(i)->args.size(); j++){
                args[j] = (char*) tknr.commands.at(i)->args.at(j).c_str();
            }
            args[j] = nullptr;

            // fork to create child
            pid = fork();
            if (pid < 0) {  // error check
                perror("fork");
                exit(2);
            }

            if (pid == 0) {  // if child, exec to run command
                if (i != (num_cmds-1)){
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[0]);
                }
                else{
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                }

                if (tknr.commands.at(i)->hasOutput()){
                    fd = open((char*) tknr.commands.at(i)->out_file.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                    dup2(fd, STDOUT_FILENO);
                }
                if (tknr.commands.at(i)->hasInput()){
                    fd = open((char*) tknr.commands.at(i)->in_file.c_str(), O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);
                    dup2(fd, STDIN_FILENO);
                }
                
                if (execvp(args[0], args) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }
            }
            else {  // if parent, wait for child to finish
                dup2(pipe_fd[0], STDIN_FILENO);
                close(pipe_fd[1]);
                if (!(tknr.commands.at(i)->isBackground())){
                    pids.push_back(pid);
                }
                else{
                    background_pids.push_back(pid);
                }
                if (i == (num_cmds-1)){
                    status = 0;
                    for (size_t k = 0; k < pids.size() ; k++){
                        waitpid(pids.at(k), &status, 0);
                        if (status > 1) {  // exit if child didn't exec properly
                            cout << status << endl;
                            exit(status);
                        }
                    }
                    dup2(og_stdin, STDIN_FILENO);
                }
            }
        }
    }
    free(prev_dir_buf);
}
