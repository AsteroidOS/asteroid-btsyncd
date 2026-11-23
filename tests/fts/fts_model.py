"""Reference model of the watch-side Adafruit File Transfer server.

This is the executable specification that ``src/fts_engine.cpp`` ports: same
stream parsing, path confinement, status codes and write semantics. It serves a real directory (``root``), which stands for the watch's
``/home/ceres``.

Rules:
- Requests are a byte stream: a command and its path may be split over several
  writes, and the WRITE_DATA payload follows its header in later writes.
- Protocol ``/`` is ``root``. ``.`` and ``..`` components and empty names are
  refused. The parent directory is resolved with symlinks and must stay inside
  ``root``; read and list also follow a final symlink only if its target stays
  inside. A peer cannot create symlinks, so this check has no race with it.
- WRITE truncates the file to ``offset`` first (keeping the prefix, zero-filling
  a gap), stores data as it arrives, and truncates to ``total`` at the end.
- Every reply message starts a new notification and is split at the
  notification size; list entries are separate messages. Adafruit's client
  needs each reply header inside one notification (true from ATT MTU 31).
- Errors: ``ERROR_NO_FILE`` when the target is missing, ``ERROR_READ_ONLY``
  on EACCES/EPERM/EROFS, ``ERROR`` otherwise.
"""

from __future__ import annotations

import errno
import os
import shutil
import stat
import sys
from collections.abc import Generator
from pathlib import Path

import fts

Need = Generator[int, bytes, None]


class Refused(Exception):
    def __init__(self, status: int) -> None:
        super().__init__(status)
        self.status = status


def status_for(error: OSError) -> int:
    if error.errno in (errno.ENOENT, errno.ENOTDIR):
        return fts.ERROR_NO_FILE
    if error.errno in (errno.EACCES, errno.EPERM, errno.EROFS):
        return fts.ERROR_READ_ONLY
    return fts.ERROR


class ServerModel:
    def __init__(
        self, root: Path, *, notify_size: int = 20, free_space: int = 4096, max_read: int = 65536
    ) -> None:
        self.root = Path(os.path.realpath(root))
        self.notify_size = notify_size
        self.free_space = free_space
        self.max_read = max_read
        self.notifications: list[bytes] = []
        self.reset()

    # -- transport -----------------------------------------------------------------
    def reset(self) -> None:
        """A disconnect: drop any half-parsed command."""
        self._buf = bytearray()
        self._proc = self._serve()
        self._need = next(self._proc)

    def feed(self, data: bytes) -> None:
        self._buf += data
        while len(self._buf) >= self._need:
            chunk = bytes(self._buf[: self._need])
            del self._buf[: self._need]
            self._need = self._proc.send(chunk)

    def _notify(self, data: bytes) -> None:
        for start in range(0, len(data), self.notify_size):
            self.notifications.append(data[start : start + self.notify_size])

    # -- paths -----------------------------------------------------------------------
    def _inside(self, path: str) -> bool:
        return path == str(self.root) or path.startswith(str(self.root) + os.sep)

    def _resolve(self, raw: bytes, *, follow: bool) -> Path:
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError:
            raise Refused(fts.ERROR) from None
        if not text.startswith("/") or "\0" in text:
            raise Refused(fts.ERROR)
        parts = [p for p in text.split("/") if p]
        if any(p in (".", "..") for p in parts):
            raise Refused(fts.ERROR)
        if not parts:
            return self.root
        parent = os.path.realpath(self.root.joinpath(*parts[:-1]))
        if not self._inside(parent):
            raise Refused(fts.ERROR)
        target = Path(parent) / parts[-1]
        if follow and target.is_symlink():
            real = os.path.realpath(target)
            if not self._inside(real):
                raise Refused(fts.ERROR)
            return Path(real)
        return target

    # -- protocol --------------------------------------------------------------------
    def _serve(self) -> Need:
        handlers = {
            fts.READ: self._read,
            fts.WRITE: self._write,
            fts.DELETE: self._delete,
            fts.MKDIR: self._mkdir,
            fts.LISTDIR: self._listdir,
            fts.MOVE: self._move,
        }
        while True:
            command = (yield 1)[0]
            handler = handlers.get(command)
            if handler is None:
                continue  # unknown or stray byte (e.g. pacing after an error): skip it
            yield from handler(command)

    def _read(self, command: int) -> Need:
        rest = yield fts.READ_REQ.size - 1
        _, path_len, offset, chunk = fts.READ_REQ.unpack(bytes([command]) + rest)
        raw = yield path_len
        try:
            path = self._resolve(raw, follow=True)
            with open(path, "rb") as f:
                total = os.fstat(f.fileno()).st_size
                position = offset
                while True:
                    f.seek(position)
                    data = f.read(min(chunk, self.max_read, max(total - position, 0)))
                    self._notify(fts.READ_REPLY.pack(fts.READ_DATA, fts.OK, position, total, len(data)) + data)
                    position += len(data)
                    if position >= total or not data:
                        return
                    pacing = yield fts.READ_PACING_REQ.size
                    cmd, status, position, chunk = fts.READ_PACING_REQ.unpack(pacing)
                    if cmd != fts.READ_PACING or status != fts.OK:
                        return
        except Refused as r:
            self._notify(fts.READ_REPLY.pack(fts.READ_DATA, r.status, offset, 0, 0))
        except OSError as e:
            self._notify(fts.READ_REPLY.pack(fts.READ_DATA, status_for(e), offset, 0, 0))

    def _write(self, command: int) -> Need:
        rest = yield fts.WRITE_REQ.size - 1
        _, path_len, offset, mtime, total = fts.WRITE_REQ.unpack(bytes([command]) + rest)
        raw = yield path_len

        def reply(status: int, position: int, stored: int, free: int) -> None:
            self._notify(fts.WRITE_REPLY.pack(fts.WRITE_PACING, status, position, stored, free))

        try:
            if offset > total:
                raise Refused(fts.ERROR)
            path = self._resolve(raw, follow=False)
            if path == self.root or path.is_symlink() or path.is_dir():
                raise Refused(fts.ERROR)
            fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_NOFOLLOW, 0o644)
        except Refused as r:
            reply(r.status, offset, 0, 0)
            return
        except OSError as e:
            reply(status_for(e), offset, 0, 0)
            return
        position = offset
        try:
            os.ftruncate(fd, offset)
            while position < total:
                reply(fts.OK, position, 0, min(self.free_space, total - position))
                header = yield fts.WRITE_DATA_REQ.size
                cmd, status, at, size = fts.WRITE_DATA_REQ.unpack(header)
                if cmd != fts.WRITE_DATA or status != fts.OK or at != position:
                    return  # client aborted; the file keeps what was written
                # Defensive: the grant never exceeds the remaining size, so neither may the data.
                size = min(size, total - position)
                data = yield size
                os.pwrite(fd, data, position)
                position += size
            os.ftruncate(fd, total)
            os.utime(fd, ns=(mtime, mtime))
            reply(fts.OK, total, os.fstat(fd).st_mtime_ns, 0)
        except OSError as e:
            reply(status_for(e), position, 0, 0)
        finally:
            os.close(fd)

    def _delete(self, command: int) -> Need:
        rest = yield fts.PATH_REQ.size - 1
        _, path_len = fts.PATH_REQ.unpack(bytes([command]) + rest)
        raw = yield path_len
        try:
            path = self._resolve(raw, follow=False)
            if path == self.root:
                raise Refused(fts.ERROR)
            if path.is_dir() and not path.is_symlink():
                shutil.rmtree(path)
            else:
                path.unlink()
            status = fts.OK
        except Refused as r:
            status = r.status
        except OSError as e:
            status = status_for(e)
        self._notify(fts.STATUS_REPLY.pack(fts.DELETE_STATUS, status))

    def _mkdir(self, command: int) -> Need:
        rest = yield fts.MKDIR_REQ.size - 1
        _, path_len, mtime = fts.MKDIR_REQ.unpack(bytes([command]) + rest)
        raw = yield path_len
        stored = 0
        try:
            text = raw.decode("utf-8", "strict") if raw else ""
            parts = [p for p in text.split("/") if p]
            if not text.startswith("/") or any(p in (".", "..") for p in parts):
                raise Refused(fts.ERROR)
            # Create parents one level at a time so each new level is confined.
            for depth in range(1, len(parts) + 1):
                path = self._resolve(("/" + "/".join(parts[:depth])).encode(), follow=True)
                if not path.exists():
                    path.mkdir()
                elif not path.is_dir():
                    raise Refused(fts.ERROR)
            path = self._resolve(raw, follow=True)
            if parts:
                os.utime(path, ns=(mtime, mtime))
            stored = path.stat().st_mtime_ns
            status = fts.OK
        except UnicodeDecodeError:
            status = fts.ERROR
        except Refused as r:
            status = r.status
        except OSError as e:
            status = status_for(e)
        self._notify(fts.MKDIR_REPLY.pack(fts.MKDIR_STATUS, status, stored))

    def _listdir(self, command: int) -> Need:
        rest = yield fts.PATH_REQ.size - 1
        _, path_len = fts.PATH_REQ.unpack(bytes([command]) + rest)
        raw = yield path_len
        try:
            path = self._resolve(raw, follow=True)
            names = sorted(os.listdir(path))
        except Refused as r:
            self._notify(fts.LISTDIR_REPLY.pack(fts.LISTDIR_ENTRY, r.status, 0, 0, 0, 0, 0, 0))
            return
        except OSError as e:
            self._notify(fts.LISTDIR_REPLY.pack(fts.LISTDIR_ENTRY, status_for(e), 0, 0, 0, 0, 0, 0))
            return
        # One notification sequence per entry: Adafruit's client needs every
        # 28-byte header inside a single notification.
        for index, name in enumerate(names):
            st = os.lstat(path / name)
            if stat.S_ISLNK(st.st_mode):
                real = os.path.realpath(path / name)
                if self._inside(real) and os.path.exists(real):
                    st = os.stat(real)
            is_dir = stat.S_ISDIR(st.st_mode)
            encoded = name.encode("utf-8", "surrogateescape")
            header = fts.LISTDIR_REPLY.pack(
                fts.LISTDIR_ENTRY,
                fts.OK,
                len(encoded),
                index,
                len(names),
                fts.FLAG_DIRECTORY if is_dir else 0,
                st.st_mtime_ns,
                0 if is_dir else st.st_size & 0xFFFFFFFF,
            )
            self._notify(header + encoded)
        self._notify(fts.LISTDIR_REPLY.pack(fts.LISTDIR_ENTRY, fts.OK, 0, len(names), len(names), 0, 0, 0))

    def _move(self, command: int) -> Need:
        rest = yield fts.MOVE_REQ.size - 1
        _, old_len, new_len = fts.MOVE_REQ.unpack(bytes([command]) + rest)
        old_raw = yield old_len
        yield 1  # separator byte, any value
        new_raw = yield new_len
        try:
            old = self._resolve(old_raw, follow=False)
            new = self._resolve(new_raw, follow=False)
            if self.root in (old, new):
                raise Refused(fts.ERROR)
            if not old.exists() and not old.is_symlink():
                raise Refused(fts.ERROR_NO_FILE)
            os.rename(old, new)
            status = fts.OK
        except Refused as r:
            status = r.status
        except OSError as e:
            status = status_for(e)
        self._notify(fts.STATUS_REPLY.pack(fts.MOVE_STATUS, status))
