#!/usr/bin/env bash
# check-hw-consistency.sh — guard against drift between the canonical hardware
# table (docs/hardware/reference/canonical-values.md) and the firmware constants
# that are the runtime truth.
#
# It asserts each firmware constant still holds the value the docs claim. If a
# pin/address/calibration is changed in firmware without updating the doc table
# (or vice-versa), this fails. Run before committing hardware/doc changes.
#
# Usage:  bash scripts/check-hw-consistency.sh
# Exit:   0 = consistent, 1 = drift found.

set -u
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 2

APP="firmware/shared/application/src/AppConfig.hpp"
COORD="firmware/coordinator/lib/application/src/CoordinatorConfig.hpp"
NODE="firmware/sensor-node/lib/application/src/SensorNodeConfig.hpp"

fail=0

# check <description> <file> <extended-regex>
check() {
    local desc="$1" file="$2" re="$3"
    if grep -Eq "$re" "$file"; then
        printf '  ok    %s\n' "$desc"
    else
        printf '  DRIFT %s  (expected /%s/ in %s)\n' "$desc" "$re" "$file"
        fail=1
    fi
}

echo "Hardware consistency check (canonical table vs firmware constants):"

# I²C bus — canonical-values.md#i2c
check "i2c.sda = 6"            "$APP"   'kI2cSdaPin[[:space:]]*=[[:space:]]*6'
check "i2c.scl = 7"            "$APP"   'kI2cSclPin[[:space:]]*=[[:space:]]*7'
check "i2c.freq = 100000"      "$APP"   "kI2cFrequencyHz[[:space:]]*=[[:space:]]*100'000"
check "i2c.sda = 6 (node)"     "$NODE"  'kI2cSdaPin[[:space:]]*=[[:space:]]*6'
check "i2c.scl = 7 (node)"     "$NODE"  'kI2cSclPin[[:space:]]*=[[:space:]]*7'

# I²C devices — canonical-values.md#i2c-devices
check "chirp.addr = 0x20"      "$APP"   'kChirpAddress[[:space:]]*=[[:space:]]*0x20'
check "am2315c.addr = 0x38"    "$APP"   'kAm2315cAddress[[:space:]]*=[[:space:]]*0x38'
check "chirp.addr = 0x20 (node)"   "$NODE" 'kChirpAddress[[:space:]]*=[[:space:]]*0x20'
check "am2315c.addr = 0x38 (node)" "$NODE" 'kAm2315cAddress[[:space:]]*=[[:space:]]*0x38'

# Coordinator GPIO — canonical-values.md#coordinator-gpio
check "pump_relay = 18"        "$APP"   'kPumpRelayGpio[[:space:]]*=[[:space:]]*18'
check "pump_active_high"       "$APP"   'kPumpRelayActiveHigh[[:space:]]*=[[:space:]]*true'
check "boot_button = 9"        "$APP"   'kBootButtonGpio[[:space:]]*=[[:space:]]*9'
check "relay_in1 = 18"         "$COORD" 'kRelayIn1Pin[[:space:]]*=[[:space:]]*18'
check "pump.max_runtime = 20000" "$APP" "kPumpMaxRuntimeMs[[:space:]]*=[[:space:]]*20'000"

# Sensor-node GPIO — canonical-values.md#sensor-node-gpio
check "sensor_power_gate = 4"  "$NODE"  'kSensorPowerGateGpio[[:space:]]*=[[:space:]]*4'
check "battery_adc = 0"        "$NODE"  'kBatteryAdcGpio[[:space:]]*=[[:space:]]*0'
check "battery_divider_r1"     "$NODE"  "kBatteryDividerR1Ohm[[:space:]]*=[[:space:]]*100'000"
check "battery_divider_r2"     "$NODE"  "kBatteryDividerR2Ohm[[:space:]]*=[[:space:]]*100'000"

# Soil calibration default — canonical-values.md#calibration
check "calibration.default raw_dry = 300" "$APP" 'raw_dry[[:space:]]*=[[:space:]]*300'
check "calibration.default raw_wet = 700" "$APP" 'raw_wet[[:space:]]*=[[:space:]]*700'

echo
if [ "$fail" -eq 0 ]; then
    echo "PASS — firmware constants match docs/hardware/reference/canonical-values.md"
else
    echo "FAIL — drift detected. Reconcile the firmware constant and the canonical table."
fi
exit "$fail"
