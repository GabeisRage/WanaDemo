"""Python integration hooks for the Wana Works plugin.

These hooks are intended to be called from Unreal's Python environment so WanaAI
can enrich runtime decisions without replacing deterministic gameplay logic.
"""

from __future__ import annotations


def classify_object(object_name: str) -> dict:
    """Return a placeholder classification payload for WIT."""
    return {
        "object_name": object_name,
        "classification": "unknown",
        "confidence": 0.0,
    }


def summarize_player_profile(signal_names: list[str]) -> dict:
    """Return a lightweight summary that WAY can consume."""
    return {
        "dominant_preferences": signal_names[:3],
        "profile_size": len(signal_names),
    }
