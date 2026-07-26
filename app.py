import os
import shutil
import subprocess
import uuid

from flask import Flask, after_this_request, render_template, request, send_file, jsonify
from werkzeug.utils import secure_filename

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
HUFFMAN_BIN = os.path.join(BASE_DIR, "huffman")
WORK_DIR = os.path.join(BASE_DIR, "uploads")
MAX_CONTENT_LENGTH = 50 * 1024 * 1024  # 50 MB

os.makedirs(WORK_DIR, exist_ok=True)

app = Flask(__name__)
app.config["MAX_CONTENT_LENGTH"] = MAX_CONTENT_LENGTH


def run_huffman(mode, input_path, output_path):
    result = subprocess.run(
        [HUFFMAN_BIN, mode, input_path, output_path],
        capture_output=True,
        text=True,
        timeout=60,
    )
    return result.returncode == 0 and os.path.exists(output_path), (result.stdout + result.stderr).strip()


def process(mode, out_name_fn):
    if "file" not in request.files or request.files["file"].filename == "":
        return jsonify(error="No file selected."), 400

    upload = request.files["file"]
    original_name = secure_filename(upload.filename) or "file"

    job_dir = os.path.join(WORK_DIR, uuid.uuid4().hex)
    os.makedirs(job_dir, exist_ok=True)

    @after_this_request
    def cleanup(response):
        # Safe on Linux even for the file still being streamed: unlinking a path
        # doesn't invalidate a file descriptor that's already open for reading.
        shutil.rmtree(job_dir, ignore_errors=True)
        return response

    download_name = out_name_fn(original_name)
    input_path = os.path.join(job_dir, original_name)
    output_path = os.path.join(job_dir, download_name)

    upload.save(input_path)

    if os.path.getsize(input_path) == 0:
        return jsonify(error="Uploaded file is empty."), 400

    ok, log = run_huffman(mode, input_path, output_path)
    if not ok:
        return jsonify(error="Processing failed.", detail=log or "unknown error"), 500

    original_size = os.path.getsize(input_path)
    result_size = os.path.getsize(output_path)

    response = send_file(output_path, as_attachment=True, download_name=download_name)
    response.headers["X-Original-Size"] = str(original_size)
    response.headers["X-Result-Size"] = str(result_size)
    response.headers["Access-Control-Expose-Headers"] = "X-Original-Size, X-Result-Size"
    return response


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/compress", methods=["POST"])
def compress():
    return process("c", lambda name: name + ".huff")


@app.route("/decompress", methods=["POST"])
def decompress():
    def out_name(name):
        return name[:-5] + ".decompressed" if name.endswith(".huff") else name + ".decompressed"

    return process("d", out_name)


if __name__ == "__main__":
    if not os.path.exists(HUFFMAN_BIN):
        raise SystemExit(
            "huffman executable not found next to app.py. "
            "Build it first: g++ -O2 -std=c++17 -o huffman huffman.cpp"
        )
    app.run(host="0.0.0.0", port=5000, debug=False)
