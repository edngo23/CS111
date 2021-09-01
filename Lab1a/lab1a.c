// NAME: Ethan Ngo
// EMAIL: ethan.ngo2019@gmail.com
// ID: 205416130

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>

//ASCII Codes
#define CARRIAGE_RETURN '\r'
#define LINE_FEED       '\n'
#define END_OF_FILE     0x04 //^D
#define INTERRUPT       0x03 //^C

//much of the code is adapted from Disc 1B Week 2
void terminalSetup(void)
{
    struct termios nonCanonTerm;
    if(tcgetattr(0, &nonCanonTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to get termios settings into nonCanonTerm.");
        exit(1);
    }

    nonCanonTerm.c_iflag = ISTRIP;
    nonCanonTerm.c_oflag = 0;
    nonCanonTerm.c_lflag = 0;
    if(tcsetattr(0, TCSANOW, &nonCanonTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to set termios settings.");
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    struct termios origTerm;
    if(tcgetattr(0, &origTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to get termios settings into origTerm.");
        exit(1);
    }
    char opt;
    int shellFlag = 0, debugFlag = 0;

    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"shell", no_argument, 0, 's'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        if((opt = getopt_long(argc, argv, "sd", commandOptions, NULL)) != -1)
        {
            switch(opt)
            {
                case 's':
                    shellFlag = 1;
                    break;
                case 'd':
                    debugFlag = 1;
                    break;
                default:
                    fprintf(stderr, "Unrecognized Argument. Valid Arguements are --shell, --debug, or no arguments. \n");
                    exit(1);
            }
        }
        else
        {
            break;
        }
    }

    terminalSetup();

    if(shellFlag)
    {
        if(debugFlag)
        {
            printf("I'm in the shell.\r\n");
        }
        int toShell[2], fromShell[2];
        if(pipe(toShell) == -1)
        {
            fprintf(stderr, "Error: Piping failed.");
            exit(1);
        }
        if(pipe(fromShell) == -1)
        {
            fprintf(stderr, "Error: Piping failed.");
            exit(1);
        }

        int status;
        int ret = fork();
        if(ret == 0)
        {
            if(debugFlag)
            {
                printf("I'm in child process.\r\n");
            }
            close(toShell[1]);
            dup2(toShell[0], 0);

            close(fromShell[0]);
            dup2(fromShell[1], 1);
            dup2(fromShell[1], 2);

            execlp("/bin/bash", "shell", (char*) NULL);
        }
        else if(ret > 0)
        {
            if(debugFlag)
            {
                printf("I'm in parent process.\r\n");
            }
            char buf[256];
            int val;
            int eof = 0, shellInputDone = 1;
            close(toShell[0]);
            close(fromShell[1]);
            struct pollfd pollfds[2];

            pollfds[0].fd = 0;
            pollfds[0].events = POLLIN | POLLHUP | POLLERR;
            pollfds[1].fd = fromShell[0];
            pollfds[1].events = POLLIN | POLLHUP | POLLERR;

            if(debugFlag)
            {
                printf("Polling setup complete. EOF: %d Shell: %d \r\n", eof, shellInputDone);
            }
            
            while(!eof)
            {
                poll(pollfds, 2, -1);
                //keyboard input
                if(pollfds[0].revents & POLLIN)
                {
                    val = read(pollfds[0].fd, buf, 256);
                    if(debugFlag)
                    {
                        printf("Keyboard Input Val: %d.\r\n", val);
                    }
                    for(int i = 0; i < val; i++)
                    {
                        if(buf[i] == CARRIAGE_RETURN || buf[i] == LINE_FEED)
                        {
                            write(1, "\r\n", sizeof("\r\n"));
                            write(toShell[1], "\n", sizeof("\n"));
                            if(debugFlag)
                            {
                                printf("Keyboard: Enter hit after %d \r\n", shellInputDone);
                            }
                        }
                        else if(buf[i] == END_OF_FILE)
                        {
                            if(debugFlag)
                            {
                                printf("Keyboard EOF.\r\n");
                            }
                            eof = 1;
                            if(close(toShell[1]) < 0)
                            {
                                fprintf(stderr, "Error: Closing toShell[1] failed.");
                                exit(1);
                            }
                            break;
                        }
                        else if(buf[i] == INTERRUPT)
                        {
                            if(debugFlag)
                            {
                                printf("Keyboard Interrupt.\r\n");
                            }
                            if(kill(ret, SIGINT) < 0)
                            {
                                fprintf(stderr, "Error: Killing failed.");
                                exit(1);
                            }
                            break;
                        }
                        else
                        {
                            if(debugFlag)
                            {
                                printf("Keyboard Write.\r\n");
                            }
                            write(1, buf + i, 1);
                            write(toShell[1], buf + i, 1);
                        }
                    }
                }
                //shell pipe input
                if(pollfds[1].revents & POLLIN)
                {
                    shellInputDone = 0;
                    val = read(pollfds[1].fd, buf, 256);
                    if(debugFlag)
                    {
                        printf("Shell Input Val: %d.\r\n", val);
                    }
                    for(int i = 0; i < val; i++)
                    {
                        if(buf[i] == LINE_FEED)
                        {
                            if(debugFlag)
                            {
                                printf("Shell: Enter hit %d \r\n", shellInputDone);
                            }
                            write(1, "\r\n", sizeof("\r\n"));
                        }
                        else if(buf[i] == END_OF_FILE)
                        {
                            if(debugFlag)
                            {
                                printf("Shell EOF.\r\n");
                            }
                            eof = 1;
                            shellInputDone = 1;
                            if(close(toShell[1]) < 0)
                            {
                                fprintf(stderr, "Error: Closing toShell[1] failed.");
                                exit(1);
                            }
                            break;
                        }
                        else if(buf[i] == INTERRUPT)
                        {
                            if(debugFlag)
                            {
                                printf("Shell Interrupt.\r\n");
                            }
                             if(kill(ret, SIGINT) < 0)
                            {
                                fprintf(stderr, "Error: Killing failed.");
                                exit(1);
                            }
                            shellInputDone = 1;
                            break;
                        }
                        else
                        {
                            if(debugFlag)
                            {
                                printf("Shell Write.\r\n");
                            }
                            write(1, buf + i, 1);
                        }
                    }
                }
                if(pollfds[0].revents & (POLLHUP | POLLERR))
                {
                    if(debugFlag)
                    {
                        printf("POLLHUP OR POLLERR.\r\n");
                    }
                    break;
                }
            }
            close(toShell[1]);
            if(waitpid(ret, &status, 0) < 0)
            {
                fprintf(stderr, "Error: Waitpid failed.");
                exit(1);
            }
            if(WIFEXITED(status))
            {
                int signalStatus = WTERMSIG(status);
                int exitStatus = WEXITSTATUS(status);
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", signalStatus, exitStatus);
            }
            tcsetattr(0, TCSANOW, &origTerm);
            exit(0);
        }
        else
        {
            fprintf(stderr, "Error: Forking failed.");
            exit(1);
        }
        // if(waitpid(ret, &status, 0) < 0)
        // {
        //     fprintf(stderr, "Error: Waitpid failed.");
        //     exit(1);
        // }
        // if(WIFEXITED(status))
        // {
        //     int signalStatus = WTERMSIG(status);
        //     int exitStatus = WEXITSTATUS(status);
        //     fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", signalStatus, exitStatus);
        // }
        // tcsetattr(0, TCSANOW, &origTerm);
        // exit(0);
    }

    //buffer and stdout
    char readWriteBuffer[256];
    int len;
    while(1)
    {
        len = read(0, readWriteBuffer, 256);
        if(len < 0)
        {
            fprintf(stderr, "Error: Unable to read stdin.");
            exit(1);
        }
        for(int i = 0; i < len; i++)
        {
            if(readWriteBuffer[i] == END_OF_FILE)
            {
                if(write(1, "^D", sizeof("^D")) < 0)
                {
                    fprintf(stderr, "Error: Unable to write to stdin.");
                    exit(1);
                }
                tcsetattr(0, TCSANOW, &origTerm);
                exit(0);
            }
            else if(readWriteBuffer[i] == CARRIAGE_RETURN || readWriteBuffer[i] == LINE_FEED)
            {
                if(write(1, "\r\n", sizeof("\r\n")) < 0)
                {
                    fprintf(stderr, "Error: Unable to write to stdin.");
                    exit(1);
                }
            }
        }
        if(write(1, readWriteBuffer, len) < 0)
        {
            fprintf(stderr, "Error: Unable to write to stdin.");
            exit(1);
        }
    }

    return 0;
}
