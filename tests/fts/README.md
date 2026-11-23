# Adafruit File Transfer protocol tests

Host tests for `src/fts_engine.cpp`, the protocol engine behind
`FileTransferService` (service `0xFEBB`, protocol version 4). No watch, Qt or
BlueZ needed: Python 3.10+, pytest and g++ or clang++.

    pytest tests/fts

- `fts_model.py`: reference server in Python, the executable specification the
  engine ports (stream parsing, path confinement, status codes, write semantics).
- `fts_engine_host.cpp` + `fts_engine.py`: the C++ engine built with
  ASan/UBSan and driven over stdin/stdout through the model's interface.
- `conftest.py`: every test runs against both servers (`[model]` and `[cpp]`).
- `fts.py`: an async protocol client, used as the test client.
- `vendor/`: Adafruit's own CircuitPython client, unmodified, run against both
  servers as an independent check of the wire format.

`fts.py`, the model and the tests come from
[asteroid-ble-shell](https://github.com/monperrus/asteroid-ble-shell), whose
`watch-fs` tool uses this client against the watch.
