#include "config.hpp"

#include "divide.hpp"
#include "getopt.hpp"
#include "model.hpp"
#include "progress_bar.hpp"
#include "rc.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>

// GLOBAL STATE
//
// Command line options are stored in global variables:

int command     = 0;   // 'c' or 'd'
int memoryLimit = 128; // memory limit in MiB
int orderLimit  = 4;   //  order limit in bytes

// COMPRESS AND DECOMPRESS
//
// The compressed file is prefixed with it's uncompressed length; this
// is why the program will not work with unseekable files.

void Compress(FILE * textFile, FILE * codeFile)
{
    fseek(textFile, 0, SEEK_END);
    U32 textLength = ftell(textFile);
    fseek(textFile, 0, SEEK_SET);

    putc(textLength >> 24, codeFile);
    putc(textLength >> 16, codeFile);
    putc(textLength >>  8, codeFile);
    putc(textLength >>  0, codeFile);

    ProgressBar bar;
    Encoder rc(codeFile);
    PPM ppm;
    for (U32 processed = 0; processed != textLength; ++processed)
    {
        bar.Update(processed, textLength, ppm.GetUsedMemory());

        U32 c = getc(textFile);
        for (U32 mask = 1 << 7; mask != 0; mask >>= 1)
        {
            U32 p1 = ppm.Predict();
            if (c & mask)
            {
                rc.Encode<1>(p1);
                ppm.Update<1>();
            }
            else
            {
                rc.Encode<0>(p1);
                ppm.Update<0>();
            }
            rc.Normalize();
        }
    }
    rc.FlushBuffer();
    bar.Finish(textLength, ftell(codeFile), ppm.GetUsedMemory());
}

void Decompress(FILE * codeFile, FILE * textFile)
{
    U32 textLength = 0;
    textLength += getc(codeFile) << 24;
    textLength += getc(codeFile) << 16;
    textLength += getc(codeFile) <<  8;
    textLength += getc(codeFile) <<  0;

    ProgressBar bar;
    Decoder rc(codeFile);
    PPM ppm;
    rc.FillBuffer();
    for (U32 processed = 0; processed != textLength; ++processed)
    {
        bar.Update(processed, textLength, ppm.GetUsedMemory());

        U32 c = 0;
        for (U32 mask = 1 << 7; mask != 0; mask >>= 1)
        {
            U32 p1 = ppm.Predict();
            if (rc.Decode(p1))
            {
                ppm.Update<1>();
                c |= mask;
            }
            else
            {
                ppm.Update<0>();
            }
            rc.Normalize();
        }
        putc(c, textFile);
    }
    bar.Finish(textLength, ftell(codeFile), ppm.GetUsedMemory());
}

int main(int argc, char ** argv)
{
    bool help = false;
    bool version = false;

    int c;
    while ((c = getopt(argc, argv, "hVvqm:O:")) != -1)
    {
        if      (c == 'h') help    = true;
        else if (c == 'V') version = true;
        else if (c == 'm' || c == 'O')
        {
            errno = 0;
            char * rest;
            long val = strtol(optarg, &rest, 10);
            if (errno != 0 || *rest != '\0' || val < 0)
            {
                fprintf(stderr,
                        "%s: invalid argument '%s' for option '%c'\n",
                        argv[0], optarg, c);
                return 1;
            }
            if (c == 'm') memoryLimit = val;
            else          orderLimit  = val;
        }
        else return 1;
    }

    help = help || (!version && optind == argc);

    if (help)
    {
        puts("To compress a file invoke the program like this\n"
             "  crook c INPUT OUTPUT\n"
             "To decompress\n"
             "  crook d INPUT OUTPUT\n"
             "Existing output files are overwritten.\n"
             "\n"
             "Options:\n"
             "  -h   print this message\n"
             "  -V   print program version\n"
             "  -mN  use at most N megabytes of memory (default: 128)\n"
             "  -ON  use at most N previous bytes as context (default: 4)\n"
             "Options may be specified anywhere on the command line.\n"
             "\n"
             "Warning: identical options must be passed both when compressing and\n"
             "when decompressing, otherwise decompression will fail silently.\n");
    }

    if (help || version)
    {
        puts("crook 0.1 by Jüri Valdmann <juri.valdmann@gmail.com>");
        return 0;
    }


    if ((argv[optind][0] != 'c' && argv[optind][0] != 'd') ||
        argv[optind][1] != 0)
    {
        fprintf(stderr, "%s: unrecognized command '%s'\n",
	       	argv[0], argv[optind]);
        return 1;
    }

    if (optind + 2 >= argc)
    {
	fprintf(stderr, "%s: not enough arguments given\n", argv[0]);
        return 1;
    }

    command = argv[optind][0];

    FILE * input = fopen(argv[optind+1], "rb");
    if (input == NULL)
    {
	fprintf(stderr, "%s: cannot open '%s' (%s)\n",
		argv[0], argv[optind+1], strerror(errno));
        return 1;
    }

    FILE * output = fopen(argv[optind+2], "wb");
    if (output == NULL)
    {
	fprintf(stderr, "%s: cannot open '%s' (%s)\n",
		argv[0], argv[optind+2], strerror(errno));
        return 1;
    }

    if (command == 'c')
        Compress(input, output);
    else
        Decompress(input, output);

    if (ferror(input))
    {
        fprintf(stderr, "%s: cannot read from '%s' (%s)\n",
                argv[0], argv[optind+1], strerror(errno));
        return 1;
    }

    if (ferror(output))
    {
        fprintf(stderr, "%s: cannot write to '%s' (%s)\n",
                argv[0], argv[optind+2], strerror(errno));
        return 1;
    }
}
