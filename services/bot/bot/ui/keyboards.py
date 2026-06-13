"""Inline-keyboard builders. Callback data is kept short ('s:<id>') because
Telegram caps callback_data at 64 bytes and device ids can be long. Button
labels are localised via i18n; the chosen language is passed in by handlers."""
from telegram import InlineKeyboardButton, InlineKeyboardMarkup

from bot import i18n
from bot.ui.formatting import format_device_button


def main_menu(lang: str) -> InlineKeyboardMarkup:
    return InlineKeyboardMarkup(
        [
            [InlineKeyboardButton(i18n.t(lang, "btn_devices"), callback_data="m:devices")],
            [InlineKeyboardButton(i18n.t(lang, "btn_notify"), callback_data="m:notify")],
            [InlineKeyboardButton(i18n.t(lang, "btn_language"), callback_data="m:lang")],
        ]
    )


def device_picker(devices: list[dict], *, prefix: str, lang: str) -> InlineKeyboardMarkup:
    rows = [
        [
            InlineKeyboardButton(
                format_device_button(d), callback_data=f"{prefix}:{d['device_id']}"
            )
        ]
        for d in devices
    ]
    rows.append([InlineKeyboardButton(i18n.t(lang, "btn_back"), callback_data="m:main")])
    return InlineKeyboardMarkup(rows)


def device_card(device_id: str, lang: str) -> InlineKeyboardMarkup:
    """One screen per device: pump on/off, refresh, back to the device list.
    Refresh reuses the 'd:' prefix so it re-renders the same card."""
    return InlineKeyboardMarkup(
        [
            [
                InlineKeyboardButton(i18n.t(lang, "btn_pump_on"), callback_data=f"po:{device_id}"),
                InlineKeyboardButton(i18n.t(lang, "btn_pump_off"), callback_data=f"pf:{device_id}"),
            ],
            [InlineKeyboardButton(i18n.t(lang, "btn_refresh"), callback_data=f"d:{device_id}")],
            [InlineKeyboardButton(i18n.t(lang, "btn_back"), callback_data="m:devices")],
        ]
    )


def notify_toggle(enabled: bool, lang: str) -> InlineKeyboardMarkup:
    key = "btn_notify_disable" if enabled else "btn_notify_enable"
    data = "n:off" if enabled else "n:on"
    return InlineKeyboardMarkup(
        [
            [InlineKeyboardButton(i18n.t(lang, key), callback_data=data)],
            [InlineKeyboardButton(i18n.t(lang, "btn_back"), callback_data="m:main")],
        ]
    )


def language_picker() -> InlineKeyboardMarkup:
    rows = [
        [InlineKeyboardButton(i18n.LANGUAGE_NAMES[code], callback_data=f"lang:{code}")]
        for code in i18n.SUPPORTED
    ]
    rows.append([InlineKeyboardButton("⬅️", callback_data="m:main")])
    return InlineKeyboardMarkup(rows)


def back_to_main(lang: str) -> InlineKeyboardMarkup:
    return InlineKeyboardMarkup(
        [[InlineKeyboardButton(i18n.t(lang, "btn_menu"), callback_data="m:main")]]
    )
