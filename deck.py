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
import win32con


# COM-Port anpassen
ser = serial.Serial('COM3', 9600)

last_time = None
last_track = None
last_connection = None
last_clipboard = None
last_check = 0
CHECK_INTERVAL = 5  # Sekunden

media_manager = None  # globales MediaManager-Objekt

firefox = webbrowser.Mozilla("firefox.exe")

def sanitize(text):
    """Entfernt Sonderzeichen"""
    return re.sub(r"[^a-zA-Z0-9 \-:]", "", text or "")

def send_serial_line(prefix: str, text: str):
    """Sichere Ausgabe an Arduino"""
    if text and text.strip():
        #prefix, wie z.B. "TIME:", damit der arduino weiß, für was das signal ist
        line = f"{prefix}:{text.strip()}\n"
        ser.write(line.encode("ascii", errors="ignore"))
        print(f"[SENT] {line.strip()}")  # sendet rückmeldung

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
    """Nur neue Minute senden"""
    global last_time
    now = datetime.now()
    current_time = now.strftime("%H:%M")
    if last_time is None or current_time != last_time:
        send_serial_line("TIME", current_time)
        last_time = current_time

def internet_status():
    """Status prüfen"""
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
    """Nur senden, wenn sich Status ändert"""
    global last_connection, last_check
    if time.time() - last_check < CHECK_INTERVAL:
        return
    last_check = time.time()

    status = internet_status()
    if status != last_connection:
        send_serial_line("CONNECTION", status)
        last_connection = status
#Media-player auslesen, um track-namen zu bekommen
async def init_media_manager():
    """Einmalige Initialisierung des MediaManagers"""
    global media_manager
    if media_manager is None:
        try:
            media_manager = await MediaManager.request_async()
        except Exception as e:
            print("MediaManager init error:", e)
            media_manager = None

async def get_media_info():
    """Aktuellen Track holen"""
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
        media_manager = None  # beim Fehler erneut init versuchen
    return None

async def media_loop():
    """Track regelmäßig senden"""
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
    """Hauptloop"""
    loop = asyncio.get_event_loop()
    asyncio.ensure_future(media_loop())
    asyncio.ensure_future(clipboard_loop())

    # --- Initiale Statuswerte senden ---
    time.sleep(2)
    get_current_time()                # Uhrzeit einmal senden
    send_connection_status()          # Internetstatus einmal senden
    send_serial_line("TRACK", "Nothing Playing...")  # Track einmal senden

    while True:
        # 1. Zeit aktualisieren
        get_current_time()
        # 2. Verbindung prüfen
        send_connection_status()

        # 3. Befehle vom Arduino empfangen
        if ser.in_waiting:
            line = ser.readline().decode(errors="ignore").strip()
            print(f"[RECV] {line}")  # rückmeldung an konsole
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
        loop.run_until_complete(asyncio.sleep(0.1))  # kleiner Delay


if __name__ == "__main__":
    main_loop()
