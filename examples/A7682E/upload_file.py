#!/usr/bin/env python3
"""Upload a local file to the A7682E C:/ file system over an AT bridge."""

# 关闭串口监视器、连接电池后执行：
# cd D:\dgx\code\0_lilygo\T_Deck_Pro_V1.1
# python .\examples\A7682E\upload_file.py COM57 "D:\dgx\source\music\sd_2.mp3"
# python .\examples\A7682E\upload_file.py COM57 "D:\dgx\source\music\sd_2.mp3" --remote-name music.mp3

# 需要安装依赖时：
# python -m pip install pyserial

import argparse
import re
import sys
import time
from pathlib import Path


DEFAULT_BAUD = 115200
CHUNK_SIZE = 256
CHUNK_DELAY_SECONDS = 0.05
MODEM_READY_TIMEOUT_SECONDS = 20
INVALID_REMOTE_NAME_CHARS = set('\\/:*?"<>|')


class UploadError(RuntimeError):
    pass


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Upload a file through the T-Deck Pro serial bridge to the "
            "A7682E internal C:/ file system."
        )
    )
    parser.add_argument("port", help="serial port connected to T-Deck Pro, for example COM57")
    parser.add_argument("file", type=Path, help="local file to upload")
    parser.add_argument(
        "--remote-name",
        metavar="NAME",
        help="name on A7682E; defaults to the local file name",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help="serial baud rate (default: %(default)s)",
    )
    return parser.parse_args()


def validate_source(path):
    path = path.expanduser().resolve()
    if not path.is_file():
        raise UploadError(f"Local file does not exist: {path}")

    size = path.stat().st_size
    if size <= 0:
        raise UploadError(f"Local file is empty: {path}")
    return path, size


def validate_remote_name(name):
    if not name:
        raise UploadError("Remote file name cannot be empty")
    if name[0] in {".", " "} or name.endswith("."):
        raise UploadError("Remote file name cannot start with '.'/space or end with '.'")
    if any(char in INVALID_REMOTE_NAME_CHARS for char in name):
        raise UploadError("Remote file name contains a character not supported by A7682E")

    try:
        remote_path = f"C:/{name}"
        encoded_path = remote_path.encode("ascii")
    except UnicodeEncodeError as exc:
        raise UploadError("The A7682E C:/ file name must contain ASCII characters only") from exc

    if len(encoded_path) > 115:
        raise UploadError("The complete A7682E C:/ path cannot exceed 115 bytes")
    return remote_path


def readable_response(response):
    return response.decode("ascii", errors="replace").strip() or "<no response>"


def response_has_line(response, expected):
    return any(line.strip() == expected for line in response.splitlines())


def read_command_response(serial_port, timeout, wait_for_prompt=False):
    response = bytearray()
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        chunk = serial_port.read(serial_port.in_waiting or 1)
        if not chunk:
            continue
        response.extend(chunk)
        if wait_for_prompt and b">" in response:
            break
        if response_has_line(response, b"OK") or response_has_line(response, b"ERROR"):
            break

    return bytes(response)


def send_command(serial_port, command, timeout=5, allow_error=False):
    serial_port.reset_input_buffer()
    serial_port.write((command + "\r\n").encode("ascii"))
    serial_port.flush()

    response = read_command_response(serial_port, timeout)
    has_error = response_has_line(response, b"ERROR")
    has_ok = response_has_line(response, b"OK")
    if has_error and not allow_error:
        raise UploadError(f"Command failed: {command}\n{readable_response(response)}")
    if not has_ok and not has_error:
        raise UploadError(f"Command timed out: {command}\n{readable_response(response)}")
    return response


def wait_for_modem(serial_port):
    deadline = time.monotonic() + MODEM_READY_TIMEOUT_SECONDS
    while time.monotonic() < deadline:
        serial_port.reset_input_buffer()
        serial_port.write(b"AT\r\n")
        serial_port.flush()
        response = read_command_response(serial_port, 1)
        if response_has_line(response, b"OK"):
            return
        time.sleep(0.5)

    raise UploadError(
        "A7682E did not respond. Check the battery, COM port, and test_AT bridge firmware."
    )


def check_free_space(serial_port, file_size):
    response = send_command(serial_port, "AT+FSMEM")
    match = re.search(rb"\+FSMEM:\s*C:\((\d+),\s*(\d+)\)", response)
    if not match:
        return

    total, used = (int(value) for value in match.groups())
    free = total - used
    if free < file_size:
        raise UploadError(
            f"Not enough A7682E storage: need {file_size} bytes, have {free} bytes"
        )


def remove_existing_file(serial_port, remote_name):
    response = send_command(
        serial_port,
        f'AT+FSDEL="{remote_name}"',
        allow_error=True,
    )
    if response_has_line(response, b"OK"):
        print(f"Removed existing C:/{remote_name}")


def send_file_data(serial_port, source, file_size):
    sent = 0
    with source.open("rb") as source_file:
        while True:
            chunk = source_file.read(CHUNK_SIZE)
            if not chunk:
                break
            serial_port.write(chunk)
            serial_port.flush()
            sent += len(chunk)
            percent = sent * 100 // file_size
            print(f"\rUploading: {sent}/{file_size} bytes ({percent:3d}%)", end="", flush=True)
            time.sleep(CHUNK_DELAY_SECONDS)
    print()


def verify_upload(serial_port, remote_name, expected_size):
    response = send_command(serial_port, f'AT+FSATTRI="{remote_name}"')
    match = re.search(rb"\+FSATTRI:\s*(\d+)", response)
    if not match:
        raise UploadError("Upload completed, but A7682E did not report the remote file size")

    actual_size = int(match.group(1))
    if actual_size != expected_size:
        raise UploadError(
            f"Remote size mismatch: expected {expected_size} bytes, got {actual_size} bytes"
        )


def upload(serial_port, source, remote_path, file_size):
    remote_name = remote_path[3:]

    print("Waiting for A7682E...")
    wait_for_modem(serial_port)
    print("A7682E is ready")

    send_command(serial_port, "AT+FSCD=C:")
    check_free_space(serial_port, file_size)
    remove_existing_file(serial_port, remote_name)

    serial_port.reset_input_buffer()
    command = f'AT+CFTRANRX="{remote_path}",{file_size}\r\n'
    serial_port.write(command.encode("ascii"))
    serial_port.flush()

    response = read_command_response(serial_port, 10, wait_for_prompt=True)
    if b">" not in response:
        raise UploadError(
            "A7682E did not enter file receive mode:\n" + readable_response(response)
        )

    send_file_data(serial_port, source, file_size)

    response = read_command_response(serial_port, 30)
    if response_has_line(response, b"ERROR"):
        raise UploadError("A7682E rejected the file data:\n" + readable_response(response))
    if not response_has_line(response, b"OK"):
        raise UploadError("Timed out waiting for upload confirmation")

    verify_upload(serial_port, remote_name, file_size)


def open_serial(serial_module, port, baud):
    serial_port = serial_module.Serial()
    serial_port.port = port
    serial_port.baudrate = baud
    serial_port.timeout = 0.1
    serial_port.write_timeout = 10
    serial_port.dtr = False
    serial_port.rts = False
    serial_port.open()
    return serial_port


def main():
    args = parse_args()

    try:
        import serial
    except ImportError:
        print("pyserial is required. Install it with: python -m pip install pyserial", file=sys.stderr)
        return 2

    try:
        source, file_size = validate_source(args.file)
        remote_name = args.remote_name or source.name
        remote_path = validate_remote_name(remote_name)

        print(f"Local file : {source}")
        print(f"Remote file: {remote_path}")
        print(f"File size  : {file_size} bytes")

        with open_serial(serial, args.port, args.baud) as serial_port:
            upload(serial_port, source, remote_path, file_size)

        print(f"Upload complete: {remote_path} ({file_size} bytes)")
        return 0
    except KeyboardInterrupt:
        print("\nUpload cancelled", file=sys.stderr)
        return 130
    except (UploadError, OSError, serial.SerialException) as exc:
        print(f"Upload failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
