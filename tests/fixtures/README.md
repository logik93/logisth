# Backward-compatibility fixtures

`parameters-0.9.4.1.json` records all 736 compiled parameter specifications from
published commit df868d15d6ee8ed9b83b4de85e95f4811b73eada (logisth v0.9.4.1).
The processor integration test compares every field and host parameter index.
Keep this reference immutable when adding parameters.

`legacy-0.9.4.1/` contains the original MIT-licensed rendering headers from the
same commit. CTest builds the old/current engines independently and compares
20 A/B audio fixtures and voice lifetimes with C silent and tail loops off.
The 2e-6 cross-compiler tolerance allows floating-point contraction differences;
current local Linux renders are byte-identical. This covers the documented
fixtures rather than claiming every possible DAW session is identical.
