"""Adafruit BLE File Transfer protocol, version 4: constants, codec and async client.

Wire format: https://github.com/adafruit/Adafruit_CircuitPython_BLE_File_Transfer
(README.rst and ``adafruit_ble_file_transfer.py``). All integers are little
endian; paths are UTF-8, absolute, with an explicit length and no terminator.

The client treats notifications as a byte stream: a reply may be split over
several notifications, and several replies may share one. It is transport
agnostic so the same code runs over Bleak and against in-memory test servers.
"""

from __future__ import annotations

import asyncio
import struct
import time
from collections.abc import Awaitable, Callable
from dataclasses import dataclass

SERVICE_UUID = "0000febb-0000-1000-8000-00805f9b34fb"
VERSION_UUID = "adaf0100-4669-6c65-5472-616e73666572"
TRANSFER_UUID = "adaf0200-4669-6c65-5472-616e73666572"
PROTOCOL_VERSION = 4

READ = 0x10
READ_DATA = 0x11
READ_PACING = 0x12
WRITE = 0x20
WRITE_PACING = 0x21
WRITE_DATA = 0x22
DELETE = 0x30
DELETE_STATUS = 0x31
MKDIR = 0x40
MKDIR_STATUS = 0x41
LISTDIR = 0x50
LISTDIR_ENTRY = 0x51
MOVE = 0x60
MOVE_STATUS = 0x61

OK = 0x01
ERROR = 0x02
ERROR_NO_FILE = 0x03
ERROR_PROTOCOL = 0x04
ERROR_READ_ONLY = 0x05

FLAG_DIRECTORY = 0x01

# Request headers (what the client sends) and reply headers (what the server notifies).
READ_REQ = struct.Struct("<BxHII")  # cmd, path len, offset, chunk size
READ_REPLY = struct.Struct("<BBxxIII")  # cmd, status, offset, total length, chunk length
READ_PACING_REQ = struct.Struct("<BBxxII")  # cmd, status, offset, chunk size
WRITE_REQ = struct.Struct("<BxHIQI")  # cmd, path len, offset, mtime ns, total size
WRITE_REPLY = struct.Struct("<BBxxIQI")  # cmd, status, offset, truncated mtime ns, free space
WRITE_DATA_REQ = struct.Struct("<BBxxII")  # cmd, status, offset, data size
PATH_REQ = struct.Struct("<BxH")  # DELETE and LISTDIR: cmd, path len
STATUS_REPLY = struct.Struct("<BB")  # DELETE_STATUS and MOVE_STATUS: cmd, status
MKDIR_REQ = struct.Struct("<BxHxxxxQ")  # cmd, path len, mtime ns
MKDIR_REPLY = struct.Struct("<BBxxxxxxQ")  # cmd, status, truncated mtime ns
LISTDIR_REPLY = struct.Struct("<BBHIIIQI")  # cmd, status, path len, index, total, flags, mtime, size
MOVE_REQ = struct.Struct("<BxHH")  # cmd, old path len, new path len; old, 1 pad byte, new

DEFAULT_READ_CHUNK = 65536
STATUS_NAMES = {
    OK: "ok",
    ERROR: "error",
    ERROR_NO_FILE: "no such file",
    ERROR_PROTOCOL: "protocol error",
    ERROR_READ_ONLY: "read-only",
}


class FtsError(Exception):
    """The server answered a command with a non-OK status."""

    def __init__(self, command: str, path: str, status: int) -> None:
        self.command = command
        self.path = path
        self.status = status
        name = STATUS_NAMES.get(status, "unknown status")
        super().__init__(f"{command} {path}: {name} (0x{status:02x})")


class ProtocolError(Exception):
    """The server sent bytes the protocol does not allow at this point."""


@dataclass(frozen=True)
class Entry:
    """One directory entry as returned by LISTDIR."""

    name: str
    is_dir: bool
    size: int
    mtime_ns: int


def now_ns() -> int:
    return time.time_ns()


class Client:
    """Async Adafruit File Transfer client over an abstract byte transport.

    ``send`` writes one GATT Write Command (callers never exceed ``max_write``).
    Notifications are fed in with ``on_notify``; ``timeout`` bounds every wait
    for server bytes.
    """

    def __init__(
        self,
        send: Callable[[bytes], Awaitable[None]],
        max_write: int,
        *,
        timeout: float = 10.0,
    ) -> None:
        if max_write <= 0:
            raise ValueError("max_write must be positive")
        self._send = send
        self.max_write = max_write
        self.timeout = timeout
        self._rx = bytearray()
        self._rx_event = asyncio.Event()
        self._lock = asyncio.Lock()

    def on_notify(self, data: bytes | bytearray) -> None:
        self._rx += data
        self._rx_event.set()

    async def _write(self, data: bytes) -> None:
        for start in range(0, len(data), self.max_write):
            await self._send(bytes(data[start : start + self.max_write]))

    async def _take(self, size: int) -> bytes:
        while len(self._rx) < size:
            self._rx_event.clear()
            try:
                await asyncio.wait_for(self._rx_event.wait(), self.timeout)
            except asyncio.TimeoutError:
                raise TimeoutError(
                    f"watch sent {len(self._rx)} of {size} expected bytes "
                    f"within {self.timeout:g} s"
                ) from None
        data = bytes(self._rx[:size])
        del self._rx[:size]
        return data

    async def _reply(self, header: struct.Struct, command: int) -> tuple:
        fields = header.unpack(await self._take(header.size))
        if fields[0] != command:
            raise ProtocolError(f"expected reply 0x{command:02x}, got 0x{fields[0]:02x}")
        return fields

    async def read(
        self, path: str, *, offset: int = 0, chunk_size: int = DEFAULT_READ_CHUNK
    ) -> bytes:
        """Return the file's bytes from ``offset`` to its end."""
        encoded = path.encode()
        async with self._lock:
            await self._write(READ_REQ.pack(READ, len(encoded), offset, chunk_size) + encoded)
            out = bytearray()
            position = offset
            while True:
                _, status, chunk_offset, total, length = await self._reply(READ_REPLY, READ_DATA)
                if status != OK:
                    raise FtsError("read", path, status)
                if chunk_offset != position:
                    raise ProtocolError(f"read chunk at {chunk_offset}, expected {position}")
                out += await self._take(length)
                position += length
                if position >= total or length == 0:
                    return bytes(out)
                await self._write(
                    READ_PACING_REQ.pack(READ_PACING, OK, position, min(chunk_size, total - position))
                )

    async def write(
        self,
        path: str,
        data: bytes,
        *,
        offset: int = 0,
        mtime_ns: int | None = None,
        progress: Callable[[int, int], None] | None = None,
    ) -> int:
        """Write ``data`` at ``offset``, truncate the file to ``offset + len(data)``.

        Returns the modification time the server stored, in ns since the epoch.
        """
        encoded = path.encode()
        total = offset + len(data)
        stamp = now_ns() if mtime_ns is None else mtime_ns
        async with self._lock:
            await self._write(WRITE_REQ.pack(WRITE, len(encoded), offset, stamp, total) + encoded)
            while True:
                _, status, position, stored, free = await self._reply(WRITE_REPLY, WRITE_PACING)
                if status != OK:
                    raise FtsError("write", path, status)
                if position == total and free == 0:
                    return stored
                if not offset <= position < total or free == 0:
                    raise ProtocolError(f"write pacing at {position} with {free} free of {total}")
                size = min(free, total - position)
                await self._write(WRITE_DATA_REQ.pack(WRITE_DATA, OK, position, size))
                await self._write(data[position - offset : position - offset + size])
                if progress:
                    progress(position + size - offset, len(data))

    async def delete(self, path: str) -> None:
        """Delete a file, or a directory with everything below it."""
        encoded = path.encode()
        async with self._lock:
            await self._write(PATH_REQ.pack(DELETE, len(encoded)) + encoded)
            _, status = await self._reply(STATUS_REPLY, DELETE_STATUS)
        if status != OK:
            raise FtsError("delete", path, status)

    async def mkdir(self, path: str, *, mtime_ns: int | None = None) -> int:
        """Create a directory and its missing parents; return the stored mtime."""
        encoded = path.encode()
        stamp = now_ns() if mtime_ns is None else mtime_ns
        async with self._lock:
            await self._write(MKDIR_REQ.pack(MKDIR, len(encoded), stamp) + encoded)
            _, status, stored = await self._reply(MKDIR_REPLY, MKDIR_STATUS)
        if status != OK:
            raise FtsError("mkdir", path, status)
        return stored

    async def listdir(self, path: str) -> list[Entry]:
        encoded = path.encode()
        entries: list[Entry] = []
        async with self._lock:
            await self._write(PATH_REQ.pack(LISTDIR, len(encoded)) + encoded)
            while True:
                _, status, name_len, index, total, flags, mtime, size = await self._reply(
                    LISTDIR_REPLY, LISTDIR_ENTRY
                )
                name = (await self._take(name_len)).decode("utf-8", "replace")
                if status != OK:
                    raise FtsError("listdir", path, status)
                if index >= total:
                    return entries
                entries.append(Entry(name, bool(flags & FLAG_DIRECTORY), size, mtime))

    async def move(self, old: str, new: str) -> None:
        old_b, new_b = old.encode(), new.encode()
        async with self._lock:
            await self._write(MOVE_REQ.pack(MOVE, len(old_b), len(new_b)) + old_b + b"\0" + new_b)
            _, status = await self._reply(STATUS_REPLY, MOVE_STATUS)
        if status != OK:
            raise FtsError("move", f"{old} -> {new}", status)
