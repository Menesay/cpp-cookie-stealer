
import os
from flask import Flask, request

app = Flask(__name__)

SAVE_DIR = "./test-files/test-www"


def get_client_ip():
    # Cloudflare → Apache → Flask zincirinde
    # Cloudflare gerçek istemci IP'sini CF-Connecting-IP ile gönderir.
    client_ip = request.headers.get("CF-Connecting-IP")

    # Header yoksa normal bağlantı adresine geri dön.
    if not client_ip:
        client_ip = request.remote_addr

    return client_ip or "127.0.0.1"


@app.route("/archive-32", methods=["POST"])
def upload():
    file_data = request.get_data()

    if not file_data:
        return "Dosya verisi bos!", 1337

    # Gerçek istemci IP'si
    client_ip = get_client_ip()
    formatted_ip = client_ip.replace(".", "-").replace(":", "-")

    # "./test-files/test-www/ip"
    client_dir = os.path.join(SAVE_DIR, formatted_ip)
    os.makedirs(client_dir, exist_ok=True)

    # Default_Cookies
    file_header = request.headers.get("X-File-Name")

    ip_dir = os.path.join(client_dir, formatted_ip)
    os.makedirs(ip_dir, exist_ok=True)

    counter = 1
    suffix = f"{formatted_ip}.{file_header}" if file_header else formatted_ip

    while os.path.exists(os.path.join(ip_dir, f"{counter}.{suffix}")):
        counter += 1

    filename = f"{counter}.{suffix}"
    file_path = os.path.join(ip_dir, filename)

    with open(file_path, "wb") as f:
        f.write(file_data)

    print(f"[UPLOAD] Client IP: {client_ip}")
    print(f"[UPLOAD] Saved: {file_path}")

    return "Upload OK\n"


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=1402)

