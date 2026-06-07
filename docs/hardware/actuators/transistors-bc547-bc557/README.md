# Transistors — BC547 (NPN) / BC557 (PNP)

> Optional level-shift / buffer parts. Used **only** if a relay clone needs more
> than 3.3 V on its IN pin to trigger reliably from the ESP32 GPIO.
> Purchasing: [COMPONENTS.md](../../../../COMPONENTS.md) #20, #21.
> Datasheets: [BC547.pdf](../../datasheets/BC547.pdf) · [BC557.pdf](../../datasheets/BC557.pdf).

| Parameter | BC547 (NPN) | BC557 (PNP) |
|---|---|---|
| I_C max | 100 mA | −100 mA |
| V_CE max | 45 V | −45 V |
| V_CE(sat) | ≤ 0.3 V | ≤ −0.65 V |
| h_FE | ~110–800 | ~110–800 |
| Package | TO-92 | TO-92 |

## When you need them

The relay module is HIGH-active and accepts 3.3 V on IN when jumpered correctly,
so **in the nominal build these are not used**. They exist as a fallback for a
clone whose opto-input needs ~5 V:

```
3.3 V GPIO ── 1 kΩ ── base (BC547)
collector ── relay IN
emitter   ── GND
relay IN pulled up to 5 V via the module's own resistor
```

This inverts the logic (GPIO HIGH → IN pulled LOW), so if you add this stage,
flip `kPumpRelayActiveHigh` accordingly and re-test the safe-on-reset behaviour.

> Keep the base resistor (1 kΩ) — direct GPIO-to-base would exceed the 40 mA
> source limit ([chip I/O](../../reference/canonical-values.md#chip)).
