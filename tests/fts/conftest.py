"""Run the protocol tests against both server implementations: the Python model and the C++ engine."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fts_engine import EngineProcess, build
from fts_model import ServerModel


@pytest.fixture(scope="session")
def engine_binary(tmp_path_factory) -> Path:
    try:
        return build(tmp_path_factory.mktemp("engine"))
    except FileNotFoundError:
        pytest.skip("no C++ compiler for the server engine")


@pytest.fixture(params=["model", "cpp"])
def make_server(request):
    """Factory with ServerModel's signature, once per implementation."""
    started: list[EngineProcess] = []

    def make(root: Path, **kwargs):
        if request.param == "model":
            return ServerModel(root, **kwargs)
        engine = EngineProcess(request.getfixturevalue("engine_binary"), root, **kwargs)
        started.append(engine)
        return engine

    yield make
    for engine in started:
        assert engine.close() == 0, "engine exited non-zero (sanitizer report on stderr?)"
