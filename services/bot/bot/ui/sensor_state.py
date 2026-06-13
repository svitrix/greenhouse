"""Reading value → qualitative state band, ported verbatim from the dashboard's
``services/dashboard/src/lib/sensor-state.ts`` thresholds so the bot and the web
UI agree on what "dry" or "optimal" means. Pure functions, no telegram/i18n
import here — the band's display label is resolved by the caller via i18n
(``band_<tone>`` keys); this module only owns the numeric bands and their emoji."""
from collections.abc import Callable

# tone -> emoji shown next to the localized band label
_TONE_EMOJI = {
    "cold": "❄️",
    "cool": "🔵",
    "optimal": "✅",
    "moist": "✅",
    "warm": "🟠",
    "humid": "💧",
    "wet": "💧",
    "saturated": "🌊",
    "hot": "🔴",
    "dry": "⚠️",
}


def _air_temp(v: float) -> str:
    if v < 10:
        return "cold"
    if v < 18:
        return "cool"
    if v <= 28:
        return "optimal"
    if v <= 32:
        return "warm"
    return "hot"


def _air_humidity(v: float) -> str:
    if v < 40:
        return "dry"
    if v <= 75:
        return "optimal"
    if v <= 90:
        return "humid"
    return "saturated"


def _soil_moist(v: float) -> str:
    if v < 30:
        return "dry"
    if v <= 70:
        return "moist"
    if v <= 90:
        return "wet"
    return "saturated"


def _soil_temp(v: float) -> str:
    if v < 10:
        return "cold"
    if v <= 25:
        return "optimal"
    if v <= 30:
        return "warm"
    return "hot"


_BANDS: dict[str, Callable[[float], str]] = {
    "air_temp": _air_temp,
    "air_humidity": _air_humidity,
    "soil_moist": _soil_moist,
    "soil_temp": _soil_temp,
}


def tone_for(kind: str, value: float) -> str | None:
    """Return the band tone for a reading, or None for kinds without bands
    (e.g. battery) or values that aren't numeric."""
    band = _BANDS.get(kind)
    if band is None or not isinstance(value, (int, float)) or isinstance(value, bool):
        return None
    return band(float(value))


def emoji_for(tone: str) -> str:
    return _TONE_EMOJI.get(tone, "")
