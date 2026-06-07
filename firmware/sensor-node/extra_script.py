"""
extra_script.py — sensor-node strict-warning helper
=====================================================
Adds -Wextra, -Wconversion, -Wshadow, -fno-rtti to the compilation of
src/main.cpp only (via projenv).

These flags are intentionally NOT placed in build_flags in platformio.ini
because the Arduino-ESP32 framework sources (Wire.cpp, Preferences.cpp,
WString.h) and the robtillaart/AM2315C lib_dep do not compile cleanly
under them, and PlatformIO applies build_flags to ALL sources.

Our local lib/ sources (gh-infra-sensor-node, gh-app-sensor-node) get
the same strict flags via "build": { "flags": "..." } in their
library.json files.

Our project .cpp files handle the WString.h inline-instantiation noise
via per-include  #pragma GCC diagnostic push / ignored / pop  blocks.
"""

Import("env", "projenv")

STRICT_FLAGS = ["-Wextra", "-Wconversion", "-Wshadow", "-fno-rtti"]

# src/main.cpp — add strict flags so the composition root is checked.
projenv.Append(CXXFLAGS=STRICT_FLAGS)
