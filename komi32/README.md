# Komi32
Komi32 is a 32-bit hash function written in C that is highly portable,
uses no special CPU instructions, and passes all the tests in SMHasher3.<br>
It is a fork of Aleksey Vaneev's 64-bit komihash, which uses a special
multiply of two 64-bit numbers producing a 128-bit result.<br>
Komi32 only uses a 32-bit by 32-bit multiplication producing a 64-bit result,
and is suitable for all hash tables with up to 4 billion buckets.<br>
It is currently the fastest hasher meeting the above specifications for input
strings of length less than 32 bytes, as measured against other 32-bit
hashers meeting those specifications that are documented in SMHasher3.<br>
For input strings of length greater than or equal to 32 bytes, Mult32 is faster.
