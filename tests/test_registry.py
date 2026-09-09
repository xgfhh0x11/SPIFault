"""Backend registry tests — the registry replaces the previously
hard-coded BACKENDS dict so a new backend can register without editing
the runner."""
import pytest

from fcdf import register_backend, get_backend, available_backends
from fcdf.simulator import SimulatorBackend


def test_simulator_is_registered():
    assert "simulator" in available_backends()
    assert get_backend("simulator") is SimulatorBackend


def test_unknown_backend_resolves_to_none():
    assert get_backend("definitely-not-a-backend") is None


def test_register_backend_adds_and_is_resolvable():
    @register_backend
    class _DummyBackend:
        name = "dummy-test-backend"

        def run(self, campaign):  # pragma: no cover - not executed here
            raise NotImplementedError

    try:
        assert get_backend("dummy-test-backend") is _DummyBackend
        assert "dummy-test-backend" in available_backends()
    finally:
        # keep the global registry clean for other tests
        from fcdf.registry import BACKENDS
        BACKENDS.pop("dummy-test-backend", None)


def test_register_backend_requires_a_name():
    with pytest.raises(ValueError):
        @register_backend
        class _NoName:
            pass


def test_register_backend_rejects_conflicting_name():
    with pytest.raises(ValueError):
        @register_backend
        class _Clash:
            name = "simulator"  # already owned by SimulatorBackend
