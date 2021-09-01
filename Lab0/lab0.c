// NAME: Ethan Ngo
// EMAIL: ethan.ngo2019@gmail.com
// ID: 205416130


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

void segFaultRoutine()
{
    char* segPointer = NULL; //set pointer to null
    *segPointer = 'f'; //store through the null pointer
}

void catch()
{
    fprintf(stderr, "Segmentation Fault Caught.\n");
    exit(4);
}

int main(int argc, char* argv[])
{
    char opt;
    int optionIndex = 0;
    int inputFlag = 0, outputFlag = 0, segFaultFlag = 0, signalFlag = 0;
    char* inputFile;
    char* outputFile;
    //need struct of all options for getopt_long (from man page)
    //has to be array of structs to put all the possibilities
    static struct option longOptions[] =
    {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"segfault", no_argument, 0, 's'},
        {"catch", no_argument, 0, 'c'}
    };
    while(1)
    {
        //adapted from getopt(3) man page example but we need getopt_long because
        // of the need to accept '--' as valid
        if((opt = getopt_long(argc, argv, "i:o:sc", longOptions, &optionIndex)) != -1)
        {
            switch(opt)
            {
                //input/output redirection straight from the File Descriptor Management page
                case 'i':
                    inputFlag = 1;
                    inputFile = optarg;
                    break;
                case 'o':
                    outputFlag = 1;
                    outputFile = optarg;
                    break;
                case 's':
                    segFaultFlag = 1;
                    break;
                case 'c':
                    signalFlag = 1;
                    break;
                default:
                    fprintf(stderr, "Unrecognized Argument. Valid Arguements are --input=filename, --output=filename, --segfault, and --catch \n");
                    exit(1);
            }
        }
        else
        {
            break;
        }
    }
     
    //Carry actions out in the specified order from the specs
    //1. file redirection
    if(inputFlag)
    {
        int ifd = open(inputFile, O_RDONLY);
            if (ifd >= 0) {
                close(0);
                dup(ifd);
                close(ifd);
            }
            else
            {
                fprintf(stderr, "Unable to open file '%s' due to error '%s'\n", inputFile, strerror(errno));
                exit(2);
            }
        inputFlag = 0;
    }

    if(outputFlag)
    {
        int ofd = creat(outputFile, 0666);
        if (ofd >= 0) {
            close(1);
            dup(ofd);
            close(ofd);
        }
        else
        {
            fprintf(stderr, "Unable to create file '%s' due to error '%s'\n", outputFile, strerror(errno));
            exit(3);
        }
        outputFlag = 0;
    }

    //2. register the signal handler
    if(signalFlag)
    {
        signal(SIGSEGV, catch);
    }

    //3. cause the segfault
    if(segFaultFlag)
    {
        segFaultRoutine();
    }

    //4. copy stdin to stdout
    //Assuming parsing finished and there were no errors, begin copying standard input and write it to standard output
    char buffer[1];
    ssize_t readStatus = read(0, &buffer, 1);
    while(readStatus > 0)
    {
        write(1, &buffer, 1);
        readStatus = read(0, &buffer, 1);
    }

    exit(0);
}