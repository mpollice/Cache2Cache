# Cache2Cache
Cache2Cache is a tool that measures the communication latency between logical CPU cores in a Microsoft Windows system.

Cache2Cache measures the latency between any pair of logical CPUs in the system. It does so by alternatively incrementing a set of numbers by two threads that are assigned to the respective logical cores. Either thread continues after the other has incremented its number. The result is the average latency of 100000000 iterations.

## Usage
Simply run the tool from the command prompt in the current directory like this:

```
./Cache2Cache
```

In some cases it may be desirable to run the loop with an additional instruction inserted. In particular it has been found by the original author, that the PAUSE and NOP instructions are [interesting choices](http://web.archive.org/web/20051106060139/http://www.aceshardware.com:80/forums/read_post.jsp?id=20687&forumid=2). NOP only acts as a control for any change of behavior introduced by PAUSE, since PAUSE acts as a NOP on processors that don't support it. Given PAUSE is part of the now ubiquitious SSE2 instruction set, this caveat has largely only historical relevance. PAUSE has been found to reduce the latency between logical processors of the same physical core by a sizable amount. Latency between logical processors of different physical cores is still reduced somewhat.

As per [Intel's documentation](https://software.intel.com/en-us/articles/intel-sdm) PAUSE is supposed to "improve the performance of spin-wait loops", which is a core element of this program. It allows the CPU to avoid memory order violations that are otherwise detected when exiting such a loop.

In order to insert a NOP or PAUSE, all that is required to do is to pass a parameter to the program, either "nop" or "pause" (both lower-case):

```
./Cache2Cache nop
```
```
./Cache2Cache pause
```
For convenience the [release distribution](https://github.com/mpollice/Cache2Cache/releases) contains batch files to run either the 32-bit or 64-bit version of the tool in the different variants and append the results to a text file "Cache2Cache_result.txt" in the same directory. Each set of results is prepended with some information what variant is run and the current date and time.

## Acknowledgements
Originally this program was posted in the [Ace's Hardware Forums](http://web.archive.org/web/20060326233205/http://www.aceshardware.com/forums/read_post.jsp?id=20681&forumid=2) (archived version of the post at archive.org) by Michael_S. I obtained permission from the author via email to provide this software under the MIT license.