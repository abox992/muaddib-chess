"""Load/save FEN lists. One FEN per line."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable, Sequence


def load_fens(path: Path) -> list[str]:
    if not path.is_file():
        return []
    return [line.strip() for line in path.read_text().splitlines() if line.strip()]


def save_fens(path: Path, fens: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(fen + "\n" for fen in fens))


def append_unique(path: Path, fens: Iterable[str]) -> list[str]:
    existing = load_fens(path)
    seen = set(existing)
    added: list[str] = []
    for fen in fens:
        if fen not in seen:
            seen.add(fen)
            existing.append(fen)
            added.append(fen)
    if added:
        save_fens(path, existing)
    return added
