#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p){
	// open
	// dup2(fd, 0):redirect stdin to fd
	// close
	// open
	// dup2(fd, 1):redirect stdout to fd
	// close
	
	// change input/output to file/pipe
	int fd;
	
	// input redirection
	if (p->in_file != NULL) {
		// open command input
		fd = open(p->in_file, O_RDONLY);
		if (fd == -1) {
			perror(p->in_file);
			exit(1);
		}

		// stdin redirect to fd
		dup2(fd, 0);
		close(fd);
	}

	// output redirection
	if (p->out_file != NULL) {
		// open command ouput (if no exist, create)
		fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd == -1) {
			perror(p->out_file);
			exit(1);
		}

		// stdout redirect to fd
		dup2(fd, 1);
		close(fd);
	}
	
	
	// if there is the pipe in command
	
	// p->in != 0, this command input need to read from pipe
	if (p->in != 0) {
		// redirect stdin to previous command output
		if (dup2(p->in, STDIN_FILENO) == -1) {
            		perror("dup2 pipe in");
            		close(p->in);
            		exit(1);
        	}
        	close(p->in);
	}
	// p->in != 1, this command output need to write to next pipe commamd
	if (p->out != 1) {        
		// redirect stdout to next command input
        	if (dup2(p->out, STDOUT_FILENO) == -1) {
            		perror("dup2 pipe out");
            		close(p->out);
            		exit(1);
        	}
        	close(p->out);
    	}
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	// child process:fork()
	// execvp()
	// parent process:
	// waitpid()
	
	pid_t pid = fork();
	int status;
	
	// fork fail
	if (pid < 0) {
		perror("fork");
	}
	// child process
	else if (pid == 0) {
		// set external command input/output
		redirection(p);

		// call external command, if false return -1
		if (execvp(p->args[0], p->args) == -1) {
			perror("execvp");
			// if no exit, child will keep excute shell process
			exit(EXIT_FAILURE);
		}
	}
	// parent process
	else {
		// shell wait child process finish
		waitpid(pid, &status, 0);
	}
  	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd)
{
	// int pipe_[fd]
	// pipe(pipe_fd) -> pipe_fd[0] pipe_fd[1]
	
	// command node linkedlist 
	struct cmd_node *cur = cmd->head;
   	
	// count commmand/pipe numbers
	int count = 0;
	struct cmd_node *tmp = cmd->head;
	while (tmp) {
    		count++;
    		tmp = tmp->next;
	}
	int num_pipes = count - 1;

	// set pipe array
   	int pipe_fd[2 * num_pipes];
   	pid_t pid;
   	int status;


	// create pipe connection
    	for (int i = 0; i < num_pipes; i++) {
        	if (pipe(pipe_fd + i * 2) < 0) {
            		perror("pipe");
            	return -1;
        	}
    	}
	
	// for loop excute all command inorder
    	int idx = 0;
    	for (cur = cmd->head; cur != NULL; cur = cur->next, idx++) {
		
		// setting command input/output
        	if (idx == 0) 
            		cur->in = 0;  
		else 
            		cur->in = pipe_fd[(idx - 1) * 2];

        	if (idx == num_pipes)
            		cur->out = 1; 
	       	else 
            		cur->out = pipe_fd[idx * 2 + 1]; 
        	
		// spawn_proc()
        	pid = fork();
        	if (pid == 0) {
			redirection(cur);
          		
			// child already redirection, don't need fd
            		for (int j = 0; j < 2 * num_pipes; j++)
                		close(pipe_fd[j]);

            		execvp(cur->args[0], cur->args);
                	perror("execvp");
                	exit(1);
		}
		else if (pid < 0) {
            	perror("fork");
            	return -1;
        	}
	}
	
	// close all pipe
    for (int i = 0; i < 2 * num_pipes; i++)
        close(pipe_fd[i]);
	// wait all child process
   	for (int i = 0; i <= num_pipes; i++)
        wait(&status);

	return 1;
}
// ===============================================================


void shell()
{
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 | out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)  dup2(in, 0);
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
