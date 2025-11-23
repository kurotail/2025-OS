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
    // Handle input redirection from file ( < )
    if (p->in_file != NULL) {
        int fd_in = open(p->in_file, O_RDONLY);
        if (fd_in < 0) {
            perror("open input file");
            exit(1);
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(fd_in);
            exit(1);
        }
        close(fd_in);
    }
    // Handle input redirection from pipe
    else if (p->in != STDIN_FILENO) {
        if (dup2(p->in, STDIN_FILENO) < 0) {
            perror("dup2 pipe input");
            exit(1);
        }
        close(p->in);
    }

    // Handle output redirection to file ( > )
    if (p->out_file != NULL) {
        int fd_out = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_out < 0) {
            perror("open output file");
            exit(1);
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(fd_out);
            exit(1);
        }
        close(fd_out);
    }
    // Handle output redirection to pipe
    else if (p->out != STDOUT_FILENO) {
        if (dup2(p->out, STDOUT_FILENO) < 0) {
            perror("dup2 pipe output");
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
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork failed");
		return 0;
	}
	if (pid) {
		int status;
		wait(&status);
	} else {
		redirection(p);
		execvp(p->args[0], p->args);
		perror("execvp");
		exit(1);
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
    struct cmd_node *curr = cmd->head;
    int prev_pipe_read = STDIN_FILENO;  // 第一個命令從 stdin 讀取
    pid_t pid;
    
    // 遍歷所有 cmd_node
    while (curr != NULL) {
        int pipefd[2];
        
        // 如果不是最後一個命令，創建 pipe
        if (curr->next != NULL) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return 0;
            }
        }
        
        pid = fork();
        if (pid < 0) {
            perror("fork");
            return 0;
        }
        
        if (pid == 0) {
            // 設定輸入：從前一個 pipe 或 stdin 讀取
            if (prev_pipe_read != STDIN_FILENO) {
                curr->in = prev_pipe_read;
            }
            
            // 設定輸出：寫到下一個 pipe 或 stdout
            if (curr->next != NULL) {
                curr->out = pipefd[1];
                close(pipefd[0]);  // child 不需要 pipe 的讀端
            }
            redirection(curr);
            int status = searchBuiltInCommand(curr);
            if (status != -1) {
                execBuiltInCommand(status, curr);
                exit(0);
            } else {
                // External command
                execvp(curr->args[0], curr->args);
                perror("execvp");
                exit(1);
            }
        } else {
            // 關閉已使用完的 pipe 讀端
            if (prev_pipe_read != STDIN_FILENO) {
                close(prev_pipe_read);
            }
            
            // 如果創建了新的 pipe，關閉寫端並保存讀端
            if (curr->next != NULL) {
                close(pipefd[1]);  // parent 不需要 pipe 的寫端
                prev_pipe_read = pipefd[0];  // 保存給下一個命令使用
            }
            
            curr = curr->next;
        }
    }
    
    while (wait(NULL) > 0);    
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
				if( in == -1 || out == -1)
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
