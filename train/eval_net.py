"""Shared eval-net architecture. Must stay in lockstep with src/eval_net.cpp."""

from __future__ import annotations

import struct
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn

INPUT = 768
HIDDEN = 32
MAGIC = b"MUADNET1"

# python-chess a1=0 / h1=7; this engine a1=7 / h1=0.
def engine_square(chess_sq: int) -> int:
    return chess_sq ^ 7


def encode_fen(fen: str) -> np.ndarray:
    import chess

    board = chess.Board(fen)
    x = np.zeros(INPUT, dtype=np.float32)
    stm_white = board.turn == chess.WHITE
    flip = 0 if stm_white else 56
    for piece_type in range(1, 7):
        stm_plane = piece_type - 1
        opp_plane = stm_plane + 6
        stm_color = chess.WHITE if stm_white else chess.BLACK
        opp_color = chess.BLACK if stm_white else chess.WHITE
        for sq in board.pieces(piece_type, stm_color):
            x[stm_plane * 64 + (engine_square(sq) ^ flip)] = 1.0
        for sq in board.pieces(piece_type, opp_color):
            x[opp_plane * 64 + (engine_square(sq) ^ flip)] = 1.0
    return x


class EvalNet(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.fc1 = nn.Linear(INPUT, HIDDEN)
        self.fc2 = nn.Linear(HIDDEN, HIDDEN)
        self.fc3 = nn.Linear(HIDDEN, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        h = torch.relu(self.fc1(x))
        h = torch.relu(self.fc2(h))
        return self.fc3(h).squeeze(-1)


def save_muadnet(model: EvalNet, path: str | Path) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    sd = model.state_dict()
    with path.open("wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<IIII", INPUT, HIDDEN, HIDDEN, 1))
        for key in ("fc1.weight", "fc1.bias", "fc2.weight", "fc2.bias", "fc3.weight", "fc3.bias"):
            f.write(sd[key].detach().cpu().contiguous().float().numpy().tobytes())


def load_muadnet(path: str | Path) -> EvalNet:
    path = Path(path)
    model = EvalNet()
    with path.open("rb") as f:
        magic = f.read(8)
        if magic != MAGIC:
            raise ValueError(f"bad magic in {path}")
        dims = struct.unpack("<IIII", f.read(16))
        if dims != (INPUT, HIDDEN, HIDDEN, 1):
            raise ValueError(f"unexpected dims {dims}")
        sd = model.state_dict()
        for key in ("fc1.weight", "fc1.bias", "fc2.weight", "fc2.bias", "fc3.weight", "fc3.bias"):
            n = sd[key].numel()
            raw = f.read(n * 4)
            sd[key] = torch.from_numpy(np.frombuffer(raw, dtype=np.float32).copy().reshape(sd[key].shape))
        model.load_state_dict(sd)
    return model
