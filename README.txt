    ___ _ __ ___   ___ | | __
   / __| '__/ _ \ / _ \| |/ /
  | (__| | | (_) | (_) |   <
   \___|_|  \___/ \___/|_|\_\.

This is "crook" - a simple experimental file compressor.  Being
experimental you probably shouldn't use it to compress important data.

LICENSE
=======

This software comes without any warranty.  Everyone is permitted to
use and distribute this software or modified copies of this software
for any purpose, commercial or non-commercial.

COMPILATION
===========

When compiling with GCC try the following for maximum performance:
  g++ -O3 -s -fno-exceptions -finline-limit=10000 -fwhole-program
      crook.cpp -o crook

INVOCATION
==========

To compress a file invoke the program like this
  crook c INPUT OUTPUT
To decompress
  crook d INPUT OUTPUT
Existing output files are overwritten.

Options:
  -h   print this message
  -V   print program version
  -mN  use at most N megabytes of memory (default: 128)
  -ON  use at most N previous bytes as context (default: 4)
Options may be specified anywhere on the command line.

Warning: identical options must be passed both when compressing and
when decompressing, otherwise decompression will fail silently.

WHY PPM IS BETTER THAN DMC
==========================

If you're not familiar with the "modern data compression paradigm"
then I'd recommend skimming through "Text compression" by Bell, Cleary
& Witten [1].

Crook can be considered a variant of the PPM (prediction by partial
matching, [2]) algorithm, except it works with a binary alphabet and
so does not need to use escapes.  Instead of escapes it uses
information inheritance to blend statistics from different orders.
Indeed both escapes and inheritance can be considered different
optimizations of the context mixing or full blending model: escapes
make mixing faster and inheritance precomputes the mixture on state
creation.

Crook started life as a simple, vanilla implementation of DMC (dynamic
Markov compression, [3]).  In her PhD thesis [4] Suzanne Bunton
explores the theoretical properties of models built by DMC.  She
concludes that DMC, while theoretically capable of building more
expressive models than PPM, can in practice be a little bit flawed.

I wrote Crook to explore and experiment with DMC's model structure and
growth heuristics.  As a result of these experiments I can say that
most of the structural features of DMC that distinguish it from binary
PPM are in fact counterproductive.  To wit:

* DMC can have distinct states which correspond to the same longest
  matching context.  In theory this increases the expressivity of the
  model.  In practice the statistics become diluted.  A simple way to
  fix this is protect edges from being redirected twice.  This gave an
  immediate improvement in compression ratio.

* DMC's model structure is affected by the temporal order in which
  contexts occur.  Imagine a PPM algorithm that "forgot" to update
  higher-order states' successor pointers when adding a new state.  So
  if you have current symbol "c" and current context "ra" with
  successor succ("ra", "c") == "ac".  You create a new state "rac" and
  update succ("ra", "c") := "rac", but you forget to update
  succ("bra", "c") to "rac", and it'll still point to "ac".  In the
  end it's as if PPM got so drunk it can't figure out what the longest
  matching context is anymore.

  I fixed this by introducing suffix pointers into each state and
  using right-extension pointers instead of successor pointers since I
  couldn't figure out how to update the successor pointers correctly.
  I think Shkarin's PPMd maybe does this but I was too lazy to try
  deciphering how that bit works.

REFERENCES
==========

[1] Bell, Timothy C.; Cleary, John G.; Witten, Ian H.
    "Text compression"
    Prentice Hall (1990).

[2] Cleary, John G.; Witten, Ian H.
    "Data Compression Using Adaptive Coding and Partial String Matching"
    IEEE Trans. Comm. 32 (1984) no. 4, 396-402.

[3] Cormack, G. V.; Horspool, R. N. S.
    "Data compression using dynamic Markov modelling"
    Comput. J. 30 (1987) no. 6, 541-550.

[4] Bunton, Suzanne
    "On-Line Stochastic Processes in Data Compression"
    University of Washington (1996).

