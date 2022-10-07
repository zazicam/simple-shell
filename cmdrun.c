/*
 * Skeleton code for Lab 2 - Shell processing
 * This file contains skeleton code for executing parsed commands.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "cmdparse.h"
#include "cmdrun.h"

static int 
prepare(command_t *cmd, int *pass_pipefd, int *pipefd, int *redirfd)
{
    // 1. Set up stdout to point to this command's pipe, if necessary;
    //    close some file descriptors, if necessary (which ones?)
    if (cmd->controlop == CMD_PIPE) 
    {
        dup2(pipefd[1], 1);
    }

    // 2. Set up stdin to point to the PREVIOUS command's pipe (that
    //    is, *pass_pipefd), if appropriate; close a file
    //    descriptor (which one?)
    if(*pass_pipefd != STDIN_FILENO) 
    {
        dup2(*pass_pipefd, 0);
    }

    // 3. Set up redirections. Use the open() system call.
    //    Hints:
    //       --For input redirections, the oflag should be O_RDONLY.
    //       --For output redirections (stdout and stderr), what
    //       oflags do you want?
    //       --Set the mode argument of open() to be 0666
    if(cmd->redirect_filename[0])
    {
        redirfd[0] = open(cmd->redirect_filename[0], O_RDONLY, 0666); 
        if(redirfd[0] < 0)
            return -1;
        dup2(redirfd[0], 0);
    }
    if(cmd->redirect_filename[1])
    {
        redirfd[1] = open(cmd->redirect_filename[1], O_WRONLY|O_CREAT|O_TRUNC, 0666); 
        if(redirfd[1] < 0)
            return -1;
        dup2(redirfd[1], 1);
    }
    if(cmd->redirect_filename[2])
    {
        redirfd[2] = open(cmd->redirect_filename[2], O_WRONLY|O_CREAT|O_TRUNC, 0666); 
        if(redirfd[2] < 0)
            return -1;
        dup2(redirfd[2], 2);
    }

    return 0;
}

static pid_t
cmd_exec(command_t *cmd, int *pass_pipefd, int *used_pipefd)
{
	pid_t pid = -1;		// process ID for child
	int pipefd[2];		// file descriptors for this process's pipe

	// Create a pipe, if this command is the left-hand side of a pipe.
	// Return -1 if the pipe fails.
	if (cmd->controlop == CMD_PIPE) {
        int res = pipe(pipefd);
        if(res == -1)
            return -1;
    }

    // b. A null command (no subshell, no arguments).
    //    Exit with status 0.
    if(!cmd->argv[0] && !cmd->subshell) // empty command
    {
        return 0;
    }

    if(cmd->argv[0])
    {
        // c. "exit".
        if(!strcmp(cmd->argv[0], "exit"))
        {
            exit(0);
        }

        // d. "cd".
        if(!strcmp(cmd->argv[0], "cd"))
        {
            if(!cmd->argv[1] || cmd->argv[2])
            {
                printf("cd: Syntax error! Wrong number of arguments!\n");
                return -1;
            }

            int savefd[3] = {dup(0), dup(1), dup(2)};
            int redirfd[3] = {-1, -1, -1};
            if(prepare(cmd, pass_pipefd, pipefd, redirfd)<0)
                return -1;

            int res = chdir(cmd->argv[1]);
            if(res == -1) 
            {
                perror("cd");
            }
                
            for(int i=0;i<3;i++)
            {
                if(redirfd[i]!=-1)
                {
                    close(redirfd[i]);
                    dup2(savefd[i], i);
                    close(savefd[i]);
                }
            }

            if(res == -1) 
                return -1;
            return 0;
        }

        // e. "our_pwd".
        if(!strcmp(cmd->argv[0], "our_pwd"))
        {
            int savefd[3] = {dup(0), dup(1), dup(2)};
            int redirfd[3] = {-1, -1, -1};
            if(prepare(cmd, pass_pipefd, pipefd, redirfd)<0)
                return -1;

            char cwd[1024];
            char *p = getcwd(cwd, sizeof(cwd));
            if(!p)
                return -1;

            printf("%s\n", cwd);

            for(int i=0;i<3;i++)
            {
                if(redirfd[i]!=-1)
                {
                    close(redirfd[i]);
                    dup2(savefd[i], i);
                }
            }
            return 0;
        }
    }
    
    pid = fork();
    if(pid == -1)
        return -1;

    if(pid == 0) // child
    {
        int redirfd[3] = {-1, -1, -1};
        if(prepare(cmd, pass_pipefd, pipefd, redirfd)<0)
            return -1;

        if(cmd->subshell)
        {
            int res = cmd_line_exec(cmd->subshell);
            exit(res);
        }

        execvp(cmd->argv[0], cmd->argv);
        exit(0);
    }

    // parent : we are to update pass_pipefd to transfer it
    // to the next command and used_pipefd to transfer it
    // to cmd_line_exec() and close there (when created
    // child will be closed - after waitpid)
    if (cmd->controlop == CMD_PIPE) {
        *pass_pipefd = pipefd[0];
        *used_pipefd = pipefd[1];
    } 
    else
    {
        *pass_pipefd = STDIN_FILENO;
        *used_pipefd = pipefd[0];
    }

	// return the child process ID
	return pid;
}


/* cmd_line_exec(cmdlist)
 *
 *   Execute the command list.
 *
 *   Execute each individual command with 'cmd_exec'.
 *   String commands together depending on the 'cmdlist->controlop' operators.
 *   Returns the exit status of the entire command list, which equals the
 *   exit status of the last completed command.
 *
 *   The operators have the following behavior:
 *
 *      CMD_END, CMD_SEMICOLON
 *                        Wait for command to exit.  Proceed to next command
 *                        regardless of status.
 *      CMD_AND           Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status 0.  Otherwise
 *                        exit the whole command line.
 *      CMD_OR            Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status != 0.
 *                        Otherwise exit the whole command line.
 *      CMD_BACKGROUND, CMD_PIPE
 *                        Do not wait for this command to exit.  Pretend it
 *                        had status 0, for the purpose of returning a value
 *                        from cmd_line_exec.
 */
int
cmd_line_exec(command_t *cmdlist)
{
	int cmd_status = 0;	    // status of last command executed
	int pipefd = STDIN_FILENO;  // read end of last pipe
    int usedfd = -1; // used end of last pipe (we are to close it)

	while (cmdlist) {
		int wp_status;	    // Use for waitpid's status argument!
				    // Read the manual page for waitpid() to
				    // see how to get the command's exit
				    // status (cmd_status) from this value.

        cmd_status = cmd_exec(cmdlist, &pipefd, &usedfd);

        if(cmd_status > 0 && cmdlist->controlop != CMD_BACKGROUND)
        {
            waitpid(cmd_status, &wp_status, 0);
            if (usedfd > 2) {
                close(usedfd);
            }

            if(WIFEXITED(wp_status)) 
                cmd_status = WEXITSTATUS(wp_status);
        }

        switch(cmdlist->controlop)
        {
            case CMD_END:
            case CMD_SEMICOLON:
                
            break;

            case CMD_OR:
                if(!cmd_status)
                    goto done; 
            break;

            case CMD_AND:
                if(cmd_status)
                    goto done; 
            break;

            case CMD_BACKGROUND:
            case CMD_PIPE:
            break;
        }

		cmdlist = cmdlist->next;
	}

	while (waitpid(0, 0, WNOHANG) > 0);

done:
	return cmd_status;
}
