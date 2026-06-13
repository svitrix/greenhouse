from bot.ui import sensor_state


def test_air_temp_bands():
    assert sensor_state.tone_for("air_temp", 5) == "cold"
    assert sensor_state.tone_for("air_temp", 15) == "cool"
    assert sensor_state.tone_for("air_temp", 24) == "optimal"
    assert sensor_state.tone_for("air_temp", 30) == "warm"
    assert sensor_state.tone_for("air_temp", 40) == "hot"


def test_soil_moisture_bands_match_dashboard():
    # Mirrors services/dashboard/src/lib/sensor-state.ts soilMoistureState.
    assert sensor_state.tone_for("soil_moist", 10) == "dry"
    assert sensor_state.tone_for("soil_moist", 30) == "moist"
    assert sensor_state.tone_for("soil_moist", 80) == "wet"
    assert sensor_state.tone_for("soil_moist", 95) == "saturated"


def test_unbanded_and_nonnumeric_return_none():
    assert sensor_state.tone_for("battery_pct", 87) is None
    assert sensor_state.tone_for("air_temp", None) is None
    assert sensor_state.tone_for("air_temp", True) is None  # bool isn't a reading


def test_every_band_has_an_emoji():
    bands = {"cold", "cool", "optimal", "moist", "warm",
             "humid", "wet", "saturated", "hot", "dry"}
    for tone in bands:
        assert sensor_state.emoji_for(tone) != ""
