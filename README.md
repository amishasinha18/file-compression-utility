# 🗜️ File Compression Utility

[![Live Demo](https://img.shields.io/badge/demo-online-brightgreen?style=flat-square)](https://file-compression-utility.onrender.com)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)](huffman.cpp)
[![Python](https://img.shields.io/badge/Python-3.8%2B-3776AB?style=flat-square&logo=python&logoColor=white)](app.py)
[![Flask](https://img.shields.io/badge/Flask-web%20server-000000?style=flat-square&logo=flask&logoColor=white)](app.py)

A full-stack file compression tool built around a Huffman coding engine written from scratch in C++, wrapped in a modern Flask web application.

### 🚀 [Live Demo → file-compression-utility.onrender.com](https://file-compression-utility.onrender.com)

> The free Render tier spins down after periods of inactivity, so the first request after a while may take 30–60 seconds to wake up.

## Overview

This project pairs a native C++ compression core with a browser-based interface, so files can be compressed and decompressed either from the command line or through a polished web UI. The C++ binary does all the actual encoding/decoding work; the Flask layer is a thin, secure wrapper around it (subprocess calls, no shell interpolation) that adds uploads, downloads, and a richer user experience on top.

## 📸 Screenshots

| Compress (light theme) | Result + before/after preview |
|---|---|
| ![Compress view](screenshots/compress-light.png) | ![Result preview](screenshots/result-preview.png) |

| Dark theme + batch upload | Session history & analytics drawer |
|---|---|
| ![Dark theme batch](screenshots/batch-dark.png) | ![History drawer](screenshots/history-drawer.png) |

## 🏗️ Architecture — How It Works

Three layers talk to each other on every compress/decompress request: the browser UI, the Flask backend, and the compiled C++ engine.

```
┌──────────────────────────────────────────────┐
│               BROWSER — Web UI               │
│                                              │
│ templates/index.html                         │
│ drag & drop · batch upload                   │
│ history drawer · analytics                   │
│ before / after preview                       │
└──────────────────────────────────────────────┘
                        │  multipart file upload
                        │  (POST /compress or /decompress)
                        ▼
┌──────────────────────────────────────────────┐
│            FLASK BACKEND — app.py            │
│                                              │
│ /compress   /decompress                      │
│ save upload -> uploads/<job-id>/             │
│ subprocess.run([huffman, mode, in, out])     │
│ stream result back + auto cleanup            │
└──────────────────────────────────────────────┘
                        │  argv: mode, in-path, out-path
                        ▼
┌──────────────────────────────────────────────┐
│           C++ ENGINE — huffman.cpp           │
│                                              │
│ build frequency table                        │
│ build Huffman tree                           │
│ encode / decode bits                         │
│ argv: c|d <input> <output>                   │
└──────────────────────────────────────────────┘
                        │  writes output file to job dir
                        ▼
             uploads/<job-id>/output
                        │
                        │  read back by Flask, streamed as
                        │  a download (size headers attached)
                        ▼
             back up to the Browser UI
```

**Request lifecycle:**

1. The browser uploads a file as multipart form data to `/compress` or `/decompress`.
2. Flask saves it into a per-request temp folder (`uploads/<job-id>/`) and shells out to the compiled `huffman` binary via `subprocess.run([...])` — arguments only, never a shell string, so filenames can't be used for command injection.
3. `huffman.cpp` reads the input file, builds a frequency table and Huffman tree (compress) or replays the stored code table (decompress), and writes the result to the job folder.
4. Flask streams the output file straight back as a download, attaching `X-Original-Size` / `X-Result-Size` headers, then deletes the temp folder once the response has been sent.
5. The browser reads the response and headers to update the result card, the before/after preview, the session history drawer, and the analytics — all without a page reload.

## ✨ Features

- 🔤 **C++ Huffman coding engine** — builds a frequency-based Huffman tree and encodes/decodes files at the bit level, exposed as a small standalone CLI binary.
- 🌐 **Flask web interface** — drag-and-drop compress/decompress backed by the compiled engine, with real upload progress and error handling.
- 📦 **Batch file processing** — select and process multiple files in one go, with live per-file progress and a results list.
- 🕒 **Session history** — a slide-out panel tracking every file compressed or decompressed in the current session, with one-click re-download.
- 📊 **Analytics dashboard** — aggregate stats (files processed, total size before/after, space saved) with a live progress meter.
- 🔍 **Before/after preview** — side-by-side view of a file's content pre- and post-processing (readable text when possible, a hex dump for binary data).
- 🎨 **Modern UI** — glassmorphism design with light/dark theme toggle, animated drag & drop, and toast notifications.

## 🛠️ Tech Stack

| Layer | Technology |
|---|---|
| Compression engine | C++ (STL `priority_queue`, `unordered_map`) |
| Backend | Python 3, Flask |
| Frontend | HTML5, CSS3 (glassmorphism/gradients), vanilla JavaScript |
| Deployment | Render |

## 📂 Project Structure

```
file-compression-utility/
├── huffman.cpp          # Core Huffman compression/decompression engine
├── app.py               # Flask backend — wraps the compiled binary
├── templates/
│   └── index.html       # Web UI (compress/decompress, history, previews)
├── requirements.txt     # Python dependencies
├── screenshots/         # Images used in this README
└── uploads/             # Per-request scratch space (auto-cleaned after each request)
```

## ⚙️ Installation / Setup Guide

### Prerequisites

- A C++ compiler with C++17 support (`g++` recommended)
- Python 3.8+
- pip

### Step 1 — Clone the repository

```bash
git clone https://github.com/amishasinha18/file-compression-utility.git
cd file-compression-utility
```

### Step 2 — Compile the C++ engine

```bash
g++ -O2 -std=c++17 -o huffman huffman.cpp
```

This produces a `huffman` executable in the project root, which `app.py` calls directly — it must exist before the web app will start.

### Step 3 — Install Python dependencies

```bash
pip install -r requirements.txt
```

### Step 4 — Run the web app

```bash
python3 app.py
```

Then open **http://localhost:5000** in your browser.

## ▶️ Usage

### Using the web interface

1. Open the app and pick the **Compress** or **Decompress** tab.
2. Drag & drop one or more files onto the dropzone (or click to choose).
3. Click **Compress**/**Decompress** — a progress bar tracks the upload and processing.
4. Review the result: size metrics, compression ratio, and a before/after preview of the file's content.
5. Download the output, or find it again later in the session history drawer (clock icon, top right).

### Using the CLI directly

The compiled binary also works standalone, without the web app:

```bash
./huffman c <input_file> <output_file>   # compress
./huffman d <input_file> <output_file>   # decompress
```

## 📝 Notes

- Huffman coding stores its code table inside the compressed file, so very small or low-diversity inputs can come out *larger* than the original — this is expected overhead, not a bug.
- Files made up of a single repeated byte are a known edge case of the current tree-building logic and may not round-trip correctly.
