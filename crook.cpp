//                         _
//     ___ _ __ ___   ___ | | __
//    / __| '__/ _ \ / _ \| |/ /
//   | (__| | | (_) | (_) |   <
//    \___|_|  \___/ \___/|_|\_\.
//
// This is "crook" - a simple experimental file compressor.
//
// LICENSE
//
// This software comes without any warranty.  Everyone is permitted to
// use and distribute this software or modified copies of this
// software for any purpose, commercial or non-commercial.

#define NDEBUG
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef __GLIBC__
#define putc putc_unlocked
#define getc getc_unlocked
#endif

#ifdef _MSC_VER
#define putc _fputc_nolock
#define getc _fgetc_nolock
#endif

using namespace std;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

const U32 ARI_P_BITS  = 12;
const U32 ARI_P_SCALE = 1 << ARI_P_BITS;

const U32 DIVISOR_BITS     = 10;
const U32 DIVISOR_LIMIT    = 1 << DIVISOR_BITS;
const U32 RECIPROCAL_BITS  = 15;
const U32 RECIPROCAL_LIMIT = 1 << RECIPROCAL_BITS;

const U32 PPM_P_BITS  = 22;
const U32 PPM_C_BITS  = 10;
const U32 PPM_P_SCALE = 1 << PPM_P_BITS;
const U32 PPM_C_LIMIT = 1 << PPM_C_BITS;
const U32 PPM_C_SCALE = 32;
const U32 PPM_P_MASK  = (PPM_P_SCALE  - 1) << PPM_C_BITS;
const U32 PPM_C_MASK  = PPM_C_LIMIT - 1;
const U32 PPM_P_START = PPM_P_SCALE / 2;
const U32 PPM_C_START = PPM_C_SCALE * 12;
const U32 PPM_C_INH   = PPM_C_SCALE * 3 / 2;
const U32 PPM_C_INC   = PPM_C_SCALE;

int command     = 0;
int memoryLimit = 128;
int orderLimit  = 4;

class ReciprocalTable
{
    U16 t[DIVISOR_LIMIT];
public:
    ReciprocalTable()
    {
        for (U32 n = 0; n < DIVISOR_LIMIT; ++n)
            t[n] = RECIPROCAL_LIMIT / (n + 2);
    }
    U32 operator[](U32 n)
    {
        assert(n < DIVISOR_LIMIT);
        return t[n];
    }
} reciprocals;

U32 Fit(U32 x, U32 n, U32 m)
{
    assert(x < (1 << n));
    return (n > m) ? x >> (n - m) : x << (m - n);
}

U32 Fit0(U32 x, U32 n, U32 m)
{
    assert(0 < x && x < (1 << n));
    return Fit(x, n, m) + 1 - (x >> (n - 1));
}

U32 Excess(U32 n, U32 m)
{
    return (n > m) ? n - m : 0;
}

U32 Divide(U32 x, U32 n, U32 y, U32 m)
{
    assert (x < (1 << n));
    assert (y < (1 << m));
    U32 dn = Excess(n, 32 - RECIPROCAL_BITS);
    U32 dm = Excess(m, DIVISOR_BITS);
    U32 dk = RECIPROCAL_BITS + dm - dn;
    return ((x >> dn) * reciprocals[y >> dm]) >> dk;
}

struct Node
{
    U32 ext0;
    U32 ext1;
    U32 sfx;
    U32 ctr;

    Node() {}

    Node(U32 ext0, U32 ext1, U32 sfx)
        : ext0(ext0),
          ext1(ext1),
          sfx(sfx),
          ctr((PPM_P_START << PPM_C_BITS) + PPM_C_START) {}

    Node(U32 sfx, const Node * sfxp)
        : ext0(0),
          ext1(0),
          sfx(sfx),
          ctr((sfxp->ctr & PPM_P_MASK) + PPM_C_INH) {}

    U32 Predict()
    {
        return ctr >> PPM_C_BITS;
    }

    template <bool bit> void Update()
    {
        U32 cnt = ctr  & PPM_C_MASK;
        U32 p1  = ctr >> PPM_C_BITS;

        if (cnt < PPM_C_LIMIT - PPM_C_INC)
            cnt += PPM_C_INC;
        else
            cnt = PPM_C_LIMIT-1;

        // alternative:
        // cnt += PPM_C_INC;
        // cnt = -(cnt >> PPM_C_BITS) | cnt;
        // cnt &= PPM_C_MASK;

        assert(cnt < PPM_C_LIMIT);

        if (bit)
            p1 += PPM_C_SCALE * Divide(PPM_P_SCALE - p1, PPM_P_BITS, cnt, PPM_C_BITS);
        else
            p1 -= PPM_C_SCALE * Divide(              p1, PPM_P_BITS, cnt, PPM_C_BITS);

        assert(0 < p1 && p1 < PPM_P_SCALE);

        ctr = (p1 << PPM_C_BITS) + cnt;
    }

    template <bool bit> U32 & Ext()
    {
        return bit ? ext1 : ext0;
    }
};

class PPM
{
    Node * nodes;
    Node * top;
    Node * end;

    Node * act;
    int order;

    const int nodesLimit;
    const int orderLimitBits;
public:
    PPM()
        : nodesLimit(memoryLimit * (1 << 20) / sizeof(Node)),
          orderLimitBits(8 * orderLimit + 7)
    {
        nodes = new Node[nodesLimit];
        end = nodes + nodesLimit;
        top = nodes;
        act = nodes + 1;
        order = 0;

        *top++ = Node(1, 1, 0);
        for (int dst = 2; dst != 256; dst += 2)
            *top++ = Node(dst, dst+1, 0);
        for (int i = 128; i != 256; ++i)
            *top++ = Node(0, 0, 0);
    }

    U32 Predict()
    {
        return Fit0(act->Predict(), PPM_P_BITS, ARI_P_BITS);
    }

    template <bool bit> void Update()
    {
        act->Update<bit>();

        Node * lst = act;
        while (act->Ext<bit>() == 0)
        {
            lst = act;
            act = nodes + act->sfx;
            order -= 8;
            act->Update<bit>();
        }

        if (act != lst && order+9 <= orderLimitBits && top < end)
        {
            Node * ext = nodes + act->Ext<bit>();
            lst->Ext<bit>() = top - nodes;
            *top = Node(ext - nodes, ext);
            act = top++;
            order += 9;
        }
        else
        {
            Node * ext = nodes + act->Ext<bit>();
            act = ext;
            order++;
        }
    }

    U32 GetUsedMemory()
    {
        return ((top - nodes) * sizeof(Node)) >> 20;
    }
};

class Encoder
{
    FILE * codeFile;
    U64 low;
    U32 range;
    U32 fluxLen;
    U8  fluxFst;
public:
    Encoder(FILE * codeFile)
        : codeFile(codeFile),
          low(0),
          range(0xFFFFFFFF),
          fluxLen(1),
          fluxFst(0) {}

    template <bool bit> void Encode(U32 p1)
    {
        assert(0 < p1 && p1 < ARI_P_SCALE);
        U32 mid = range / ARI_P_SCALE * p1;
        if (bit)
            range = mid;
        else
            low += mid, range -= mid;
    }

    void Normalize()
    {
        while (range <= 0xFFFFFF)
        {
            U32 lo32 = low, hi32 = low >> 32;
            if (lo32 < 0xFF000000 || hi32 != 0)
            {
                putc(fluxFst + hi32, codeFile);
                while (--fluxLen)
                    putc(0xFF + hi32, codeFile);
                fluxFst = lo32 >> 24;
            }
            ++fluxLen;
            low = (lo32 << 8);
            range <<= 8;
        }
    }

    void FlushBuffer()
    {
        U32 lo32 = low, hi32 = low >> 32;
        putc(fluxFst + hi32, codeFile);
        while (--fluxLen)
            putc(0xFF + hi32, codeFile);
        putc(lo32 >> 24, codeFile);
        putc(lo32 >> 16, codeFile);
        putc(lo32 >>  8, codeFile);
        putc(lo32 >>  0, codeFile);
    }
};

class Decoder
{
    FILE * codeFile;
    U32 range;
    U32 cml;
public:
    Decoder(FILE * codeFile)
        : codeFile(codeFile),
          range(0xFFFFFFFF),
          cml(0) {}

    void FillBuffer()
    {
        for (int i = 0; i < 5; ++i)
            cml = (cml << 8) + getc(codeFile);
    }

    bool Decode(U32 p1)
    {
        assert(0 < p1 && p1 < ARI_P_SCALE);
        U32 mid = range / ARI_P_SCALE * p1;
        if (cml < mid)
        {
            range = mid;
            return 1;
        }
        else
        {
            cml -= mid, range -= mid;
            return 0;
        }
    }

    void Normalize()
    {
        while (range <= 0xFFFFFF)
        {
            cml = (cml << 8) + getc(codeFile);
            range <<= 8;
        }
    }
};

class ProgressBar
{
    static const int period = 1 << 18;
    clock_t start;
public:
    ProgressBar()
    {
        start = clock();
    }

    void Display(U32 processed, U32 total, U32 memory)
    {
        if (processed % period != 0)
            return;

        int percentage = (100 * processed + total / 2) / total;
        printf("\r%3d%% ", percentage);

        const char blocks[] = "[########################################]";
        const char spaces[] = "[                                        ]";
        int maxBlocks = 40;
        int numBlocks = (maxBlocks * processed + total / 2) / total;
        int fromBlocks = numBlocks + 1;
        int fromSpaces = maxBlocks + 1 - numBlocks;
        fwrite(blocks             , fromBlocks, 1, stdout);
        fwrite(spaces + fromBlocks, fromSpaces, 1, stdout);

        clock_t current = clock();
        int speed = processed / 1024 * CLOCKS_PER_SEC / (current - start + 1);
        printf("%6d kiB/s %d/%d MiB", speed, memory, memoryLimit);

        fflush(stdout);
    }

    void Finish(U32 textLength, U32 codeLength)
    {
        double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
        double bpc = 8.0 * codeLength / textLength;

        if (command == 'd')
            swap(textLength, codeLength);

        putc('\n', stdout);
        printf("%d -> %d, %.2f s, %.3f bpc.\n",
               textLength, codeLength, seconds, bpc);
    }
};

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
        bar.Display(processed, textLength, ppm.GetUsedMemory());

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
    bar.Finish(textLength, ftell(codeFile));
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
        bar.Display(processed, textLength, ppm.GetUsedMemory());

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
    bar.Finish(textLength, ftell(codeFile));
}

char * optarg = NULL;
int    optind = 0;

// cmd -ab1 x y -cd2 z -e
//
// cmd              |       | -ab1  x y -cd2  z -e   --->   'a'
// cmd              |       | -a*b1 x y -cd2  z -e   --->   'b' 1
// cmd -ab1         |       |       x y -cd2  z -e   --->   'c'
// cmd -ab1         | x y   |           -c*d2 z -e   --->   'd' 2
// cmd -ab1 -cd2    | x y   |                 z -e   --->   'e'
// cmd -ab1 -cd2 -e | x y z |                        --->   -1

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
