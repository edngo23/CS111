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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <zlib.h>

#define CHUNK 128000

//ASCII Codes
#define CARRIAGE_RETURN '\r'
#define LINE_FEED       '\n'
#define END_OF_FILE     0x04 //^D
#define INTERRUPT       0x03 //^C

//maps cr or lf to crlf
void mapToCrlf(char buf[], int* readSize, int bufSize)
{
    char temp[bufSize];
    int pos = 0;
    for(int i = 0; i < (*readSize); i++, pos++)
    {
        if(buf[i] == LINE_FEED)
        {
            temp[pos] = CARRIAGE_RETURN;
            pos++;
            temp[pos] = LINE_FEED;
        }
        else
        {
            temp[pos] = buf[i];
        }
    }
    memcpy(buf, temp, pos);
    *readSize = pos;
}

int server_connect(unsigned int port_num)
/* port_num: which port to listen, return socket for subsequent communication*/
{
    int sockfd, new_fd; /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr; /* my address */
    struct sockaddr_in their_addr; /* connector addr */
    socklen_t sin_size; //I HOPE IT IS SOCKLEN_T
    /* create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        fprintf(stderr, "Error: Failed to create socket in client. \r\n");
        exit(1);
    }
    /* set the address info */
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_num); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;
    /* INADDR_ANY allows clients to connect to any one of the host’s IP address. */
    memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero)); //padding zeros
    /* bind the socket to the IP address and port number */
    bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));

    listen(sockfd, 5); /* maximum 5 pending connections */
    sin_size = sizeof(struct sockaddr_in);
    /* wait for client’s connection, their_addr stores client’s address */
    new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
    return new_fd; /* new_fd is returned not sock_fd*/
}

//adapted from zlib example
int def(int inSize, unsigned char in[CHUNK], int* outSize, unsigned char out[CHUNK])
{
    int ret;
    unsigned temp;
    z_stream strm;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = inSize;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        deflate(&strm, Z_FINISH);    /* no bad return value */
        temp = CHUNK - strm.avail_out;
    } while (strm.avail_out == 0);
    *outSize = temp;

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

int inf(int inSize, unsigned char in[CHUNK], int* outSize, unsigned char out[CHUNK])
{
    int ret;
    unsigned temp;
    z_stream strm;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = inSize;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = inflate(&strm, Z_FINISH);    /* no bad return value */
        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
                //fall through
            case Z_DATA_ERROR:
                //fall through
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
        }
        temp = CHUNK - strm.avail_out;
    } while (strm.avail_out == 0);
    *outSize = temp;

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int main(int argc, char* argv[])
{
    char opt;
    int portFlag = 0, compressFlag = 0, debugFlag = 0;
    int portNumber, socketFd;

    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"port", required_argument, 0, 'p'},
        {"compress", no_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        if((opt = getopt_long(argc, argv, "sd", commandOptions, NULL)) != -1)
        {
            switch(opt)
            {
                case 'p':
                    portNumber = atoi(optarg);
                    portFlag = 1;
                    break;
                case 'c':
                    compressFlag = 1;
                    break;
                case 'd':
                    debugFlag = 1;
                    break;
                default:
                    fprintf(stderr, "Unrecognized Argument. Valid usage is --port=portNumber [--compress][--debug]. \n");
                    exit(1);
            }
        }
        else
        {
            break;
        }

        if(portFlag != 1)
        {
            fprintf(stderr, "Error: --port is required.\n");
            exit(1);
        }
    }

    socketFd = server_connect(portNumber);

    if(1)
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
            char buf[CHUNK];
            char compBuf[CHUNK];
            int compSize;
            int val;
            int eof = 0, intrpt = 0;
            close(toShell[0]);
            close(fromShell[1]);
            struct pollfd pollfds[2];

            pollfds[0].fd = socketFd;
            pollfds[0].events = POLLIN + POLLHUP + POLLERR;
            pollfds[1].fd = fromShell[0];
            pollfds[1].events = POLLIN + POLLHUP + POLLERR;

            if(debugFlag)
            {
                printf("Polling setup complete. EOF: %d Interrupt: %d \r\n", eof, intrpt);
            }
            
            while(!eof && !intrpt)
            {
                poll(pollfds, 2, -1);
                //keyboard input
                if(pollfds[0].revents & POLLIN)
                {
                    val = read(pollfds[0].fd, buf, CHUNK);
                    if(val == 0)
                    {
                        exit(0);
                    }
                    if(compressFlag)
                    {
                        inf(val,(unsigned char*) buf, &compSize,(unsigned char*) compBuf);
                        for(int i = 0; i < val; i++)
                        {
                            if(compBuf[i] == CARRIAGE_RETURN || compBuf[i] == LINE_FEED)
                            {
                                write(toShell[1], "\n", sizeof("\n"));
                            }
                            else if(compBuf[i] == END_OF_FILE)
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
                            else if(compBuf[i] == INTERRUPT)
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
                                intrpt = 1;
                                break;
                            }
                            else
                            {
                                write(toShell[1], compBuf + i, 1);
                            }
                        }
                        //write(toShell[1], compBuf, compSize);
                    }
                    //no compression
                    else
                    {
                        for(int i = 0; i < val; i++)
                        {
                            if(buf[i] == CARRIAGE_RETURN || buf[i] == LINE_FEED)
                            {
                                write(toShell[1], "\n", sizeof("\n"));
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
                                intrpt = 1;
                                break;
                            }
                            else
                            {
                                write(toShell[1], buf + i, 1);
                            }
                        }
                    }
                }
                //shell pipe input
                if(pollfds[1].revents & POLLIN)
                {
                    if(compressFlag)
                    {
                        val = read(pollfds[1].fd, buf, CHUNK);
                        if(val == 0)
                        {
                            exit(0);
                        }
                        for(int i = 0; i < val; i++)
                        {
                            if(buf[i] == END_OF_FILE)
                            {
                                if(debugFlag)
                                {
                                    printf("Shell EOF.\r\n");
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
                                    printf("Shell Interrupt.\r\n");
                                }
                                if(kill(ret, SIGINT) < 0)
                                {
                                    fprintf(stderr, "Error: Killing failed.");
                                    exit(1);
                                }
                                intrpt = 1;
                                break;
                            }
                        }
                        mapToCrlf(buf, &val, sizeof(buf));
                        def(val,(unsigned char*) buf, &compSize,(unsigned char*) compBuf);
                        write(socketFd, compBuf, compSize);
                    }
                    //no compresssion
                    else
                    {
                        val = read(pollfds[1].fd, buf, CHUNK);
                        if(val == 0)
                        {
                            exit(0);
                        }
                        for(int i = 0; i < val; i++)
                        {
                            if(buf[i] == LINE_FEED)
                            {
                                write(socketFd, "\r\n", sizeof("\r\n"));
                            }
                            else if(buf[i] == END_OF_FILE)
                            {
                                if(debugFlag)
                                {
                                    printf("Shell EOF.\r\n");
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
                                    printf("Shell Interrupt.\r\n");
                                }
                                if(kill(ret, SIGINT) < 0)
                                {
                                    fprintf(stderr, "Error: Killing failed.");
                                    exit(1);
                                }
                                intrpt = 1;
                                break;
                            }
                            else
                            {
                                write(socketFd, buf + i, 1);
                            }
                        }
                    }
                }
                if(pollfds[0].revents & (POLLHUP | POLLERR))
                {
                    if(debugFlag)
                    {
                        printf("0: POLLHUP OR POLLERR.\r\n");
                    }
                    break;
                }
                if(pollfds[1].revents & (POLLHUP | POLLERR))
                {
                    if(debugFlag)
                    {
                        printf("1: POLLHUP OR POLLERR.\r\n");
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
        }
        else
        {
            fprintf(stderr, "Error: Forking failed.");
            exit(1);
        }
    }

    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
    exit(0);
    return 0;
}
