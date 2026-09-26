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
sys.path.insert(0, str(TRAIN_DIR))
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


def sample_from_pgn(
    stream, max_positions: int, min_ply: int, max_ply: int, per_game: int = 1
) -> list[chess.Board]:
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
        random.shuffle(candidates)
        for pick in candidates[: max(1, per_game)]:
            fen = pick.fen()
            if fen in seen:
                continue
            seen.add(fen)
            boards.append(pick)
            if len(boards) >= max_positions:
                break
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
    parser.add_argument("--max-games", type=int, default=None)
    parser.add_argument("--max-positions", type=int, default=200)
    parser.add_argument("--min-ply", type=int, default=8)
    parser.add_argument("--max-ply", type=int, default=24)
    parser.add_argument("--per-game", type=int, default=1, help="unique FENs to keep from each game")
    parser.add_argument("--pool-size", type=int, default=0, help="write pool.fen with this many unique FENs (no Jev)")
    parser.add_argument("--probe", type=int, default=0, help="hold out this many FENs into probe.fen if it does not exist")
    parser.add_argument("--append-pool", action="store_true", help="add unique FENs to an existing pool.fen")
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--out-dir", type=Path, default=TRAIN_DIR / "data")
    parser.add_argument("--no-jev", action="store_true", help="sample FENs only, skip Jev")
    args = parser.parse_args()
    load_env(ROOT / ".env")
    load_env(Path.cwd() / ".env")
    if args.seed is not None:
        random.seed(args.seed)

    building_pool = args.pool_size > 0 or args.append_pool
    if building_pool:
        args.no_jev = True
        if args.per_game == 1:
            args.per_game = 8
        if args.max_ply == 24:
            args.max_ply = 40

    max_games = args.max_games
    if max_games is None:
        if building_pool:
            max_games = max(400, (args.pool_size or 5000) // max(args.per_game, 1) + 100)
        else:
            max_games = 80

    if args.pool_size > 0:
        want = args.pool_size
    elif args.append_pool:
        want = 5000
    else:
        want = args.max_positions
    if args.pgn:
        with args.pgn.open() as f:
            boards = sample_from_pgn(f, want, args.min_ply, args.max_ply, args.per_game)
    else:
        print(f"fetching {max_games} games from lichess.org/api/games/user/{args.lichess_user}")
        pgn = fetch_lichess_pgn(args.lichess_user, max_games)
        boards = sample_from_pgn(io.StringIO(pgn), want, args.min_ply, args.max_ply, args.per_game)

    print(f"sampled {len(boards)} unique positions")
    args.out_dir.mkdir(parents=True, exist_ok=True)

    if building_pool:
        from fenio import load_fens, save_fens

        pool_path = args.out_dir / "pool.fen"
        probe_path = args.out_dir / "probe.fen"
        starts_path = args.out_dir / "starts.fen"
        fens: list[str] = []
        seen: set[str] = set()
        if args.append_pool:
            for fen in load_fens(pool_path):
                if fen not in seen:
                    seen.add(fen)
                    fens.append(fen)
        for fen in load_fens(starts_path):
            if fen not in seen:
                seen.add(fen)
                fens.append(fen)
        for board in boards:
            fen = board.fen()
            if fen not in seen:
                seen.add(fen)
                fens.append(fen)
        probe = load_fens(probe_path)
        if args.probe > 0 and not probe:
            if len(fens) < args.probe:
                raise SystemExit(f"need {args.probe} FENs for probe, sampled {len(fens)}")
            probe = random.sample(fens, args.probe)
            save_fens(probe_path, probe)
            print(f"wrote {len(probe)} probe FENs to {probe_path}")
        probe_set = set(probe)
        fens = [fen for fen in fens if fen not in probe_set]
        if args.pool_size > 0 and not args.append_pool:
            fens = fens[: args.pool_size]
        save_fens(pool_path, fens)
        print(f"wrote {len(fens)} pool FENs to {pool_path}")
        return

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
    jsonl_path = args.out_dir / "starts.jsonl"
    fen_path = args.out_dir / "starts.fen"
    with jsonl_path.open("w") as jf, fen_path.open("w") as ff:
        for row in picked:
            jf.write(json.dumps(row) + "\n")
            ff.write(row["fen"] + "\n")
    print(f"wrote {len(picked)} starts to {fen_path} and {jsonl_path}")


if __name__ == "__main__":
    main()
