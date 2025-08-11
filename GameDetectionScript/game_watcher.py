import time, os, sys, platform
import serial, serial.tools.list_ports
import psutil

IS_WINDOWS = platform.system() == "Windows"

def find_arduino_port(preferred=None, baud=115200, timeout=1.0):
    if preferred:
        try:
            return serial.Serial(preferred, baudrate=baud, timeout=timeout)
        except Exception:
            pass
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        manu = (p.manufacturer or "").lower()
        if "arduino" in desc or "arduino" in manu or "ch340" in desc or "wch" in manu or "usb serial" in desc:
            try:
                return serial.Serial(p.device, baudrate=baud, timeout=timeout)
            except Exception:
                continue
    # fallback: first tty/COM available
    ports = [p.device for p in serial.tools.list_ports.comports()]
    return serial.Serial(ports[0], baudrate=baud, timeout=timeout) if ports else None

def basename_no_ext(path):
    base = os.path.basename(path)
    root, ext = os.path.splitext(base)
    return root

# ---- Foreground process detection (best on Windows) ----
if IS_WINDOWS:
    import win32gui, win32process

    def get_foreground_exe_name():
        hwnd = win32gui.GetForegroundWindow()
        if not hwnd:
            return None
        try:
            _, pid = win32process.GetWindowThreadProcessId(hwnd)
            p = psutil.Process(pid)
            return basename_no_ext(p.exe())
        except Exception:
            return None
else:
    # Cross‑platform fallback: pick the newest active GUI-ish process (not perfect)
    def get_foreground_exe_name():
        # Adjust this to your known game EXE names if you want stronger matching
        candidates = []
        for p in psutil.process_iter(attrs=["name", "exe", "create_time"]):
            try:
                name = p.info.get("name") or ""
                exe = p.info.get("exe") or ""
                if name.endswith(".exe") or exe.endswith(".exe") or "Unity" in name or "Unreal" in name:
                    candidates.append((p.info.get("create_time") or 0, basename_no_ext(exe or name)))
            except Exception:
                continue
        if not candidates:
            return None
        # most recently started
        return sorted(candidates, key=lambda t: t[0], reverse=True)[0][1]

def main():
    com_hint = sys.argv[1] if len(sys.argv) > 1 else None
    ser = find_arduino_port(com_hint)
    if not ser:
        print("I don’t know much about that: no serial port found. Pass COM port as arg, e.g. `python game_watcher.py COM5`.")
        return

    # Give the Arduino a moment to reset after opening the port
    time.sleep(2.0)

    last_name = None
    heartbeat_t = 0.0
    print(f"Connected on {ser.port}. Watching foreground process…")
    while True:
        try:
            name = get_foreground_exe_name()
            # Normalize: Title Case, strip dots
            if name:
                norm = name.replace(".", " ").strip()
                norm = norm[:20]  # keep it short for small displays
            else:
                norm = ""

            now = time.time()
            changed = (norm and norm != last_name)

            # Send on change, and also heartbeat every 10s with current name (if any)
            if changed or (now - heartbeat_t > 10.0 and norm):
                msg = f"APP:{norm}\n"
                ser.write(msg.encode("utf-8"))
                last_name = norm
                heartbeat_t = now
                print("→", msg.strip())

            time.sleep(0.2)
        except serial.SerialException:
            print("Serial lost. Reconnecting…")
            time.sleep(1.0)
            ser = find_arduino_port(com_hint)
            if ser:
                time.sleep(2.0)
        except KeyboardInterrupt:
            break
        except Exception as e:
            # Don’t crash on odd focus or permission errors
            time.sleep(0.2)

if __name__ == "__main__":
    main()
