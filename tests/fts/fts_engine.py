"""Drive the C++ server engine (src/fts_engine.cpp) with ServerModel's interface."""

from __future__ import annotations

import shutil
import struct
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SOURCES = [REPO / "src/fts_engine.cpp", REPO / "tests/fts/fts_engine_host.cpp"]
END = 0xFFFFFFFF


def build(out_dir: Path) -> Path:
    """Compile the engine and its host driver with ASan/UBSan; return the binary."""
    compiler = shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        raise FileNotFoundError("no C++ compiler")
    binary = out_dir / "fts_engine_host"
    subprocess.run(
        [compiler, "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
         "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
         f"-I{REPO / 'src'}", *map(str, SOURCES), "-o", str(binary)],
        check=True,
    )
    return binary


class EngineProcess:
    """Same surface as fts_model.ServerModel: feed(), reset(), notifications, notify_size."""

    def __init__(self, binary: Path, root: Path, *, notify_size: int = 20,
                 free_space: int = 4096, max_read: int = 65536) -> None:
        self.notify_size = notify_size
        self.notifications: list[bytes] = []
        self._proc = subprocess.Popen(
            [str(binary), str(root), str(free_space), str(max_read)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        )

    def _exchange(self, frame: bytes) -> None:
        assert self._proc.stdin and self._proc.stdout
        self._proc.stdin.write(frame)
        self._proc.stdin.flush()
        while True:
            header = self._proc.stdout.read(4)
            if len(header) != 4:
                raise RuntimeError(f"engine exited with {self._proc.wait()}")
            (size,) = struct.unpack("<I", header)
            if size == END:
                return
            message = self._proc.stdout.read(size)
            for start in range(0, len(message), self.notify_size):
                self.notifications.append(message[start : start + self.notify_size])

    def feed(self, data: bytes) -> None:
        self._exchange(b"F" + struct.pack("<I", len(data)) + data)

    def reset(self) -> None:
        self._exchange(b"R")

    def close(self) -> int:
        if self._proc.stdin:
            self._proc.stdin.close()
        code = self._proc.wait(timeout=10)
        if self._proc.stdout:
            self._proc.stdout.close()
        return code
