# Combo32
Combo32 is a 32-bit hash function written in C that is highly portable,
uses no special CPU instructions, and passes all the tests in SMHasher3.<br>
It uses Komi32 for byte strings of length < 32, and uses Mult32 for
byte strings of length >= 32.
