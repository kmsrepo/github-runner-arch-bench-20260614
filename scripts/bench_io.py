import argparse
import os
import random
import tempfile

from bench_utils import append_summary, system_info, timed, write_json


BLOCK = 1024 * 1024


def write_file(path, size_mb):
    pattern = bytes((index * 17) % 256 for index in range(BLOCK))
    with open(path, "wb", buffering=BLOCK) as handle:
        for _ in range(size_mb):
            handle.write(pattern)
        handle.flush()
        os.fsync(handle.fileno())
    return {"bytes": size_mb * BLOCK}


def read_file(path):
    total = 0
    checksum = 0
    with open(path, "rb", buffering=BLOCK) as handle:
        while True:
            chunk = handle.read(BLOCK)
            if not chunk:
                break
            total += len(chunk)
            checksum ^= chunk[0]
            checksum ^= chunk[-1]
    return {"bytes": total, "checksum": checksum}


def random_read(path, samples):
    size = os.path.getsize(path)
    checksum = 0
    random.seed(42)
    with open(path, "rb", buffering=0) as handle:
        for _ in range(samples):
            offset = random.randrange(0, max(1, size - 4096))
            handle.seek(offset)
            chunk = handle.read(4096)
            checksum ^= chunk[0]
            checksum ^= chunk[-1]
    return {"samples": samples, "checksum": checksum}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--size-mb", type=int, default=256)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--output", default="results/io.json")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmpdir:
        path = os.path.join(tmpdir, "io-bench.bin")
        measurements = []
        for repetition in range(args.repetitions):
            measurements.append(timed(f"write_{repetition + 1}", lambda: write_file(path, args.size_mb)))
            measurements.append(timed(f"read_{repetition + 1}", lambda: read_file(path)))
            measurements.append(timed(f"random_read_{repetition + 1}", lambda: random_read(path, 16_384)))

    payload = {
        "benchmark": "io",
        "size_mb": args.size_mb,
        "repetitions": args.repetitions,
        "system": system_info(),
        "measurements": measurements,
    }
    write_json(args.output, payload)

    rows = "\n".join(
        f"| {item['label']} | {item['seconds']:.3f} |"
        for item in measurements
    )
    append_summary(
        f"## I/O Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- File size: `{args.size_mb} MB`\n\n"
        f"| Test | Seconds |\n| --- | ---: |\n{rows}\n"
    )


if __name__ == "__main__":
    main()
