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
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job* list;

static void cd(char *args[])
{
    if(args[1] == NULL)
    {
        printf("ERROR, no directory found.\n");
    }else if(args[2] != NULL)
    {
        printf("ERROR with the directory name format.\n");
    } else
    {
        if(chdir(args[1]) == 0)
        {
            if(strcmp(args[1], "..") == 0)
            {
                printf("Returning to the previous directory.\n");
            } else
            {
                printf("Accessing to the directory %s.\n", args[1]);
            }

        } else
        {
            printf("ERROR, directory %s not found.\n", args[1]);
        }
    }
}

static void bg(char* args[])
{
    int num;

    if(list->next == NULL)
    {
        printf("ERROR, no processes suspended or in background.\n");
    } else
    {
        if(args[1] != NULL)
        {
            num = atoi(args[1]);
        } else{
            num = 1;
        }

        job* plist = get_item_bypos(list, num);
        plist->state = BACKGROUND;
        killpg(plist->pgid, SIGCONT);
    }
}

static void fg(char* args[])
{
    int num, pid_wait, status;

    if(list->next == NULL)
    {
        printf("ERROR, no processes suspended or in background.\n");
    } else
    {
        if(args[1] == NULL)
        {
            num = atoi(args[1]);
        } else{
            num = 1;
        }

        job* plist = get_item_bypos(list, num);
        set_terminal(plist->pgid);
        plist->state = FOREGROUND;
        killpg(list->pgid, SIGCONT);
        pid_wait = waitpid(plist->pgid, &status, WUNTRACED);

        if(pid_wait == plist->pgid)
        {
            printf("Process %s has finished.\n", plist->command);
            delete_job(list, plist);
        }

        set_terminal(getpid());
    }
}

static void jobs()
{
    if(empty_list(list) != 1)
    {
        print_job_list(list);
    } else
    {
        printf("The list is empty.\n");
    }
}

static void handler ()
{
    enum status status_res;
    int exStat, info, hasToBeDeleted;
    pid_t pid_aux;
    job* plist = list->next;

    while(plist != NULL)
    {
        hasToBeDeleted = 0;
        pid_aux = waitpid(plist->pgid, &exStat, WNOHANG | WUNTRACED);

        if(pid_aux == plist->pgid)
        {
            status_res = analyze_status(exStat, &info);

            if(status_res != SUSPENDED)
            {
                hasToBeDeleted = 1;
                printf("The process %d has finished successfully.\n", plist->pgid);
            } else
            {
                printf("The process %d is suspended.\n", plist->pgid);
                plist->state = STOPPED;
            }

        }

        if(hasToBeDeleted)
        {
            struct job_* jobToDelete = plist;
            plist = plist->next;
            delete_job(list, jobToDelete);
        } else
        {
            plist = plist->next;
        }
    }
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void)
{
    list = new_list("Jobs");

    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */

    // probably useful variables:

    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;             /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;   			/* info processed by analyze_status() */

    ignore_terminal_signals();

    while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
    {
        signal(SIGCHLD, handler);
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

        if(args[0]==NULL)
        {
            continue;   // if empty command
        }

       if(strcmp(args[0], "cd") == 0)
       {
           cd(args);
       } else if(strcmp(args[0], "jobs") == 0)
       {
           jobs();
       } else if(strcmp(args[0], "fg") == 0)
       {
           fg(args);
       } else if(strcmp(args[0], "bg") == 0)
       {
           bg(args);
       } else
       {
           block_SIGCHLD();
           pid_fork = fork();

           if(pid_fork < 0)
           {
               printf("ERROR trying to fork.\n");
               exit(1);
           } else if(pid_fork == 0)
           {
               unblock_SIGCHLD();
               new_process_group(getpid());

               if(!background)
               {
                   set_terminal(getpid());
               }

               restore_terminal_signals();
               execvp(args[0], args);
               exit(1);
           } else
           {
               if(!background) {
                   set_terminal(pid_fork);
                   pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                   status_res = analyze_status(status, &info);
                   set_terminal(getpid());

                   if (info != 1) {
                       printf("Foreground pid: %d, command: %s, %d, info: %d\n", pid_fork, args[0], status_res, info);
                   }

                   if (status_res == SUSPENDED) {
                       add_job(list, new_job(pid_fork, args[0], STOPPED));
                       printf("Process (pid: %d, command: %s) has been suspended.\n", getpid(), args[0]);
                       unblock_SIGCHLD();
                   }

               } else
               {
                   add_job(list, new_job(pid_fork, args[0], BACKGROUND));
                   printf("Background job running... pid: %d command: %s\n", pid_fork, args[0]);
                   unblock_SIGCHLD();
               }
           }
       }
    }
}
