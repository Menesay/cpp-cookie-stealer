#!/usr/bin/env python3
"""
Chrome / Chromium Cookie Decryptor
------------------------------------
Gereksinimler:
    pip install pycryptodome  (Linux/macOS)
    pip install pycryptodome pywin32  (Windows)

Kullanım:
    python decrypt_cookies.py \
        --local-state "Local State" \
        --cookies    "Cookies" \
        --output     "Cookies_decrypted.db"

Desteklenen platformlar: Windows (DPAPI), Linux/macOS (PBKDF2 / raw key)
"""

import argparse
import base64
import json
import os
import platform
import shutil
import sqlite3
import sys
import tempfile

# ── Bağımlılık kontrolleri ──────────────────────────────────────────────────
try:
    from Crypto.Cipher import AES
except ImportError:
    sys.exit("[!] pycryptodome bulunamadı → pip install pycryptodome")

SYSTEM = platform.system()

# ── Anahtar çekme ────────────────────────────────────────────────────────────

def get_encryption_key(local_state_path: str) -> bytes:
    """Local State dosyasından şifreleme anahtarını çeker ve çözer."""
    with open(local_state_path, "r", encoding="utf-8") as f:
        local_state = json.load(f)

    # Base64 kodlu encrypted_key
    encrypted_key_b64 = (
        local_state.get("os_crypt", {}).get("encrypted_key", "")
    )
    if not encrypted_key_b64:
        sys.exit("[!] Local State içinde 'encrypted_key' bulunamadı.")

    encrypted_key = base64.b64decode(encrypted_key_b64)

    # İlk 5 byte "DPAPI" prefix'ini çıkar
    if encrypted_key[:5] == b"DPAPI":
        encrypted_key = encrypted_key[5:]
    else:
        print("[!] DPAPI prefix bulunamadı, ham anahtar denenecek.")

    # Platform'a göre çöz
    if SYSTEM == "Windows":
        return _decrypt_key_windows(encrypted_key)
    elif SYSTEM == "Darwin":
        return _decrypt_key_macos(encrypted_key)
    else:  # Linux
        return _decrypt_key_linux(encrypted_key)


def _decrypt_key_windows(encrypted_key: bytes) -> bytes:
    try:
        import win32crypt
        return win32crypt.CryptUnprotectData(encrypted_key, None, None, None, 0)[1]
    except ImportError:
        sys.exit("[!] Windows için pywin32 gerekli → pip install pywin32")


def _decrypt_key_macos(encrypted_key: bytes) -> bytes:
    """macOS Keychain'den Chrome şifresini alır, PBKDF2 ile anahtar üretir."""
    import subprocess
    result = subprocess.run(
        ["security", "find-generic-password", "-w",
         "-a", "Chrome", "-s", "Chrome Safe Storage"],
        capture_output=True, text=True
    )
    password = result.stdout.strip().encode() if result.returncode == 0 else b"peanuts"

    from hashlib import pbkdf2_hmac
    key = pbkdf2_hmac("sha1", password, b"saltysalt", 1003, dklen=16)
    return key


def _decrypt_key_linux(encrypted_key: bytes) -> bytes:
    """
    Linux'ta iki senaryo:
    1. Chrome v80+ → Local State'deki DPAPI benzeri AES-256-GCM anahtarı
       (genellikle secret-tool / kwallet ile şifreli; bu script raw deneme yapar)
    2. Eski Chrome → PBKDF2 ile 'peanuts' parolasından üretilmiş sabit anahtar
    """
    # Eğer anahtar 32 byte düz ise doğrudan kullan (bazı headless ortamlar)
    if len(encrypted_key) == 32:
        print("[*] Linux: ham 32-byte anahtar kullanılıyor.")
        return encrypted_key

    # Eski Chrome / Chromium sabit türetme
    from hashlib import pbkdf2_hmac
    key = pbkdf2_hmac("sha1", b"peanuts", b"saltysalt", 1, dklen=16)
    print("[*] Linux: PBKDF2 sabit anahtar kullanılıyor.")
    return key


# ── Cookie şifre çözme ───────────────────────────────────────────────────────

def decrypt_cookie_value(encrypted_value: bytes, key: bytes) -> str:
    """
    Chrome cookie değerini çözer.
    Formatlar:
      v10/v11 → AES-256-GCM  (prefix: b'v10' veya b'v11')
      DPAPI   → Windows DPAPI (prefix yoksa)
    """
    if not encrypted_value:
        return ""

    prefix = encrypted_value[:3]

    # ── AES-256-GCM (v10 / v11) ─────────────────────────────────────────────
    if prefix in (b"v10", b"v11"):
        try:
            iv    = encrypted_value[3:15]          # 12 byte nonce
            ct    = encrypted_value[15:-16]        # ciphertext
            tag   = encrypted_value[-16:]          # auth tag
            cipher = AES.new(key, AES.MODE_GCM, nonce=iv)
            return cipher.decrypt_and_verify(ct, tag).decode("utf-8", errors="replace")
        except Exception as e:
            return f"[DECRYPT_ERR: {e}]"

    # ── Windows DPAPI (eski format) ──────────────────────────────────────────
    if SYSTEM == "Windows":
        try:
            import win32crypt
            decrypted = win32crypt.CryptUnprotectData(
                encrypted_value, None, None, None, 0
            )[1]
            return decrypted.decode("utf-8", errors="replace")
        except Exception as e:
            return f"[DPAPI_ERR: {e}]"

    # ── AES-CBC (eski Linux/macOS, 16-byte IV = space*16) ───────────────────
    try:
        iv      = b" " * 16
        ct      = encrypted_value[3:]  # prefix'i at
        cipher  = AES.new(key[:16], AES.MODE_CBC, IV=iv)
        raw     = cipher.decrypt(ct)
        # PKCS7 padding kaldır
        pad_len = raw[-1]
        return raw[:-pad_len].decode("utf-8", errors="replace")
    except Exception as e:
        return f"[CBC_ERR: {e}]"


# ── SQLite okuma & yazma ─────────────────────────────────────────────────────

SCHEMA = """
CREATE TABLE IF NOT EXISTS cookies (
    host_key          TEXT,
    name              TEXT,
    value             TEXT,
    path              TEXT,
    expires_utc       INTEGER,
    is_secure         INTEGER,
    is_httponly       INTEGER,
    has_expires       INTEGER,
    is_persistent     INTEGER,
    priority          INTEGER,
    samesite          INTEGER,
    source_scheme     INTEGER,
    source_port       INTEGER,
    is_same_party     INTEGER,
    last_access_utc   INTEGER,
    creation_utc      INTEGER
);
"""

READ_COLUMNS = [
    "host_key", "name", "encrypted_value", "path",
    "expires_utc", "is_secure", "is_httponly", "has_expires",
    "is_persistent", "priority", "samesite", "source_scheme",
    "source_port", "is_same_party", "last_access_utc", "creation_utc"
]

WRITE_COLUMNS = [c for c in READ_COLUMNS if c != "encrypted_value"]


def process_cookies(cookies_path: str, output_path: str, key: bytes) -> int:
    """Cookies DB'yi oku, çöz, yeni DB'ye yaz. Kaç satır işlendiğini döndür."""

    # Orijinal dosyayı kopyala (Chrome açıksa kilitli olabilir)
    with tempfile.NamedTemporaryFile(delete=False, suffix=".db") as tmp:
        tmp_path = tmp.name
    shutil.copy2(cookies_path, tmp_path)

    try:
        src = sqlite3.connect(tmp_path)
        src.row_factory = sqlite3.Row
        src_cur = src.cursor()

        # Mevcut tablo sütunlarını kontrol et
        src_cur.execute("PRAGMA table_info(cookies)")
        existing_cols = {row["name"] for row in src_cur.fetchall()}

        cols_to_read = [c for c in READ_COLUMNS if c in existing_cols]
        cols_to_write = [c for c in WRITE_COLUMNS if c in existing_cols]

        src_cur.execute(f"SELECT {', '.join(cols_to_read)} FROM cookies")
        rows = src_cur.fetchall()
        src.close()

        # Çıktı DB
        if os.path.exists(output_path):
            os.remove(output_path)
        dst = sqlite3.connect(output_path)
        dst_cur = dst.cursor()
        dst_cur.executescript(SCHEMA)

        inserted = 0
        for row in rows:
            row_dict = dict(row)
            enc_val  = row_dict.pop("encrypted_value", b"")
            row_dict["value"] = decrypt_cookie_value(enc_val, key)

            vals = [row_dict.get(c, None) for c in cols_to_write]
            placeholders = ", ".join(["?"] * len(cols_to_write))
            dst_cur.execute(
                f"INSERT INTO cookies ({', '.join(cols_to_write)}) VALUES ({placeholders})",
                vals
            )
            inserted += 1

        dst.commit()
        dst.close()
        return inserted

    finally:
        os.unlink(tmp_path)


# ── CLI ──────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Chrome Cookie Decryptor — şifreli cookieleri düz metin SQLite'a dönüştürür"
    )
    parser.add_argument(
        "--local-state", required=True,
        metavar="PATH", help="'Local State' dosyasının yolu"
    )
    parser.add_argument(
        "--cookies", required=True,
        metavar="PATH", help="Chrome 'Cookies' SQLite dosyasının yolu"
    )
    parser.add_argument(
        "--output", default="Cookies_decrypted.db",
        metavar="PATH", help="Çıktı SQLite dosyası (varsayılan: Cookies_decrypted.db)"
    )
    args = parser.parse_args()

    # Dosya kontrolleri
    for label, path in [("Local State", args.local_state), ("Cookies", args.cookies)]:
        if not os.path.isfile(path):
            sys.exit(f"[!] {label} bulunamadı: {path}")

    print(f"[*] Platform  : {SYSTEM}")
    print(f"[*] Local State: {args.local_state}")
    print(f"[*] Cookies    : {args.cookies}")
    print(f"[*] Çıktı      : {args.output}")
    print()

    print("[*] Şifreleme anahtarı çekiliyor...")
    key = get_encryption_key(args.local_state)
    print(f"[+] Anahtar   : {key.hex()} ({len(key)} byte)")
    print()

    print("[*] Cookieler çözülüyor ve yazılıyor...")
    count = process_cookies(args.cookies, args.output, key)

    print(f"\n[+] Tamamlandı! {count} cookie → {args.output}")
    print(f"    Görüntülemek için: sqlite3 \"{args.output}\" \"SELECT host_key,name,value FROM cookies LIMIT 20;\"")


if __name__ == "__main__":
    main()
