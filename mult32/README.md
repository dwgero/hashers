# Mult32
Mult32 is a 32-bit hash function written in C that is highly portable,
uses no special CPU instructions, and passes all the tests in SMHasher3.<br>
It only uses a 32-bit by 32-bit multiplication producing a 64-bit result,
and is suitable for all hash tables with up to 4 billion buckets.<br>
It is currently the fastest hasher meeting the above specifications for input
strings of length greater than or equal to 32 bytes, as measured
against other 32-bit hashers meeting those specifications that are
documented in SMHasher3.<br>
For input strings of length less than 32 bytes, Komi32 is faster.<br>
Average bulk speed tests of Mult32 in SMHasher3 are 7.7 to 8.2 bytes/cycle
running on a 2.6 Ghz processor in a system from 2016.
