#!/usr/bin/env python3

# Stream Video from Raspberry Pi Camera to Local Computer

# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

import io
import threading
import picamera2
from flask import Flask, Response

# ---------------------------------------------------------------------------
# Main functions
# ---------------------------------------------------------------------------

app = Flask(__name__)

# Singleton for the camera - created only once
camera = None
camera_lock = threading.Lock()
frame_lock = threading.Lock()
current_frame = None


def initialize_camera():
    global camera
    if camera is None:
        with camera_lock:
            if camera is None:  # Double-check locking
                camera = picamera2.Picamera2()
                camera.configure(
                    camera.create_preview_configuration(
                        main={"format": "XRGB8888", "size": (4056, 3040)}
                    )
                )
                camera.start()


def capture_frame():
    """Capture a frame and cache it"""
    global current_frame
    stream = io.BytesIO()
    try:
        camera.capture_file(stream, format="jpeg")
        stream.seek(0)
        with frame_lock:
            current_frame = stream.read()
    except Exception as e:
        print(f"Error during capture: {e}")


def generate_frames():
    """Generate frames for the stream"""
    initialize_camera()

    # Continuously capture frames in the background
    import threading

    def capture_loop():
        while True:
            capture_frame()
            threading.Event().wait(0.04)  # ~25 FPS

    # Start the capture thread
    capture_thread = threading.Thread(target=capture_loop, daemon=True)
    capture_thread.start()

    # Send frames to clients
    while True:
        if current_frame:
            with frame_lock:
                frame_data = current_frame
            yield b"--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + str(
                len(frame_data)
            ).encode() + b"\r\n\r\n" + frame_data + b"\r\n"
        threading.Event().wait(0.01)


@app.route("/video_feed")
def video_feed():
    return Response(
        generate_frames(), mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/")
def index():
    return """
    <html>
    <head><title>Pi Camera Stream</title></head>
    <body>
        <h1>Raspberry Pi Camera Stream</h1>
        <img src="/video_feed" style="max-width: 100%; border: 1px solid #ccc;">
    </body>
    </html>
    """


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    initialize_camera()
    app.run(host="0.0.0.0", port=5000, threaded=True)
