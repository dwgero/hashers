# Mult32
Mult32 is a 32-bit hash function written in C that is highly portable,
uses no special CPU instructions, and only requires a 32-bit by 32-bit
multiplication producing a 64-bit result.<br>
It is currently the fastest hasher meeting those requirements for input
strings of length greater than or equal to 64 bytes, as measured
against other 32-bit hashers meeting those requirements that are
documented in SMHasher3.<br>
For input strings of length less than 64 bytes, Komi32 is faster.<br>
Average bulk speed tests of Mult32 in SMHasher3 are 7.7 to 8.2 bytes/cycle.
