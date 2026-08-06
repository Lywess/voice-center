#!/usr/bin/env python3
"""循环将 CSV 设备数据以 UDP 广播发出，默认端口 9999。"""

from __future__ import annotations

import argparse
import csv
import json
import socket
import time
from pathlib import Path


def load_rows(csv_path: Path) -> list[dict[str, str]]:
    with csv_path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def broadcast_loop(
    rows: list[dict[str, str]],
    host: str,
    port: int,
    interval: float,
) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    total = len(rows)
    print(f"UDP 广播启动 -> {host}:{port}，共 {total} 条，间隔 {interval}s，Ctrl+C 停止")

    index = 0
    try:
        while True:
            row = rows[index]
            payload = json.dumps(row, ensure_ascii=False).encode("utf-8")
            sock.sendto(payload, (host, port))
            name = row.get("name", "")
            print(f"[{index + 1}/{total}] {name} ({len(payload)} bytes)")
            index = (index + 1) % total
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n已停止广播")
    finally:
        sock.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="CSV 设备数据 UDP 循环广播")
    parser.add_argument(
        "--csv",
        default=str(Path(__file__).with_name("yu_device_data.csv")),
        help="CSV 文件路径",
    )
    parser.add_argument("--host", default="255.255.255.255", help="广播地址")
    parser.add_argument("--port", type=int, default=9999, help="目标端口")
    parser.add_argument("--interval", type=float, default=1.0, help="每条间隔秒数")
    args = parser.parse_args()

    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise SystemExit(f"找不到 CSV 文件: {csv_path}")

    rows = load_rows(csv_path)
    if not rows:
        raise SystemExit("CSV 没有数据行")

    broadcast_loop(rows, args.host, args.port, args.interval)


if __name__ == "__main__":
    main()
