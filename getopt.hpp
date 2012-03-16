// GETOPT
//
// Here I have implemented a function for parsing command-line
// arguments, a proposition so radical that neither the C nor the C++
// standards committees have had the foresight necessary to include
// such functionality in their respective standard libraries.
//
// This supports only a subset of GNU getopt's functionality.  The
// most important of these omissions is that it cannot parse separate
// arguments.  It does however shuffle the non-options to the end of
// argv.

#ifndef GETOPT_HPP
#define GETOPT_HPP

#include "config.hpp"

#include <algorithm>
#include <cstring>

char * optarg = NULL;
int    optind = 0;

// Example of a parse:
// cmd -ab1 x y -cd2 z -e
//
// cmd              |       | -ab1 x y -cd2 z -e   --->   'a'
// cmd              |       | -ab1 x y -cd2 z -e   --->   'b' "1"
// cmd -ab1         |       |   ^  x y -cd2 z -e   --->   'c'
// cmd -ab1         | x y   |    \     -cd2 z -e   --->   'd' "2"
// cmd -ab1 -cd2    | x y   |     \      ^  z -e   --->   'e'
// cmd -ab1 -cd2 -e | x y z |      \    /          --->   -1
//                  ^       ^       \  /
//                 /       /         \/
//           optind     end         next

int getopt(int argc, char ** argv, const char * spec)
{
    static char * next = NULL;
    static int end = 0;

    if (next == NULL || next[0] == '\0')
    {
        rotate(argv + optind, argv + end, argv + end + 1);
        ++optind;
        ++end;

        while (end != argc && (argv[end][0] != '-' || argv[end][1] == '\0'))
            ++end;

        if (end == argc)
            return -1;

        next = &argv[end][1];
    }

    const char * opt = strchr(spec, next[0]);

    if (opt == NULL)
    {
        fprintf(stderr, "%s: invalid option '-%c'\n",
                argv[0], next[0]);
        optarg = NULL;
        ++next;
        return '?';
    }
    else if (opt[1] == ':' && next[1] == '\0')
    {
        fprintf(stderr, "%s: missing argument for option '-%c'\n",
                argv[0], next[0]);
        optarg = NULL;
        next = NULL;
        return '?';
    }
    else if (opt[1] == ':' && next[1] != '\0')
    {
        optarg = next + 1;
        next = NULL;
        return opt[0];
    }
    else
    {
        optarg = NULL;
        ++next;
        return opt[0];
    }
}

#endif
