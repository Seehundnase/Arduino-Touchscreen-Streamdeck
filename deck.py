import serial
import time
import re
import keyboard
import asyncio
from datetime import datetime
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
import socket
import psutil
import subprocess
import webbrowser
import win32clipboard

def choose_serial_port():
    default_port = "COM3"
    port = input(f"Arduino port:").strip()
    if not port:
        port = default_port
    try:
        ser = serial.Serial(port, 9600, timeout=2)
        print(f"Connected to Arduino on {port}")
        return ser
    except serial.SerialException:
        print(f"Could not open {port}.c")
        exit(1)

# adjust com port
ser = choose_serial_port()

last_time = None
last_track = None
last_connection = None
last_clipboard = None
last_check = 0
CHECK_INTERVAL = 5  # seconds

media_manager = None  # global MediaManager-Object

firefox = webbrowser.Mozilla("firefox.exe")

def sanitize(text):
    """removes special characters from text"""
    return re.sub(r"[^a-zA-Z0-9 \-:]", "", text or "")

def send_serial_line(prefix: str, text: str):
    """secure output to Arduino"""
    if text and text.strip():
        #prefix, e.g. "TIME:", so Arduino can identify the signal
        line = f"{prefix}:{text.strip()}\n"
        ser.write(line.encode("ascii", errors="ignore"))
        print(f"[SENT] {line.strip()}")  # sending feedback

def get_clipboard_text():
    try:
        win32clipboard.OpenClipboard()
        if win32clipboard.IsClipboardFormatAvailable(win32clipboard.CF_UNICODETEXT):
            data = win32clipboard.GetClipboardData(win32clipboard.CF_UNICODETEXT)
        else:
            data = None
        win32clipboard.CloseClipboard()
        return data
    except Exception:
        return None

def get_current_time():
    """only send new time"""
    global last_time
    now = datetime.now()
    current_time = now.strftime("%H:%M")
    if last_time is None or current_time != last_time:
        send_serial_line("TIME", current_time)
        last_time = current_time

def internet_status():
    """check status"""
    try:
        socket.create_connection(("8.8.8.8", 53), timeout=2)
        online = True
    except OSError:
        online = False

    adapters = psutil.net_if_addrs()
    active_adapters = [name for name, addrs in adapters.items() if addrs]

    if not online:
        return "No connection"
    elif any("Wi-Fi" in a or "WLAN" in a for a in active_adapters):
        return "Wi-Fi"
    elif any("Ethernet" in a for a in active_adapters):
        return "Ethernet"
    else:
        return "Connected (unknown)"

def send_connection_status():
    """only send, if status changes"""
    global last_connection, last_check
    if time.time() - last_check < CHECK_INTERVAL:
        return
    last_check = time.time()

    status = internet_status()
    if status != last_connection:
        send_serial_line("CONNECTION", status)
        last_connection = status
#read media manager to get track name
async def init_media_manager():
    """initialize Windows media manager"""
    global media_manager
    if media_manager is None:
        try:
            media_manager = await MediaManager.request_async()
        except Exception as e:
            print("MediaManager init error:", e)
            media_manager = None

async def get_media_info():
    """get current track"""
    global media_manager
    if media_manager is None:
        await init_media_manager()
        if media_manager is None:
            return None
    try:
        current_session = media_manager.get_current_session()
        if current_session:
            info = await current_session.try_get_media_properties_async()
            title = sanitize(info.title or "Unknown")
            artist = sanitize(info.artist or "Unknown")
            return f"{title} - {artist}"
    except Exception as e:
        print("Media error:", e)
        media_manager = None
    return None

async def media_loop():
    """continue sending track"""
    global last_track
    await init_media_manager()
    while True:
        track = await get_media_info()
        if track and track != last_track:
            send_serial_line("TRACK", track)
            last_track = track
        await asyncio.sleep(2)
        
async def clipboard_loop():
    global last_clipboard
    while True:
        text = get_clipboard_text()
        if text and text != last_clipboard:
            clean = sanitize(text)
            if clean:
                send_serial_line("CLIP", clean[:200])  # limit length (Arduino safety)
                last_clipboard = text
        await asyncio.sleep(0.4)  # non-blocking

def main_loop():
    """main loop"""
    loop = asyncio.get_event_loop()
    asyncio.ensure_future(media_loop())
    asyncio.ensure_future(clipboard_loop())

    # --- Send status initially ---
    time.sleep(2)
    get_current_time()
    send_connection_status()
    send_serial_line("TRACK", "Nothing Playing...")

    while True:
        # 1. time
        get_current_time()
        # 2. connection
        send_connection_status()

        # 3. receive signals
        if ser.in_waiting:
            line = ser.readline().decode(errors="ignore").strip()
            print(f"[RECV] {line}")  # feedback
            if line == "PLAY":
                keyboard.send("play/pause media")
            elif line == "SKIP":
                keyboard.send("next track")
            elif line == "PREVIOUS":
                keyboard.send("previous track")
            elif line == "VOLUP":
                keyboard.send("volume up")
            elif line == "VOLDOWN":
                keyboard.send("volume down")
            elif line == "MUTE":
                keyboard.send("volume mute")
            elif line == "CALC":
                subprocess.Popen(["calc.exe"])
            elif line == "BROWSER":
                subprocess.Popen(["firefox.exe"])
            elif line == "EXPLORER":
                subprocess.Popen(["explorer.exe"])
            elif line == "DEEZER":
                appid = "Deezer.62021768415AF_q7m17pa7q8kj0!Deezer.Music"
                subprocess.Popen(["explorer.exe", f"shell:AppsFolder\\{appid}"])
            elif line == "YOUTUBE":
                firefox.open("youtube.com")
        loop.run_until_complete(asyncio.sleep(0.1))  # small delay


if __name__ == "__main__":
    main_loop()
