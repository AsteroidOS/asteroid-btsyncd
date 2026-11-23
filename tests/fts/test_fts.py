"""Adafruit File Transfer: our client and Adafruit's reference client against the server model."""

from __future__ import annotations

import asyncio
import importlib.util
import os
import sys
import types
import uuid
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import fts

# -- our client over an in-memory link ------------------------------------------------


class Link:
    """Connects a Client to a server (model or C++ engine); notifications are delivered after each write."""

    def __init__(self, model, max_write: int) -> None:
        self.model = model
        self.client = fts.Client(self.send, max_write, timeout=1)

    async def send(self, data: bytes) -> None:
        assert len(data) <= self.client.max_write
        self.model.feed(data)
        while self.model.notifications:
            self.client.on_notify(self.model.notifications.pop(0))


def run(coro):
    return asyncio.run(coro)


@pytest.fixture(params=[(20, 7, 5), (20, 20, 4096), (512, 509, 65536)], ids=["tiny", "mtu23", "mtu512"])
def link(request, tmp_path: Path, make_server) -> Link:
    max_write, notify, free = request.param
    return Link(make_server(tmp_path, notify_size=notify, free_space=free), max_write)


def test_write_read_roundtrip(link: Link, tmp_path: Path) -> None:
    data = os.urandom(10_000)
    stored = run(link.client.write("/a.bin", data, mtime_ns=1_700_000_000_123_456_789))
    assert (tmp_path / "a.bin").read_bytes() == data
    assert stored == (tmp_path / "a.bin").stat().st_mtime_ns == 1_700_000_000_123_456_789
    assert run(link.client.read("/a.bin", chunk_size=777)) == data
    assert run(link.client.read("/a.bin", offset=9_990)) == data[9_990:]


def test_empty_file(link: Link, tmp_path: Path) -> None:
    run(link.client.write("/empty", b""))
    assert (tmp_path / "empty").read_bytes() == b""
    assert run(link.client.read("/empty")) == b""


def test_overwrite_shrinks_file(link: Link, tmp_path: Path) -> None:
    (tmp_path / "f").write_bytes(b"x" * 100)
    run(link.client.write("/f", b"short"))
    assert (tmp_path / "f").read_bytes() == b"short"


def test_offset_write_keeps_prefix_and_zero_fills_gap(link: Link, tmp_path: Path) -> None:
    (tmp_path / "f").write_bytes(b"abcdef")
    run(link.client.write("/f", b"XY", offset=3))
    assert (tmp_path / "f").read_bytes() == b"abcXY"
    run(link.client.write("/f", b"Z", offset=8))
    assert (tmp_path / "f").read_bytes() == b"abcXY\0\0\0Z"


def test_mkdir_list_move_delete(link: Link, tmp_path: Path) -> None:
    run(link.client.mkdir("/music/album", mtime_ns=1_600_000_000_000_000_000))
    assert (tmp_path / "music/album").is_dir()
    run(link.client.write("/music/album/t.mp3", b"123"))
    entries = run(link.client.listdir("/music/album"))
    assert [(e.name, e.is_dir, e.size) for e in entries] == [("t.mp3", False, 3)]
    assert [(e.name, e.is_dir) for e in run(link.client.listdir("/music"))] == [("album", True)]
    run(link.client.move("/music/album/t.mp3", "/music/t2.mp3"))
    assert (tmp_path / "music/t2.mp3").read_bytes() == b"123"
    run(link.client.delete("/music"))
    assert not (tmp_path / "music").exists()
    assert run(link.client.listdir("/")) == []


def test_many_entries_listed(link: Link, tmp_path: Path) -> None:
    for i in range(40):
        (tmp_path / f"file-{i:02d}-with-a-long-name.txt").write_text(str(i))
    names = [e.name for e in run(link.client.listdir("/"))]
    assert names == sorted(f"file-{i:02d}-with-a-long-name.txt" for i in range(40))


@pytest.mark.parametrize(
    ("op", "status"),
    [
        (lambda c: c.read("/missing"), fts.ERROR_NO_FILE),
        (lambda c: c.listdir("/missing"), fts.ERROR_NO_FILE),
        (lambda c: c.delete("/missing"), fts.ERROR_NO_FILE),
        (lambda c: c.move("/missing", "/x"), fts.ERROR_NO_FILE),
        (lambda c: c.write("/no/parent/f", b"x"), fts.ERROR_NO_FILE),
        (lambda c: c.read("/../etc/passwd"), fts.ERROR),
        (lambda c: c.read("/a/./b"), fts.ERROR),
        (lambda c: c.read("relative"), fts.ERROR),
        (lambda c: c.delete("/"), fts.ERROR),
        (lambda c: c.write("/", b"x"), fts.ERROR),
        (lambda c: c.mkdir("/../outside"), fts.ERROR),
        (lambda c: c.move("/", "/x"), fts.ERROR),
    ],
)
def test_refusals(link: Link, op, status: int) -> None:
    with pytest.raises(fts.FtsError) as raised:
        run(op(link.client))
    assert raised.value.status == status
    # The stream stays in sync after an error.
    run(link.client.write("/after", b"ok"))
    assert run(link.client.read("/after")) == b"ok"


def test_symlinks_cannot_escape_root(tmp_path: Path, make_server) -> None:
    root, outside = tmp_path / "root", tmp_path / "outside"
    root.mkdir()
    outside.mkdir()
    (outside / "secret").write_text("s")
    (root / "out").symlink_to(outside)
    (root / "secret-link").symlink_to(outside / "secret")
    (root / "inside").write_text("i")
    (root / "inside-link").symlink_to(root / "inside")
    client = Link(make_server(root, notify_size=64), 64).client
    for op in (
        client.read("/out/secret"),
        client.read("/secret-link"),
        client.listdir("/out"),
        client.write("/out/new", b"x"),
        client.write("/secret-link", b"x"),
        client.mkdir("/out/d"),
        client.move("/inside", "/out/moved"),
    ):
        with pytest.raises(fts.FtsError):
            run(op)
    assert sorted(p.name for p in outside.iterdir()) == ["secret"]
    assert (outside / "secret").read_text() == "s"
    assert run(client.read("/inside-link")) == b"i"
    run(client.delete("/out"))  # removes the link, not the target
    assert (outside / "secret").exists() and not (root / "out").is_symlink()


def test_disconnect_mid_write_leaves_partial_file_and_fresh_parser(tmp_path: Path, make_server) -> None:
    model = make_server(tmp_path, notify_size=20, free_space=10)
    model.feed(fts.WRITE_REQ.pack(fts.WRITE, 2, 0, 0, 30) + b"/f")
    model.feed(fts.WRITE_DATA_REQ.pack(fts.WRITE_DATA, fts.OK, 0, 10) + b"0123456789")
    model.reset()
    assert (tmp_path / "f").read_bytes() == b"0123456789"
    model.notifications.clear()
    link = Link(model, 20)
    assert run(link.client.read("/f")) == b"0123456789"


# -- Adafruit's reference client (vendored, unmodified) ---------------------------------


def _load_adafruit():
    """Import the CircuitPython client with stand-ins for its BLE stack."""

    class _Any:
        WRITE_NO_RESPONSE = READ = NOTIFY = ENCRYPT_NO_MITM = 0

        def __init__(self, *args, **kwargs) -> None:
            self.args = args

    class VendorUUID(_Any):
        pass

    stubs = {
        "_bleio": {"PacketBuffer": _Any},
        "adafruit_ble": {},
        "adafruit_ble.attributes": {"Attribute": _Any},
        "adafruit_ble.characteristics": {"Characteristic": _Any, "ComplexCharacteristic": _Any},
        "adafruit_ble.characteristics.int": {"Uint32Characteristic": _Any},
        "adafruit_ble.services": {"Service": object},
        "adafruit_ble.uuid": {"StandardUUID": _Any, "VendorUUID": VendorUUID},
        "circuitpython_typing": {"ReadableBuffer": bytes, "WriteableBuffer": bytearray},
    }
    saved = {name: sys.modules.get(name) for name in stubs}
    try:
        for name, attrs in stubs.items():
            module = types.ModuleType(name)
            module.__dict__.update(attrs)
            sys.modules[name] = module
        spec = importlib.util.spec_from_file_location(
            "adafruit_ble_file_transfer", HERE / "vendor" / "adafruit_ble_file_transfer.py"
        )
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    finally:
        for name, old in saved.items():
            if old is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = old
    return module


adafruit = _load_adafruit()


def test_uuids_match_adafruit() -> None:
    def as_uuid(char_uuid) -> str:
        # VendorUUID takes the 128-bit value little endian, as BLE sends it.
        return str(uuid.UUID(bytes=bytes(reversed(char_uuid.args[0]))))

    assert as_uuid(adafruit.FileTransferUUID(0x0200)) == fts.TRANSFER_UUID
    assert as_uuid(adafruit.FileTransferUUID(0x0100)) == fts.VERSION_UUID
    for name in ("READ", "WRITE", "DELETE", "MKDIR", "LISTDIR", "MOVE", "OK", "ERROR"):
        assert getattr(adafruit.FileTransferService, name) == getattr(fts, name)


class _PacketBuffer:
    """Synchronous stand-in for _bleio.PacketBuffer wired to the model."""

    def __init__(self, model) -> None:
        self.model = model
        self.outgoing_packet_length = 20
        self.incoming_packet_length = model.notify_size

    def write(self, data) -> None:
        assert len(data) <= self.outgoing_packet_length
        self.writes = getattr(self, "writes", 0) + 1
        assert self.writes < 100_000, "Adafruit client is looping (e.g. it read a zero grant)"
        self.model.feed(bytes(data))

    def readinto(self, buf) -> int:
        if not self.model.notifications:
            raise AssertionError("Adafruit client waits for a notification the server never sent")
        packet = self.model.notifications[0]
        if len(packet) > len(buf):
            raise ValueError("buffer too small")
        self.model.notifications.pop(0)
        buf[: len(packet)] = packet
        return len(packet)


@pytest.fixture
def reference(tmp_path: Path, make_server):
    model = make_server(tmp_path, notify_size=64, free_space=100)
    service = types.SimpleNamespace(version=4, raw=_PacketBuffer(model))
    return adafruit.FileTransferClient(service), model


def test_adafruit_client_full_session(reference, tmp_path: Path) -> None:
    client, model = reference
    data = os.urandom(1_234)
    client.mkdir("/d/", modification_time=1_650_000_000_000_000_000)
    assert client.write("/d/f.bin", data, modification_time=1_650_000_000_000_000_000) == (
        1_650_000_000_000_000_000
    )
    assert (tmp_path / "d/f.bin").read_bytes() == data
    assert bytes(client.read("/d/f.bin")) == data
    assert bytes(client.read("/d/f.bin", offset=1_000)) == data[1_000:]
    listing = client.listdir("/d/")
    assert [(name, size, flags) for name, size, flags, _ in listing] == [("f.bin", 1_234, 0)]
    client.move("/d/f.bin", "/d/g.bin")
    assert (tmp_path / "d/g.bin").read_bytes() == data
    client.delete("/d/")
    assert client.listdir("/") == []
    with pytest.raises(ValueError):
        client.read("/missing")
    assert not model.notifications
