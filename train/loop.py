#!/usr/bin/env python3
"""Self-play → train challenger → match vs champion. Promote at score >= 0.60.

Each generation is stamped under train/runs/gNNN/ and never overwritten.
The promotion match is the fixed exam book, each opening played on both sides.
Probe matches are logged only; they never decide promotion.
"""

from __future__ import annotations

import argparse
import json
import random
import shutil
import subprocess
import sys
from pathlib import Path

TRAIN_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TRAIN_DIR))
from curriculum import build_starts
from fenio import load_fens, save_fens
from match import play_match

ROOT = TRAIN_DIR.parent
DATA = TRAIN_DIR / "data"
RUNS = TRAIN_DIR / "runs"
EXAM = DATA / "exam.fen"
PROBE = DATA / "probe.fen"
REPLAY = DATA / "replay.jsonl"
BEST_JSON = DATA / "best.json"
BEST_NET = DATA / "eval.muadnet"
HISTORY = RUNS / "history.jsonl"
PROMOTE_SCORE = 0.60


def gen_dir(n: int) -> Path:
    return RUNS / f"g{n:03d}"


def existing_gens() -> list[int]:
    nums: list[int] = []
    if not RUNS.is_dir():
        return nums
    for p in RUNS.iterdir():
        if p.is_dir() and p.name.startswith("g") and p.name[1:].isdigit():
            nums.append(int(p.name[1:]))
    return sorted(nums)


def next_gen() -> int:
    gens = existing_gens()
    return gens[-1] + 1 if gens else 0


def rel(path: Path | None) -> str | None:
    if path is None:
        return None
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


def load_best() -> dict:
    if BEST_JSON.is_file():
        return json.loads(BEST_JSON.read_text())
    return {"generation": 0, "eval": None}


def write_best(generation: int, eval_path: Path | None) -> None:
    payload = {"generation": generation, "eval": rel(eval_path)}
    write_json(BEST_JSON, payload)
    if eval_path is not None:
        shutil.copy2(eval_path, BEST_NET)


def append_history(row: dict) -> None:
    HISTORY.parent.mkdir(parents=True, exist_ok=True)
    with HISTORY.open("a") as f:
        f.write(json.dumps(row) + "\n")


def resolve_eval(best: dict) -> Path | None:
    raw = best.get("eval")
    if not raw:
        return None
    path = Path(raw)
    if not path.is_absolute():
        path = ROOT / path
    return path if path.is_file() else None


def previous_champion(best: dict) -> Path | None:
    """Net the current champion replaced, saved when that champion was trained."""
    if resolve_eval(best) is None:
        return None
    path = gen_dir(int(best.get("generation", 0))) / "champion.muadnet"
    return path if path.is_file() else None


def held_out() -> set[str]:
    return set(load_fens(EXAM)) | set(load_fens(PROBE))


def recent_games(up_to_gen: int, k: int = 3) -> list[Path]:
    paths: list[Path] = []
    for g in range(up_to_gen, -1, -1):
        p = gen_dir(g) / "games.jsonl"
        if p.is_file() and p.stat().st_size > 0:
            paths.append(p)
            if len(paths) >= k:
                break
    return list(reversed(paths))


def run_cmd(cmd: list[str], log_path: Path | None = None) -> None:
    print("+", " ".join(cmd), flush=True)
    if log_path is None:
        subprocess.run(cmd, cwd=ROOT, check=True)
        return
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w") as log:
        proc = subprocess.Popen(
            cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            sys.stdout.write(line)
            log.write(line)
        if proc.wait() != 0:
            raise SystemExit(f"command failed ({proc.returncode}): {' '.join(cmd)}")


def selfplay_cmd(
    args: argparse.Namespace,
    starts: Path,
    out: Path,
    seed: int,
    eval_path: Path | None,
) -> list[str]:
    cmd = [
        str(args.engine),
        "selfplay",
        "--games",
        str(args.games),
        "--depth",
        str(args.depth),
        "--workers",
        str(args.workers),
        "--starts",
        str(starts),
        "--out",
        str(out),
        "--seed",
        str(seed),
    ]
    if eval_path is not None:
        cmd += ["--eval", str(eval_path)]
    return cmd


def score_replay_cmd(current: Path, previous: Path, games: list[Path]) -> list[str]:
    cmd = [
        "uv",
        "run",
        "--project",
        str(TRAIN_DIR),
        "--group",
        "train",
        "python",
        str(TRAIN_DIR / "curriculum.py"),
        "--current",
        str(current),
        "--previous",
        str(previous),
        "--replay",
        str(REPLAY),
        "--games",
        *[str(p) for p in games],
        "--exclude",
        str(EXAM),
    ]
    if PROBE.is_file():
        cmd += [str(PROBE)]
    return cmd


def bootstrap(src: Path, seed: int) -> None:
    g0 = gen_dir(0)
    if g0.exists():
        print(f"loop: {g0} already exists, not overwriting")
        return
    if not src.is_file():
        raise SystemExit(f"bootstrap games not found: {src}")
    g0.mkdir(parents=True)
    shutil.copy2(src, g0 / "games.jsonl")
    write_json(
        g0 / "meta.json",
        {"generation": 0, "champion": "pst", "source": rel(src), "seed": seed},
    )
    if not BEST_JSON.exists():
        write_best(0, None)
    append_history(
        {
            "generation": 0,
            "promoted": True,
            "match_score": None,
            "probe_score": None,
            "n_games": sum(1 for _ in src.open() if _.strip()),
            "champion_gen": None,
            "challenger": None,
            "seed": seed,
        }
    )
    print(f"loop: bootstrapped {g0 / 'games.jsonl'} from {src}")


def create_g0(args: argparse.Namespace, seed: int) -> None:
    dest = gen_dir(0)
    if dest.exists():
        return
    dest.mkdir(parents=True)
    rng = random.Random(seed)
    starts = build_starts(rng, args.games, REPLAY, held_out(), use_replay=False)
    save_fens(dest / "starts.fen", starts)
    run_cmd(selfplay_cmd(args, dest / "starts.fen", dest / "games.jsonl", seed, None), dest / "selfplay.log")
    write_json(
        dest / "meta.json",
        {"generation": 0, "champion": "pst", "source": "explore", "seed": seed, "games": args.games},
    )
    if not BEST_JSON.exists():
        write_best(0, None)
    append_history(
        {
            "generation": 0,
            "promoted": True,
            "match_score": None,
            "probe_score": None,
            "n_games": args.games,
            "champion_gen": None,
            "challenger": None,
            "seed": seed,
        }
    )
    print(f"loop: g000 PST self-play wrote {dest / 'games.jsonl'}")


def run_generation(args: argparse.Namespace, gen: int, seed: int) -> None:
    dest = gen_dir(gen)
    if dest.exists():
        raise SystemExit(f"loop: {dest} already exists")
    dest.mkdir(parents=True)

    best = load_best()
    champion_path = resolve_eval(best)
    champion_gen = int(best.get("generation", 0))
    rng = random.Random(seed)
    exam = load_fens(EXAM)
    if not exam:
        raise SystemExit(f"loop: exam book is empty: {EXAM}")

    skip_selfplay = gen == 1 and champion_path is None and (gen_dir(0) / "games.jsonl").is_file()
    starts: list[str] = []
    if not skip_selfplay:
        previous = previous_champion(best)
        if champion_path is not None and previous is not None:
            games = recent_games(gen - 1)
            if games:
                run_cmd(score_replay_cmd(champion_path, previous, games), dest / "replay.log")
            use_replay = True
        else:
            use_replay = False
        starts = build_starts(rng, args.games, REPLAY, held_out(), use_replay)
        save_fens(dest / "starts.fen", starts)

    save_fens(dest / "match.fen", exam)

    n_selfplay = 0
    if not skip_selfplay:
        run_cmd(
            selfplay_cmd(args, dest / "starts.fen", dest / "games.jsonl", seed, champion_path),
            dest / "selfplay.log",
        )
        n_selfplay = args.games
    else:
        print("loop: skipping self-play for g001 (using bootstrap games)")

    data_files = recent_games(gen)
    if not data_files:
        raise SystemExit(f"loop: no games.jsonl to train generation {gen}")
    challenger = dest / "challenger.muadnet"
    train_cmd = [
        "uv",
        "run",
        "--project",
        str(TRAIN_DIR),
        "--group",
        "train",
        "python",
        str(TRAIN_DIR / "train.py"),
        "--out",
        str(challenger),
        "--epochs",
        str(args.epochs),
        "--data",
        *[str(p) for p in data_files],
    ]
    if champion_path is not None:
        train_cmd += ["--init", str(champion_path)]
    run_cmd(train_cmd, dest / "train.log")

    if champion_path is not None:
        shutil.copy2(champion_path, dest / "champion.muadnet")

    match_result = play_match(
        args.engine.resolve(),
        exam,
        challenger.resolve(),
        champion_path.resolve() if champion_path else None,
        args.depth,
        args.workers,
    )
    write_json(dest / "match.json", match_result)
    promoted = match_result["score"] >= PROMOTE_SCORE
    print(
        f"loop: match {match_result['wins']}-{match_result['draws']}-{match_result['losses']} "
        f"score {match_result['score']:.3f} promote={promoted}"
    )

    probe_openings = load_fens(PROBE)
    probe_result = None
    if probe_openings:
        probe_result = play_match(
            args.engine.resolve(),
            probe_openings,
            challenger.resolve(),
            champion_path.resolve() if champion_path else None,
            args.depth,
            args.workers,
        )
        write_json(dest / "probe.json", probe_result)
        print(
            f"loop: probe {probe_result['wins']}-{probe_result['draws']}-{probe_result['losses']} "
            f"score {probe_result['score']:.3f} (log only)"
        )
    else:
        print("loop: no probe.fen; skipping probe match")

    if promoted:
        write_best(gen, challenger)
        print(f"loop: promoted g{gen:03d} -> {BEST_NET}")
    else:
        print(f"loop: kept champion g{champion_gen:03d}")

    meta = {
        "generation": gen,
        "promoted": promoted,
        "champion_gen": champion_gen,
        "champion": rel(champion_path),
        "challenger": rel(challenger),
        "seed": seed,
        "depth": args.depth,
        "games": n_selfplay,
        "match_games": match_result["games_played"],
        "train_data": [rel(p) for p in data_files],
        "skip_selfplay": skip_selfplay,
        "match_score": match_result["score"],
        "probe_score": None if probe_result is None else probe_result["score"],
    }
    write_json(dest / "meta.json", meta)
    append_history(
        {
            "generation": gen,
            "promoted": promoted,
            "match_score": match_result["score"],
            "probe_score": None if probe_result is None else probe_result["score"],
            "n_games": n_selfplay,
            "champion_gen": champion_gen,
            "challenger": rel(challenger),
            "seed": seed,
        }
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "chess")
    parser.add_argument("--bootstrap", type=Path, default=None, help="copy existing games.jsonl to runs/g000")
    parser.add_argument("--gens", type=int, default=1, help="challenger generations to run")
    parser.add_argument("--games", type=int, default=20, help="self-play games per generation")
    parser.add_argument("--depth", type=int, default=4)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--seed", type=int, default=None)
    args = parser.parse_args()

    if not args.engine.is_file():
        raise SystemExit(f"engine not found: {args.engine} (build it first)")
    if not load_fens(EXAM):
        raise SystemExit(
            f"missing exam book: {EXAM}\n"
            "  uv run --project train python train/exam_book.py"
        )

    seed = args.seed if args.seed is not None else random.randrange(1, 2**31)
    print(f"loop: seed {seed}")
    RUNS.mkdir(parents=True, exist_ok=True)

    if args.bootstrap is not None:
        bootstrap(args.bootstrap, seed)
    if not gen_dir(0).exists() and next_gen() == 0:
        create_g0(args, seed)

    start = next_gen()
    if start == 0:
        raise SystemExit("loop: failed to create g000")
    for i in range(args.gens):
        gen = start + i
        print(f"loop: generation {gen}")
        run_generation(args, gen, seed + gen)


if __name__ == "__main__":
    main()
