"""
conftest.py (python/ level)
---------------------------
Loaded by pytest before any test module is imported.

Sets TREASURE_RUNNER_DIST so that bindings._find_library() locates
libbackend.so regardless of the working directory from which pytest
is invoked (repo root, python/, or python/tests/).
"""

import os
import sys
from pathlib import Path

_PYTHON_DIR = Path(__file__).resolve().parent
_DIST_DIR   = _PYTHON_DIR.parent / "dist"

# Ensure treasure_runner is importable
if str(_PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(_PYTHON_DIR))

# Point the C library finder at the dist directory
if _DIST_DIR.exists():
    os.environ.setdefault("TREASURE_RUNNER_DIST", str(_DIST_DIR))
