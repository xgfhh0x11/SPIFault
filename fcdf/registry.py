"""Backend registry for FCDF.

A backend is any class with a ``name`` attribute and a
``run(campaign) -> CampaignReport`` method. Rather than a hard-coded
table in the runner, backends register themselves with the
``@register_backend`` decorator, so a new backend (e.g. a real hardware
rig — ChipWhisperer, rpi_spidev, sigrok) can be added without editing
core runner code. The simulator is the reference registration.
"""

from __future__ import annotations

# name -> backend class
BACKENDS: dict = {}


def register_backend(cls):
    """Class decorator: register ``cls`` under its ``name`` attribute.

    Idempotent for the same class; raises if two different classes claim
    the same name (a real conflict worth surfacing loudly).
    """
    name = getattr(cls, "name", None)
    if not name:
        raise ValueError(f"backend {cls.__name__!r} must define a non-empty 'name'")
    existing = BACKENDS.get(name)
    if existing is not None and existing is not cls:
        raise ValueError(
            f"backend name {name!r} already registered to {existing.__name__}"
        )
    BACKENDS[name] = cls
    return cls


def get_backend(name: str):
    """Return the backend class registered under ``name``, or ``None``."""
    return BACKENDS.get(name)


def available_backends() -> list:
    """Sorted list of registered backend names."""
    return sorted(BACKENDS)
