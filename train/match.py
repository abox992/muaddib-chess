#!/usr/bin/env python3
"""UCI match: challenger vs champion from a list of openings.

Each opening is played twice, with the nets swapping colors, so a side-to-move
advantage in the FEN scores 0.5 unless one net plays it better. Champion eval
path omitted means PST. Match games are not training data.
"""

from __future__ import annotations

import argparse
import json
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import chess
import chess.engine

TRAIN_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TRAIN_DIR))
from fenio import load_fens

ROOT = TRAIN_DIR.parent


def start_engine(engine_bin: Path, eval_file: Path | None) -> chess.engine.SimpleEngine:
    engine = chess.engine.SimpleEngine.popen_uci([str(engine_bin), "uci"])
    if eval_file is not None:
        engine.configure({"EvalFile": str(eval_file.resolve())})
    return engine


def play_game(
    engine_bin: Path,
    white_eval: Path | None,
    black_eval: Path | None,
    fen: str,
    depth: int,
    max_ply: int,
    resign_score: int,
    resign_plies: int,
) -> dict:
    white = None
    black = None
    try:
        white = start_engine(engine_bin, white_eval)
        black = start_engine(engine_bin, black_eval)
        board = chess.Board(fen)
        resign_count = 0
        termination = "max_ply"
        winner: chess.Color | None = None
        for _ply in range(max_ply):
            if board.is_checkmate():
                termination = "mate"
                winner = not board.turn
                break
            if board.is_stalemate() or board.is_insufficient_material():
                termination = "stalemate" if board.is_stalemate() else "insufficient"
                break
            if board.can_claim_fifty_moves() or board.can_claim_threefold_repetition():
                termination = "draw"
                break
            engine = white if board.turn == chess.WHITE else black
            result = engine.play(board, chess.engine.Limit(depth=depth), info=chess.engine.INFO_SCORE)
            if result.move is None:
                termination = "no_move"
                break
            cp = None
            score = result.info.get("score")
            if score is not None:
                cp = score.relative.score(mate_score=10000)
            if cp is not None and abs(cp) >= resign_score:
                resign_count += 1
                if resign_count >= resign_plies:
                    termination = "resign"
                    winner = board.turn if cp > 0 else not board.turn
                    break
            else:
                resign_count = 0
            board.push(result.move)
        else:
            termination = "max_ply"
        if board.is_checkmate() and winner is None:
            termination = "mate"
            winner = not board.turn
        if winner is chess.WHITE:
            result_str = "1-0"
        elif winner is chess.BLACK:
            result_str = "0-1"
        else:
            result_str = "1/2-1/2"
        return {
            "fen": fen,
            "result": result_str,
            "termination": termination,
            "plies": board.ply(),
        }
    finally:
        if white is not None:
            white.quit()
        if black is not None:
            black.quit()


def play_match(
    engine_bin: Path,
    openings: list[str],
    challenger: Path | None,
    champion: Path | None,
    depth: int,
    workers: int = 1,
    max_ply: int = 120,
    resign_score: int = 600,
    resign_plies: int = 3,
) -> dict:
    if not openings:
        raise SystemExit("match: no openings")

    # Challenger White, then champion White, for every opening.
    pairs: list[tuple[str, bool]] = []
    for fen in openings:
        pairs.append((fen, True))
        pairs.append((fen, False))

    def one(i: int) -> dict:
        fen, challenger_white = pairs[i]
        white_eval = challenger if challenger_white else champion
        black_eval = champion if challenger_white else challenger
        game = play_game(
            engine_bin, white_eval, black_eval, fen, depth, max_ply, resign_score, resign_plies
        )
        game["challenger_white"] = challenger_white
        if game["result"] == "1/2-1/2":
            game["challenger_points"] = 0.5
        elif (game["result"] == "1-0") == challenger_white:
            game["challenger_points"] = 1.0
        else:
            game["challenger_points"] = 0.0
        return game

    games: list[dict] = [None] * len(pairs)  # type: ignore[list-item]
    n_workers = max(1, min(workers, len(pairs)))
    if n_workers == 1:
        for i in range(len(pairs)):
            games[i] = one(i)
            print(f"match: game {i + 1}/{len(pairs)} {games[i]['result']} {games[i]['termination']}")
    else:
        with ThreadPoolExecutor(max_workers=n_workers) as pool:
            futures = {pool.submit(one, i): i for i in range(len(pairs))}
            done = 0
            for fut in as_completed(futures):
                i = futures[fut]
                games[i] = fut.result()
                done += 1
                g = games[i]
                print(f"match: game {done}/{len(pairs)} {g['result']} {g['termination']}")

    wins = sum(1 for g in games if g["challenger_points"] == 1.0)
    draws = sum(1 for g in games if g["challenger_points"] == 0.5)
    losses = sum(1 for g in games if g["challenger_points"] == 0.0)
    n = len(games)
    score = (wins + 0.5 * draws) / n
    return {
        "wins": wins,
        "draws": draws,
        "losses": losses,
        "games_played": n,
        "score": score,
        "depth": depth,
        "challenger": str(challenger) if challenger else None,
        "champion": str(champion) if champion else None,
        "games": games,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "chess")
    parser.add_argument("--challenger", type=Path, default=None)
    parser.add_argument("--champion", type=Path, default=None, help="omit for PST")
    parser.add_argument("--openings", type=Path, required=True)
    parser.add_argument("--depth", type=int, default=4)
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument("--out", type=Path, default=None)
    args = parser.parse_args()
    result = play_match(
        args.engine.resolve(),
        load_fens(args.openings),
        args.challenger.resolve() if args.challenger else None,
        args.champion.resolve() if args.champion else None,
        args.depth,
        args.workers,
    )
    print(
        f"match: {result['wins']}-{result['draws']}-{result['losses']} "
        f"score {result['score']:.3f} (challenger)"
    )
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2) + "\n")
        print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
