Ormulogun
=========

A very simple serverless, privacy aware chess tactics
trainer. Released under the Apache License 2.0.

Dependencies
============

For the frontend, any modern browser should work.

For generating puzzle lists:

* PHP (CLI)
* [Gumble](https://gitlab.com/Artefact2/gumble)
* Another (good) UCI engine (eg stockfish)

Data structures
===============

Puzzle set manifest (`manifest.json`)
-------------------------------------

~~~
[
	{
		"id": "foo",
		"src": "foo.json",
		"count": 12,
		"name": "Foo puzzle set",
		"desc": "A set of foo puzzles. Are you up to the task?",
		"prompt": "Find the best move for {%side}.",
	},
	...
]
~~~

Puzzle set
----------

~~~
[
    {
        "ply": 30,
        "board": "4r1k1\/ppqnrppp\/2pb1n2\/3p1B2\/3P4\/2N2QP1\/PPPB1P1P\/R3R1K1 b - - 6 15",
        "reply": {
            "lan": "e7e1",
            "san": "Rxe1",
            "fen": "4r1k1\/ppqn1ppp\/2pb1n2\/3p1B2\/3P4\/2N2QP1\/PPPB1P1P\/R3r1K1 w - - 0 16"
        },
        "next": {
            "a1e1": {
                "move": {
                    "san": "Rxe1",
                    "fen": "4r1k1\/ppqn1ppp\/2pb1n2\/3p1B2\/3P4\/2N2QP1\/PPPB1P1P\/4R1K1 b - - 0 16"
                },
                "reply": {
                    "lan": "e8e1",
                    "san": "Rxe1+",
                    "fen": "6k1\/ppqn1ppp\/2pb1n2\/3p1B2\/3P4\/2N2QP1\/PPPB1P1P\/4r1K1 w - - 0 17"
                },
                "next": {
                    "d2e1": {
                        "move": {
                            "san": "Bxe1",
                            "fen": "6k1\/ppqn1ppp\/2pb1n2\/3p1B2\/3P4\/2N2QP1\/PPP2P1P\/4B1K1 b - - 0 17"
                        },
                        "reply": {
                            "lan": "c7b6",
                            "san": "Qb6",
                            "fen": "6k1\/pp1n1ppp\/1qpb1n2\/3p1B2\/3P4\/2N2QP1\/PPP2P1P\/4B1K1 w - - 1 18"
                        },
                        "next": null
                    }
                }
            }
        }
    },
	...
]
~~~

Credits
=======

* [Lichess](https://lichess.org/), for making their game dumps available
* [Colin M.L. Burnett](https://en.wikipedia.org/wiki/User:Cburnett) for releasing his chess pieces as SVG (full license in `COPYING.pieces`)
* Pablo Impallari for releasing the [Kaushan Script](https://fontlibrary.org/en/font/kaushan-script) under the SIL Open Font License
