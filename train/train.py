#!/usr/bin/env python3
"""Train the eval net from self-play jsonl.

Labels are this engine's search scores and self-play game results — not Jev, not Stockfish.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
import torch
from torch.utils.data import DataLoader, Dataset

sys.path.insert(0, str(Path(__file__).resolve().parent))
from eval_net import EvalNet, encode_fen, save_muadnet

TRAIN_DIR = Path(__file__).resolve().parent
DEFAULT_DATA = TRAIN_DIR / "data" / "games.jsonl"
DEFAULT_OUT = TRAIN_DIR / "data" / "eval.muadnet"


class PositionDataset(Dataset):
    def __init__(self, path: Path) -> None:
        if not path.is_file():
            raise SystemExit(
                f"no self-play data at {path}\n"
                "generate it first (from the repo root, PST eval is the baseline):\n"
                "  ./build/chess selfplay --games 20 --depth 4 "
                "--starts train/data/starts.fen --out train/data/games.jsonl"
            )
        self.rows: list[tuple[np.ndarray, float]] = []
        with path.open() as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                row = json.loads(line)
                fen = row["fen"]
                stm_white = fen.split()[1] == "w"
                white_result = float(row["white_result"])
                stm_result = white_result if stm_white else -white_result
                stm_score = float(row["stm_score"]) / 100.0
                stm_score = max(-15.0, min(15.0, stm_score))
                # Search value is lower variance; outcome keeps the ceiling on this engine.
                target = 0.7 * stm_score + 0.3 * (stm_result * 5.0)
                self.rows.append((encode_fen(fen), target))

    def __len__(self) -> int:
        return len(self.rows)

    def __getitem__(self, i: int) -> tuple[torch.Tensor, torch.Tensor]:
        x, y = self.rows[i]
        return torch.from_numpy(x), torch.tensor(y, dtype=torch.float32)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=Path, default=DEFAULT_DATA, help="self-play jsonl from ./chess selfplay")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch", type=int, default=256)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--init-random", action="store_true", help="write a random net and exit")
    args = parser.parse_args()

    model = EvalNet()
    if args.init_random:
        save_muadnet(model, args.out)
        print(f"wrote random weights to {args.out}")
        return

    ds = PositionDataset(args.data)
    if len(ds) == 0:
        raise SystemExit(f"no positions in {args.data}")

    loader = DataLoader(ds, batch_size=args.batch, shuffle=True, drop_last=False)
    opt = torch.optim.Adam(model.parameters(), lr=args.lr)
    model.train()
    for epoch in range(args.epochs):
        total = 0.0
        n = 0
        for x, y in loader:
            pred = model(x)
            loss = torch.mean((pred - y) ** 2)
            opt.zero_grad()
            loss.backward()
            opt.step()
            total += float(loss.item()) * len(y)
            n += len(y)
        print(f"epoch {epoch + 1}/{args.epochs} mse {total / max(n, 1):.4f} n {n}")

    save_muadnet(model, args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
