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
    block_SIGCHLD();

    int num = 1;

    if(list->next == NULL)
    {
        printf("ERROR, no processes suspended or in background.\n");
    } else if(args[1] != NULL && (list_size(list) < atoi(args[1]) || 0 > atoi(args[1])))
    {
        printf("ERROR, the list has less processes than the needed ones.\n");
    } else
    {
        if(args[1] != NULL)
        {
            num = atoi(args[1]);
        }

        job* plist = get_item_bypos(list, num);
        plist->state = BACKGROUND;
        killpg(plist->pgid, SIGCONT);
    }
    unblock_SIGCHLD();
}

static void fg(char* args[])
{
    block_SIGCHLD();

    int num = 1, pid_wait, status;

    if(list->next == NULL)
    {
        printf("ERROR, no processes suspended or in background.\n");
    } else if (args[1] != NULL && (list_size(list) < atoi(args[1]) || 0 > atoi(args[1])))
    {
        printf("ERROR, the list has less processes than the needed ones.\n");
    }else
    {
        if(args[1] != NULL) {
            num = atoi(args[1]);
        }

        job* plist = get_item_bypos(list, num);
        set_terminal(plist->pgid);
        plist->state = FOREGROUND;
        killpg(plist->pgid, SIGCONT);
        pid_wait = waitpid(plist->pgid, &status, WUNTRACED);

        if(pid_wait == plist->pgid)
        {
            printf("Process %s has finished.\n", plist->command);
            num = delete_job(list, plist);
        }

        set_terminal(getpid());
    }
    unblock_SIGCHLD();
}

static void jobs()
{
    block_SIGCHLD();

    if(empty_list(list) != 1)
    {
        print_job_list(list);
    } else
    {
        printf("The list is empty.\n");
    }
    unblock_SIGCHLD();
}

static void handler ()
{
    enum status status_res;
    int stat, info, pid_aux;
    job *aux1, *aux2;

    if(empty_list(list) != 1)
    {
        aux2 = list;
        aux1 = aux2->next;

        while(aux1 != NULL)
        {
            pid_aux = waitpid(aux1->pgid, &stat, WNOHANG|WUNTRACED);

            if(pid_aux == aux1->pgid)
            {
                status_res = analyze_status(stat, &info);

                if (status_res != SUSPENDED) {
                    printf("The process %d has finished successfully.\n", aux1->pgid);
                    delete_job(list, aux1);
                } else {
                    printf("The process %d is suspended.\n", aux1->pgid);
                    aux1->state = STOPPED;
                    aux1 = aux1->next;
                }
            } else
            {
                aux1 = aux1->next;
            }
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
        printf("\nCOMMAND->");
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
               printf("Error, Command not found: %s\n",args[0]);
               exit(1);
           } else
           {
               if(!background) {
                   set_terminal(pid_fork);
                   pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                   status_res = analyze_status(status, &info);
                   set_terminal(getpid());

                   if (info != 1) {
                       printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_strings[status_res], info);
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
