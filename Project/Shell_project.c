/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;             /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;				/* info processed by analyze_status() */

    while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
    {
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

        if(args[0]==NULL) continue;   // if empty command

        pid_fork = fork();

        if(pid_fork < 0)
        {
            printf("ERROR trying to fork.");
            exit(1);
        } else if(pid_fork == 0)
        {
            execvp(args[0], args);
            printf("ERROR, command %s not found.\n", args[0]);
            exit(1);
        } else
        {
            if(background == 0)
            {
                waitpid(pid_fork, &status, 0);

                if(WEXITSTATUS(status) == 0)
                {
                    status_res = analyze_status(waitpid(pid_wait, &status, 0), &info);
                    printf("Foreground pid: %d, command: %s, %d, info: %d\n", pid_fork, args[0], status_res, info);
                } else
                {
                    printf("Background job running... pid: %d command: %s\n", pid_fork, args[0]);
                }
            }
        }
    }
}
