"""
conftest.py
-----------
pytest configuration loaded before any test module is imported.

Sets TREASURE_RUNNER_DIST so that bindings._find_library() locates
libbackend.so regardless of the working directory from which pytest
is invoked.
"""

import os
from pathlib import Path

# Compute the repo root relative to this conftest.py
# (python/tests/conftest.py  ->  4 parents up = repo root)
_REPO_ROOT = Path(__file__).resolve().parents[2]
_DIST_DIR  = _REPO_ROOT / "dist"

if _DIST_DIR.exists():
    os.environ.setdefault("TREASURE_RUNNER_DIST", str(_DIST_DIR))
