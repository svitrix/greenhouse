import pytest
from pydantic import ValidationError

from app.schemas.pairing import PairingClaimIn, PairingOpenIn


def test_open_default_ttl_is_300():
    p = PairingOpenIn()
    assert p.ttl_seconds == 300


def test_open_ttl_below_60_invalid():
    with pytest.raises(ValidationError):
        PairingOpenIn(ttl_seconds=59)


def test_open_ttl_above_3600_invalid():
    with pytest.raises(ValidationError):
        PairingOpenIn(ttl_seconds=3601)


def _claim_kwargs(**over):
    base = {
        "claim_code": "847291",
        "device_id":  "gh-a1b2c3d4",
        "mac":        "aa:bb:cc:dd:ee:ff",
        "fw_version": "0.4.0",
        "profile_id": "gh-coordinator-v1",
    }
    return {**base, **over}


def test_claim_happy_path():
    PairingClaimIn(**_claim_kwargs())  # no raise


def test_claim_code_must_be_6_digits():
    with pytest.raises(ValidationError):
        PairingClaimIn(**_claim_kwargs(claim_code="12345"))
    with pytest.raises(ValidationError):
        PairingClaimIn(**_claim_kwargs(claim_code="abcdef"))


def test_claim_mac_lowercase_hex_with_colons():
    with pytest.raises(ValidationError):
        PairingClaimIn(**_claim_kwargs(mac="AA:BB:CC:DD:EE:FF"))  # uppercase
    with pytest.raises(ValidationError):
        PairingClaimIn(**_claim_kwargs(mac="aabbccddeeff"))       # no colons


def test_claim_profile_id_required():
    with pytest.raises(ValidationError):
        PairingClaimIn(**{k: v for k, v in _claim_kwargs().items() if k != "profile_id"})
