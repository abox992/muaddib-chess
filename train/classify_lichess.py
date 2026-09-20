#!/usr/bin/env python3
"""Sample Lichess games and classify start positions with Jev.

Jev labels the *kind* of position (phase, structure, character) so self-play
can start from a diverse bag of FENs. It does not label who is winning.
"""

from __future__ import annotations

import argparse
import io
import json
import os
import random
import sys
import time
from collections import defaultdict
from pathlib import Path
from typing import Any

import chess
import chess.pgn
import requests

JEV_URL = "https://api.typesafe.ai/v1/systemone"
ROOT = Path(__file__).resolve().parents[1]
TRAIN_DIR = Path(__file__).resolve().parent
LICHESS_UA = "muaddib-train (https://github.com/abox992/chess-engine)"


def load_env(path: Path) -> None:
    if not path.is_file():
        return
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, value = line.partition("=")
        key = key.strip()
        value = value.strip().strip("'").strip('"')
        if key and key not in os.environ:
            os.environ[key] = value

QUESTIONS = {
    "game_phase": {
        "type": "choice",
        "instructions": "Classify the game phase from development and piece density. Do not judge who is winning.",
        "criteria": {
            "opening": "Development incomplete; many pieces still on the back rank.",
            "middlegame": "Most pieces developed, kings usually safe, full fight.",
            "endgame": "Few pieces left or a clearly simplified ending.",
        },
    },
    "structure": {
        "type": "choice",
        "instructions": "Classify the pawn-structure family. Do not judge which side is better.",
        "criteria": {
            "open": "Open center, files opened by pawn trades.",
            "semi_open": "Asymmetric or one open file.",
            "closed": "Locked pawn chains, blocked center.",
            "isolated_or_hanging": "IQP, hanging pawns, or a structural-weakness theme.",
            "opposite_castling": "Kings have castled, or are likely to castle, on opposite sides.",
            "other": "Does not fit the above.",
        },
    },
    "character": {
        "type": "choice",
        "instructions": "What kind of fight is this, ignoring who is winning.",
        "criteria": {
            "quiet": "Slow maneuvering, few forcing contacts.",
            "tactical": "Pieces in contact; tactics will dominate.",
            "king_attack": "A king is the object of a direct attack.",
            "technical": "Conversion, fortress, or simplified technique.",
        },
    },
}


def piece_counts(board: chess.Board) -> dict[str, int]:
    names = {1: "p", 2: "n", 3: "b", 4: "r", 5: "q", 6: "k"}
    counts: dict[str, int] = {}
    for color, prefix in ((chess.WHITE, "w"), (chess.BLACK, "b")):
        for pt, name in names.items():
            counts[f"{prefix}{name}"] = len(board.pieces(pt, color))
    return counts


def sample_from_pgn(stream, max_positions: int, min_ply: int, max_ply: int) -> list[chess.Board]:
    seen: set[str] = set()
    boards: list[chess.Board] = []
    while len(boards) < max_positions:
        game = chess.pgn.read_game(stream)
        if game is None:
            break
        board = game.board()
        ply = 0
        candidates: list[chess.Board] = []
        for move in game.mainline_moves():
            board.push(move)
            ply += 1
            if min_ply <= ply <= max_ply:
                fen = board.fen()
                if fen not in seen:
                    candidates.append(board.copy(stack=False))
        if not candidates:
            continue
        pick = random.choice(candidates)
        seen.add(pick.fen())
        boards.append(pick)
    return boards


def fetch_lichess_pgn(user: str, max_games: int) -> str:
    url = f"https://lichess.org/api/games/user/{user}"
    params = {
        "max": max_games,
        "rated": "true",
        "perfType": "blitz,rapid,classical",
    }
    headers = {
        "Accept": "application/x-chess-pgn",
        "User-Agent": LICHESS_UA,
    }
    last_error: Exception | None = None
    for attempt in range(6):
        r = requests.get(url, params=params, headers=headers, timeout=120)
        if r.status_code == 429:
            wait = r.headers.get("Retry-After", "60")
            try:
                seconds = max(1, int(float(wait)))
            except ValueError:
                seconds = 60
            print(f"lichess rate-limited; waiting {seconds}s (attempt {attempt + 1}/6)", file=sys.stderr)
            time.sleep(seconds)
            last_error = requests.HTTPError(f"429 Too Many Requests for {r.url}", response=r)
            continue
        r.raise_for_status()
        if not r.text.strip():
            raise RuntimeError(f"lichess returned no games for {user}")
        return r.text
    assert last_error is not None
    raise last_error


def classify(fen: str, api_key: str) -> dict[str, Any]:
    board = chess.Board(fen)
    payload = {
        "model": "jev-latest",
        "state": {
            "fen": fen,
            "side_to_move": "white" if board.turn else "black",
            "piece_counts": piece_counts(board),
            "fullmove": board.fullmove_number,
        },
        "questions": QUESTIONS,
    }
    r = requests.post(
        JEV_URL,
        headers={"Authorization": f"Bearer {api_key}", "Content-Type": "application/json"},
        json=payload,
        timeout=30,
    )
    r.raise_for_status()
    body = r.json()
    answers = body.get("answers", body)
    labels: dict[str, Any] = {}
    for key in QUESTIONS:
        ans = answers.get(key, {})
        if isinstance(ans, dict):
            labels[key] = ans.get("choice", ans)
        else:
            labels[key] = ans
    return labels


def stratified(rows: list[dict[str, Any]], limit: int) -> list[dict[str, Any]]:
    buckets: dict[tuple, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        buckets[(row.get("game_phase"), row.get("structure"), row.get("character"))].append(row)
    out: list[dict[str, Any]] = []
    while len(out) < limit and any(buckets.values()):
        for key in list(buckets):
            if buckets[key]:
                out.append(buckets[key].pop())
            if len(out) >= limit:
                break
    return out


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pgn", type=Path, help="local PGN file")
    parser.add_argument("--lichess-user", default="DrNykterstein", help="public Lichess username to export")
    parser.add_argument("--max-games", type=int, default=80)
    parser.add_argument("--max-positions", type=int, default=200)
    parser.add_argument("--min-ply", type=int, default=8)
    parser.add_argument("--max-ply", type=int, default=24)
    parser.add_argument("--out-dir", type=Path, default=TRAIN_DIR / "data")
    parser.add_argument("--no-jev", action="store_true", help="sample FENs only, skip Jev")
    args = parser.parse_args()
    load_env(ROOT / ".env")
    load_env(Path.cwd() / ".env")

    if args.pgn:
        with args.pgn.open() as f:
            boards = sample_from_pgn(f, args.max_positions, args.min_ply, args.max_ply)
    else:
        print(f"fetching {args.max_games} games from lichess.org/api/games/user/{args.lichess_user}")
        pgn = fetch_lichess_pgn(args.lichess_user, args.max_games)
        boards = sample_from_pgn(io.StringIO(pgn), args.max_positions, args.min_ply, args.max_ply)

    print(f"sampled {len(boards)} unique positions")
    api_key = os.environ.get("TYPESAFE_API_KEY", "")
    rows: list[dict[str, Any]] = []
    for i, board in enumerate(boards):
        fen = board.fen()
        row: dict[str, Any] = {"fen": fen}
        if not args.no_jev:
            if not api_key:
                print("TYPESAFE_API_KEY unset; writing unclassified FENs. Re-run without --no-jev once you have a key.", file=sys.stderr)
                args.no_jev = True
            else:
                try:
                    row.update(classify(fen, api_key))
                except Exception as exc:
                    print(f"jev failed on position {i}: {exc}", file=sys.stderr)
        rows.append(row)
        if (i + 1) % 10 == 0:
            print(f"classified {i + 1}/{len(boards)}")

    picked = stratified(rows, args.max_positions) if not args.no_jev else rows
    args.out_dir.mkdir(parents=True, exist_ok=True)
    jsonl_path = args.out_dir / "starts.jsonl"
    fen_path = args.out_dir / "starts.fen"
    with jsonl_path.open("w") as jf, fen_path.open("w") as ff:
        for row in picked:
            jf.write(json.dumps(row) + "\n")
            ff.write(row["fen"] + "\n")
    print(f"wrote {len(picked)} starts to {fen_path} and {jsonl_path}")


if __name__ == "__main__":
    main()
