#!/usr/bin/env python3
"""Read-only DSS1/BANK.DSS catalog and PCM8 WAV diagnostics under build/audio-debug/."""

import argparse
import csv
import hashlib
import struct
import wave
from pathlib import Path

DEBUG_ROOT = Path(__file__).resolve().parents[1] / "build/audio-debug"


def sample_at(data, start=0):
    if start < 0 or len(data) - start < 44 or data[start:start+4] != b"DSS1":
        raise ValueError("Invalid/truncated DSS1 header")
    rate, count, flag, begin, end = struct.unpack_from("<HIHII", data, start+28)
    if rate != 5000 or not count or count > len(data)-start-44:
        raise ValueError("Unsupported sample rate or truncated PCM")
    if flag and not 0 <= begin < end <= count:
        raise ValueError("Invalid loop bounds")
    name = data[start+4:start+28].decode("ascii", errors="replace").rstrip(" \0")
    return {"name": name, "rate": rate, "count": count, "loop_flag": flag,
            "loop_begin": begin, "loop_end": end,
            "pcm": data[start+44:start+44+count], "offset": start}, start+44+count


def decode_bank(data):
    if len(data) < 2:
        raise ValueError("Truncated bank count")
    count, = struct.unpack_from("<H", data)
    if not count or count > len(data)//47:
        raise ValueError("Invalid bank count")
    records, p = [], 2
    for index in range(count):
        if p+3 > len(data):
            raise ValueError("Truncated bank metadata")
        volume, continuous, priority = data[p:p+3]
        if volume > 64 or continuous not in (0, 255):
            raise ValueError("Unvalidated bank metadata")
        record, p = sample_at(data, p+3)
        record.update(id=index, volume=volume, continuous=continuous, priority=priority)
        records.append(record)
    if p != len(data):
        raise ValueError("Bank count/length mismatch")
    return records


def extract(directory, output):
    output = output.resolve()
    if output != DEBUG_ROOT and DEBUG_ROOT not in output.parents:
        raise ValueError("Output must be under build/audio-debug/")
    records = []
    for path in sorted(directory.glob("*.DSS")):
        data = path.read_bytes()
        if path.name.upper() == "BANK.DSS":
            decoded = decode_bank(data)
        else:
            record, end = sample_at(data)
            if end != len(data):
                raise ValueError(f"Trailing DSS data in {path}")
            record.update(id=path.stem, volume="", continuous="", priority="")
            decoded = [record]
        for record in decoded:
            record["source"] = path.name
            records.append(record)
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    for record in records:
        pcm = record["pcm"]
        name = f"{Path(record['source']).stem}-{record['id']}.wav"
        with wave.open(str(output / name), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(1)
            wav.setframerate(record["rate"])
            wav.writeframes(pcm)
        rows.append({k: v for k, v in record.items() if k != "pcm"} | {
            "format": "PCM unsigned 8-bit mono", "seconds": len(pcm)/record["rate"],
            "pcm_sha256": hashlib.sha256(pcm).hexdigest(), "wav": name})
    if rows:
        with (output / "catalog.csv").open("w", newline="") as file:
            writer = csv.DictWriter(file, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    print(f"{len(rows)} records decoded; {len(set(r['pcm_sha256'] for r in rows))} unique PCM payloads")
    print(f"Catalog and WAVs: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=DEBUG_ROOT)
    args = parser.parse_args()
    extract(args.data_dir, args.output)
