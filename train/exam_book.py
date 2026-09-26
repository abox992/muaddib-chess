#!/usr/bin/env python3
"""Studied openings for the promotion exam.

Each line is a main-line structure theory treats as playable for both sides.
`train/data/exam.fen` is these moves played out, not a hand-written FEN list.
Regenerate that file with this script after editing LINES.
"""

from __future__ import annotations

from pathlib import Path

import chess

# One position per structure, stopped once both sides have developed a plan.
LINES = [
    "e4 e5 Nf3 Nc6 Bc4 Bc5",
    "e4 e5 Nf3 Nc6 Bb5 Nf6",
    "e4 e5 Nf3 Nc6 Bb5 a6 Ba4 Nf6 O-O Be7",
    "e4 e5 Nf3 Nc6 d4 exd4 Nxd4 Nf6",
    "e4 e5 Nf3 Nc6 Nc3 Nf6 Bb5 Bb4",
    "e4 e5 Nf3 Nf6 Nxe5 d6 Nf3 Nxe4 d4 d5",
    "e4 e5 Nf3 Nc6 Bc4 Bc5 c3 Nf6",
    "e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 a6",
    "e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 g6",
    "e4 c5 Nf3 Nc6 d4 cxd4 Nxd4 Nf6 Nc3 e5",
    "e4 c5 Nf3 Nc6 d4 cxd4 Nxd4 g6",
    "e4 c5 Nf3 e6 d4 cxd4 Nxd4 Nc6 Nc3 a6",
    "e4 c5 Nf3 e6 d4 cxd4 Nxd4 a6",
    "e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 e6",
    "e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 Nc6",
    "e4 e6 d4 d5 Nc3 Bb4",
    "e4 e6 d4 d5 Nd2 c5",
    "e4 e6 d4 d5 e5 c5 c3 Nc6",
    "e4 c6 d4 d5 Nc3 dxe4 Nxe4 Bf5",
    "e4 c6 d4 d5 e5 Bf5",
    "e4 c6 d4 d5 exd5 cxd5",
    "e4 d5 exd5 Qxd5 Nc3 Qa5",
    "e4 Nf6 e5 Nd5 d4 d6",
    "e4 d6 d4 Nf6 Nc3 g6",
    "e4 g6 d4 Bg7 Nc3 d6",
    "d4 d5 c4 e6 Nc3 Nf6 Bg5 Be7",
    "d4 d5 c4 dxc4 Nf3 Nf6",
    "d4 d5 c4 c6 Nf3 Nf6 Nc3 dxc4",
    "d4 d5 c4 c6 Nf3 Nf6 Nc3 e6",
    "d4 Nf6 c4 e6 Nc3 Bb4",
    "d4 Nf6 c4 e6 Nf3 b6",
    "d4 Nf6 c4 g6 Nc3 Bg7 e4 d6",
    "d4 Nf6 c4 g6 Nc3 d5",
    "d4 Nf6 c4 c5 d5 e6",
    "d4 f5 c4 Nf6 g3 e6 Bg2 d5",
    "c4 e5 Nc3 Nf6 Nf3 Nc6",
    "Nf3 d5 c4 e6 g3 Nf6 Bg2 Be7",
    "d4 Nf6 c4 e6 g3 d5 Bg2 Be7",
    "d4 Nf6 c4 e6 Nf3 Bb4+",
    "d4 Nf6 c4 e6 Nf3 d5 Nc3 Bb4",
]


def fens() -> list[str]:
    out: list[str] = []
    for san in LINES:
        board = chess.Board()
        for tok in san.split():
            board.push_san(tok)
        out.append(board.fen())
    if len(out) != len(set(out)):
        raise SystemExit("exam book has duplicate positions")
    return out


def main() -> None:
    path = Path(__file__).resolve().parent / "data" / "exam.fen"
    path.parent.mkdir(parents=True, exist_ok=True)
    text = "".join(fen + "\n" for fen in fens())
    path.write_text(text)
    print(f"wrote {len(LINES)} positions to {path}")


if __name__ == "__main__":
    main()
