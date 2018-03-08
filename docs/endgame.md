Ormulogun endgame syntax
========================

An endgame specification is a list of pieces with an optional
comma-separated list of location filters following it in curly
braces. For example, `K` or `K{…}` or `K{…,…,…}`.

If no location filters are specified, the behavior is the same as
`{*|*}`, which means pick any file and any rank at random and put the
piece there. Uppercase pieces are our own, lowercase are the
opponent's (actual colors may be swapped randomly).

Ormulogun will do its best to generate a legal and quiet position.

Location filter
---------------

A location filter is a list of files and a list of ranks, separated by
a colon `|`. The simplest filter is `*|*`, which means any file and
any rank.

Files and ranks can be specified by their number: `01|*` means any
square on the a-file or b-file. `*|7` means any square on the
opponent's back rank. Ranks and files are numbered from 0 to 7.

Files and ranks can also be relative to another piece. `FK|*` means
any square on the same file as our king. `FK+1FK-1|RK` are the two
squares immediately to the right and left of our king. The special
token "@" is the rank or file of the current piece being placed. For
example, `*|@` (or the equivalent `@|*`) means any square with equal
rank and file, ie the a1-h8 diagonal; `*|7-@` similarly means the
a8-h1 diagonal.

Examples
--------

* `Kk`: the minimum valid endgame, two kings randomly placed.
* `K{34|34}k{07|07}`: own king in the center, opponent king on a corner of the board.
* `{07|*,*|07}`: any square on the edge of the board
* `Kk{FK-2|RK,FK+2|RK}`: one king randomly placed, then one king two squares left or right of it.
* `K{07|07}k{FK+7|RK+7,FK-7|RK-7,FK+7|RK-7,FK-7|RK+7}`: randomly place kings in opposite corners of the board.
* `B{*|@}`: place a dark square bishop on the big diagonal (a1-h8).
* `B{*|7-@}`: place a light square bishop on the big diagonal (a8-h1).
* `B{*|@@+2@+4@+6@-2@-4@-6}`: place a bishop on a dark square.
* `B{*|@+1@+3@+5@+7@-1@-3@-5@-7}`: place a bishop on a light square.
* `P{*|12345}p{FP|RP+1RP+2RP+3RP+4RP+5}`: make a closed file.

BNF grammar
-----------

~~~
<endgame-spec> ::= <piece-spec> <piece-spec> | <endgame-spec> <piece-spec>

<piece-spec> ::= <piece> <position-filter>
<piece>      ::= "K" | "Q" | "R" | "B" | "N" | "P" | "k" | "q" | "r" | "b" | "n" | "p"

<position-filter>         ::= "" | "{" <position-filter-list> "}"
<position-filter-list>    ::= <position-filter-element> | <position-filter-list> "," <position-filter-element>
<position-filter-element> ::= <file-or-rank-list> "|" <file-or-rank-list>

<file-or-rank-list> ::= "*" | <file-or-rank-list> <file-or-rank> <file-or-rank-modifier>
<file-or-rank>      ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
                      | <relative-file-or-rank>

<file-or-rank-modifier> ::= "+" <file-or-rank> | "-" <file-or-rank>
<relative-file-or-rank> ::= "@" | "F" <piece> | "R" <piece>
~~~
