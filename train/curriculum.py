#!/usr/bin/env python3
"""Self-play start positions.

Explore: random legal lines from the initial position, ply 8–16.
Replay: positions whose training gradient still points along the last
champion-to-champion weight step. Half the games, once two nets exist.
Mutate: one to three legal moves from a replay position.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
from pathlib import Path

import chess

TRAIN_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TRAIN_DIR))

EXPLORE_MIN_PLY = 8
EXPLORE_MAX_PLY = 16
REPLAY_CAP = 2000


def load_replay(path: Path) -> list[dict]:
    if not path.is_file():
        return []
    rows: list[dict] = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        row = json.loads(line)
        if isinstance(row.get("fen"), str) and "stm_score" in row and "white_result" in row:
            rows.append(row)
    return rows


def save_replay(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    text = "".join(json.dumps(row) + "\n" for row in rows)
    path.write_text(text)


def _random_line(rng: random.Random, board: chess.Board, plies: int) -> bool:
    for _ in range(plies):
        moves = list(board.legal_moves)
        if not moves:
            return False
        board.push(rng.choice(moves))
    return True


def explore_starts(rng: random.Random, n: int, exclude: set[str]) -> list[str]:
    if n <= 0:
        return []
    found: list[str] = []
    seen = set(exclude)
    attempts = 0
    limit = max(1000, n * 50)
    while len(found) < n and attempts < limit:
        attempts += 1
        board = chess.Board()
        ply = rng.randint(EXPLORE_MIN_PLY, EXPLORE_MAX_PLY)
        if not _random_line(rng, board, ply):
            continue
        fen = board.fen()
        if fen in seen:
            continue
        seen.add(fen)
        found.append(fen)
    if len(found) < n:
        raise SystemExit(f"curriculum: needed {n} exploration starts, got {len(found)}")
    return found


def mutate_starts(rng: random.Random, sources: list[str], n: int, exclude: set[str]) -> list[str]:
    if n <= 0 or not sources:
        return []
    found: list[str] = []
    seen = set(exclude)
    attempts = 0
    limit = max(1000, n * 40)
    while len(found) < n and attempts < limit:
        attempts += 1
        try:
            board = chess.Board(rng.choice(sources))
        except ValueError:
            continue
        if not _random_line(rng, board, rng.randint(1, 3)):
            continue
        fen = board.fen()
        if fen in seen:
            continue
        seen.add(fen)
        found.append(fen)
    return found


def build_starts(
    rng: random.Random,
    n: int,
    replay_path: Path,
    exclude: set[str],
    use_replay: bool,
) -> list[str]:
    if n <= 0:
        raise SystemExit("curriculum: need at least one start")
    chosen: list[str] = []
    seen = set(exclude)

    def take(cands: list[str]) -> None:
        for fen in cands:
            if fen in seen or len(chosen) >= n:
                continue
            seen.add(fen)
            chosen.append(fen)

    if use_replay:
        bank = [row["fen"] for row in load_replay(replay_path) if row["fen"] not in seen]
        n_replay = min(len(bank), n // 2)
        if n_replay:
            picked = rng.sample(bank, n_replay)
            take(picked)
            n_mutate = (n - len(chosen)) // 2
            take(mutate_starts(rng, picked, n_mutate, seen))
    take(explore_starts(rng, n - len(chosen), seen))
    if len(chosen) != n:
        raise SystemExit(f"curriculum: needed {n} starts, got {len(chosen)}")
    return chosen


def _collect_rows(game_paths: list[Path], replay_path: Path, exclude: set[str]) -> dict[str, tuple[float, float]]:
    rows: dict[str, tuple[float, float]] = {}
    for row in load_replay(replay_path):
        fen = row["fen"]
        if fen not in exclude:
            rows[fen] = (float(row["stm_score"]), float(row["white_result"]))
    for path in game_paths:
        with path.open() as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                raw = json.loads(line)
                fen = raw.get("fen")
                if not isinstance(fen, str) or fen in exclude:
                    continue
                if "stm_score" not in raw or "white_result" not in raw:
                    continue
                rows[fen] = (float(raw["stm_score"]), float(raw["white_result"]))
    return rows


def _alignment(current_path: Path, previous_path: Path, rows: dict[str, tuple[float, float]]) -> list[dict]:
    import numpy as np
    import torch
    from torch.func import functional_call, jvp

    from eval_net import encode_fen, load_muadnet
    from train import training_target

    current = load_muadnet(current_path)
    previous = load_muadnet(previous_path)
    current.eval()
    params = {name: param.detach() for name, param in current.named_parameters()}
    prev_sd = previous.state_dict()
    tangents = {name: prev_sd[name].detach() - params[name] for name in params}

    fens = list(rows)
    x = torch.from_numpy(np.stack([encode_fen(fen) for fen in fens]))
    y = torch.tensor(
        [training_target(fen, rows[fen][0], rows[fen][1]) for fen in fens],
        dtype=torch.float32,
    )

    def losses(p: dict[str, torch.Tensor]) -> torch.Tensor:
        pred = functional_call(current, p, (x,))
        return (pred - y) ** 2

    _, dots = jvp(losses, (params,), (tangents,))
    rewards = dots.detach().abs().cpu().tolist()
    scored = []
    for fen, reward in zip(fens, rewards):
        stm_score, white_result = rows[fen]
        scored.append(
            {
                "fen": fen,
                "stm_score": stm_score,
                "white_result": white_result,
                "reward": float(reward),
            }
        )
    scored.sort(key=lambda row: row["reward"], reverse=True)
    return scored[:REPLAY_CAP]


def score_replay(
    current: Path,
    previous: Path,
    game_paths: list[Path],
    replay_path: Path,
    exclude: set[str],
) -> int:
    rows = _collect_rows(game_paths, replay_path, exclude)
    if not rows:
        print("curriculum: no positions to score")
        return 0
    ranked = _alignment(current, previous, rows)
    save_replay(replay_path, ranked)
    top = ranked[0]["reward"] if ranked else 0.0
    print(f"curriculum: replay bank {len(ranked)} top reward {top:.4g}")
    return len(ranked)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--previous", type=Path, required=True)
    parser.add_argument("--games", type=Path, nargs="+", required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--exclude", type=Path, nargs="*", default=[])
    args = parser.parse_args()
    from fenio import load_fens

    exclude: set[str] = set()
    for path in args.exclude:
        exclude.update(load_fens(path))
    score_replay(args.current, args.previous, args.games, args.replay, exclude)


if __name__ == "__main__":
    main()
