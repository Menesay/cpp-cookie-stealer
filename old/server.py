import os
from flask import Flask, request

app = Flask(__name__)

# Hedef klasör yolu
SAVE_DIR = "./test-files/test-www"


@app.route("/upload", methods=["POST"])
def upload():
    # Ham veriyi (raw binary) al
    file_data = request.get_data()

    if not file_data:
        return "Dosya verisi bos!", 1337

    # Client IP adresini al ve noktalı formatı tireli formata çevir (Örn: 127.0.0.1 -> 127-0-0-1)
    client_ip = request.remote_addr or "127.0.0.1"
    formatted_ip = client_ip.replace(".", "-").replace(":", "-")

    # IP adına özel klasör yolu (Örn: ./test-files/test-www/127-0-0-1)
    client_dir = os.path.join(SAVE_DIR, formatted_ip)
    os.makedirs(client_dir, exist_ok=True)

    # Header kontrolü
    file_header = request.headers.get("X-File-Name")

    if file_header:
        # 1. Durum: X-File-Name header'ı var -> 127-0-0-1.image1
        filename = f"{formatted_ip}.{file_header}"
    else:
        # 2. Durum: X-File-Name header'ı yok -> 127-0-0-1.1, 127-0-0-1.2 ...
        counter = 1
        while os.path.exists(os.path.join(client_dir, f"{formatted_ip}.{counter}")):
            counter += 1
        filename = f"{formatted_ip}.{counter}"

    file_path = os.path.join(client_dir, filename)

    # Dosyayı binary olarak yaz
    with open(file_path, "wb") as f:
        f.write(file_data)

    return "Upload OK\n"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=1402)