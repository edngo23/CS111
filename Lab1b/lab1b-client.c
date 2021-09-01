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
#include <ulimit.h>

#define CHUNK 128000

//ASCII Codes
#define CARRIAGE_RETURN '\r'
#define LINE_FEED       '\n'
#define END_OF_FILE     0x04 //^D
#define INTERRUPT       0x03 //^C

struct termios origTerm;

//atexit requires a void parameter, so I did this to solve it
void terminalRestore()
{
    tcsetattr(0, TCSANOW, &origTerm);
}

//much of the code is adapted from Disc 1B Week 2
void terminalSetup(void)
{
    struct termios nonCanonTerm;
    if(tcgetattr(0, &nonCanonTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to get termios settings into nonCanonTerm.");
        exit(1);
    }
    if(tcgetattr(0, &origTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to get termios settings into origTerm.");
        exit(1);
    }
    atexit(terminalRestore);

    nonCanonTerm.c_iflag = ISTRIP;
    nonCanonTerm.c_oflag = 0;
    nonCanonTerm.c_lflag = 0;
    if(tcsetattr(0, TCSANOW, &nonCanonTerm) < 0)
    {
        fprintf(stderr, "Error: Failed to set termios settings.");
        exit(1);
    }
}

//taken directly from Disc 1B Week 4 Slides
/* include all the headers */
int client_connect(char* hostname, unsigned int port) {
    /* e.g. host_name:”google.com”, port:80, return the socket for subsequent communication */
    int sockfd;
    struct sockaddr_in serv_addr; /* server addr and port info */
    struct hostent* server;
    //1. create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        fprintf(stderr, "Error: Failed to create socket in client. \r\n");
        exit(1);
    }
    //2. fill in socket address information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (port);
    server = gethostbyname(hostname); /* convert host_name to IP addr */
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length); /* copy ip address from server to serv_addr */
    memset(serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero); /* padding zeros*/
    //3. connect socket with corresponding address
    if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error: The port does not have an active server. \r\n");
        exit(1);
    }
    return sockfd;
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
    int portFlag = 0, logFlag = 0, compressFlag = 0, debugFlag = 0;
    int portNumber, socketFd, logFd;
    int compSize;

    //adapted from lab0.c and getopt(3) man page
    static struct option commandOptions[] =
    {
        {"port", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"compress", no_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while(1)
    {
        if((opt = getopt_long(argc, argv, "p:l:cd", commandOptions, NULL)) != -1)
        {
            switch(opt)
            {
                case 'p':
                    portNumber = atoi(optarg);
                    portFlag = 1;
                    break;
                case 'l':
                    logFlag = 1;
                    logFd = open(optarg, O_RDWR | O_CREAT | O_TRUNC, 0777);
                    break;
                case 'c':
                    compressFlag = 1;
                    break;
                case 'd':
                    debugFlag = 1;
                    break;
                default:
                    fprintf(stderr, "Unrecognized Argument. Valid usage is --port=portNumber [--log=fileName][--compress][--debug]. \n");
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

    socketFd = client_connect("localhost", portNumber);

    terminalSetup();

    if(1)
    {
        char buf[CHUNK], compBuf[CHUNK];
        int val;
        struct pollfd pollfds[2];

        pollfds[0].fd = 0;
        pollfds[0].events = POLLIN + POLLHUP + POLLERR;
        pollfds[1].fd = socketFd; //fromShell[0]
        pollfds[1].events = POLLIN + POLLHUP + POLLERR;

        if(debugFlag)
        {
            printf("Polling setup complete.\r\n");
        }
            
        while(1)
        {
            poll(pollfds, 2, -1);
            //keyboard input (stdin)
            if(pollfds[0].revents & POLLIN)
            {
                val = read(pollfds[0].fd, buf, CHUNK);
                if(compressFlag)
                {
                    def(val, (unsigned char*) buf, &compSize, (unsigned char*) compBuf);
                    // for(int i = 0; i < val; i++)
                    // {
                    //     write(1, buf + i, 1);
                    // }
                    //write(1, buf, val);
                    // for(int i = 0; i < compSize; i++)
                    // {
                    //     write(socketFd, compBuf + i, 1);
                    // }
                    write(socketFd, compBuf, compSize);
                }
                //no compression
                else
                {
                    for(int i = 0; i < val; i++)
                    {
                        write(1, buf + i, 1);
                        if(write(socketFd, buf + i, 1) < 0)
                        {
                            exit(0);
                        }
                    }
                }
                if(logFlag)
                {
                    ulimit(UL_SETFSIZE, 10000);
                    dprintf(logFd, "SENT %d bytes: ", compSize);
                    write(logFd, compBuf, compSize);
                    dprintf(logFd, "\n");
                }
            }
            //socketFd
            if(pollfds[1].revents & POLLIN)
            {
                if(compressFlag)
                {
                    val = read(pollfds[1].fd, buf, CHUNK);
                    inf(val,(unsigned char*) buf, &compSize,(unsigned char*) compBuf);
                    if(val == 0)
                    {
                        exit(0);
                    }
                    for(int i = 0; i < val; i++)
                    {
                        write(1, compBuf + i, 1);
                    }
                }
                //no compression
                else
                {
                    val = read(pollfds[1].fd, buf, CHUNK);
                    if(val == 0)
                    {
                        exit(0);
                    }
                    for(int i = 0; i < val; i++)
                    {
                        write(1, buf + i, 1);
                    }
                }
                if(logFlag)
                {
                    ulimit(UL_SETFSIZE, 10000);
                    dprintf(logFd, "RECEIVED %d bytes: ", compSize);
                    write(logFd, compBuf, compSize);
                    dprintf(logFd, "\n");
                }
            }
            if(pollfds[0].revents & (POLLHUP | POLLERR))
            {
                if(debugFlag)
                {
                    printf("Keyboard POLLHUP OR POLLERR.\r\n");
                }
                break;
            }
            if(pollfds[1].revents & (POLLHUP | POLLERR))
            {
                if(debugFlag)
                {
                    printf("Server POLLHUP OR POLLERR.\r\n");
                }
                exit(1);
                break;
            }
        }
        shutdown(socketFd, SHUT_RDWR);
        close(socketFd);
        exit(0);
    }
    return 0;
}
