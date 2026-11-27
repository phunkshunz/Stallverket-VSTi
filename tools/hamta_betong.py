#!/usr/bin/env python3
"""Hämtar kit och mönster från BETONG till STÄLLVERKETs kassett.

    python3 tools/hamta_betong.py /sökväg/till/Betong-VSTi

Skriver src/KassettData.h. Ljuden själva ligger i Betong-VSTi/assets/library
och följer med i zippen som mappen Ljudbank (se .github/workflows/build.yml).
"""
import sys, pathlib
betong = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "../Betong-VSTi")
src = (betong / "src" / "Presets.h").read_text(encoding="utf-8")
a = src.index("struct KitPreset")
b = src.index("\n", src.index("constexpr int kNumPatterns")) + 1
ut = pathlib.Path(__file__).resolve().parent.parent / "src" / "KassettData.h"
gammal = ut.read_text(encoding="utf-8")
huvud = gammal[: gammal.index("namespace kassett\n{\n") + len("namespace kassett\n{\n")]
ut.write_text(huvud + src[a:b] + "} // namespace kassett\n", encoding="utf-8")
print("skrev", ut)
