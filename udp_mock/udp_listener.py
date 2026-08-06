#!/usr/bin/env python3
"""本机监听 UDP 广播，用于验证 udp_broadcast.py 是否在发。"""

from __future__ import annotations

import argparse
import json
import socket


def main() -> None:
    parser = argparse.ArgumentParser(description="UDP 广播监听（调试用）")
    parser.add_argument("--host", default="0.0.0.0", help="监听地址")
    parser.add_argument("--port", type=int, default=9999, help="监听端口")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))

    print(f"监听 UDP {args.host}:{args.port}，Ctrl+C 退出")
    try:
        while True:
            data, addr = sock.recvfrom(65535)
            text = data.decode("utf-8", errors="replace")
            try:
                obj = json.loads(text)
                name = obj.get("name", "?")
                run = obj.get("run", "?")
                alarm = obj.get("alarm", "?")
                print(f"{addr[0]} -> {name} run={run} alarm={alarm}")
            except json.JSONDecodeError:
                print(f"{addr[0]} -> {text[:120]}")
    except KeyboardInterrupt:
        print("\n已停止监听")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
