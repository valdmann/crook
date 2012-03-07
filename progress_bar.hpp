// THE PROGRESS BAR
//
// This is a progress bar.  A progress bar is used to visually
// represent the relative amount of progress that has been done in the
// doing of a task that has not yet been done.
//
// In this case, data compression.

#ifndef PROGRESS_BAR_HPP
#define PROGRESS_BAR_HPP

#include "config.hpp"

#include <algorithm>
#include <ctime>

extern int command;
extern int memoryLimit;

class ProgressBar
{
    static const int period = 1 << 18;
    clock_t start;
    void Display(U32 processed, U32 total, U32 memory)
    {
        int percentage = (processed + total/100/2) / (total/100);
        printf("\r%3d%% ", percentage);

        const char blocks[] = "[########################################]";
        const char spaces[] = "[                                        ]";
        int maxBlocks = 40;
        int numBlocks = (processed + total/maxBlocks/2) / (total/maxBlocks);
        int fromBlocks = numBlocks + 1;
        int fromSpaces = maxBlocks + 1 - numBlocks;
        fwrite(blocks             , fromBlocks, 1, stdout);
        fwrite(spaces + fromBlocks, fromSpaces, 1, stdout);

        clock_t current = clock();
        int speed = processed / 1024 * CLOCKS_PER_SEC / (current - start + 1);
        printf("%6d kiB/s %d/%d MiB", speed, memory, memoryLimit);

        fflush(stdout);
    }
public:
    ProgressBar()
    {
        start = clock();
    }

    void Update(U32 processed, U32 total, U32 memory)
    {
        if (processed % period == 0)
            Display(processed, total, memory);
    }

    void Finish(U32 textLength, U32 codeLength, U32 memory)
    {
        Display(textLength, textLength, memory);

        double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
        double bpc = 8.0 * codeLength / textLength;

        if (command == 'd')
            swap(textLength, codeLength);

        putc('\n', stdout);
        printf("%d -> %d, %.2f s, %.3f bpc.\n",
               textLength, codeLength, seconds, bpc);
    }
};

#endif
