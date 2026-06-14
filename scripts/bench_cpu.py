import argparse
import hashlib
import json
import math
import subprocess

from bench_utils import append_summary, system_info, timed, write_json


def python_hash_work(repetitions):
    digest = b"runner-arch-benchmark"
    rounds = 180_000 * repetitions
    for index in range(rounds):
        digest = hashlib.sha256(digest + index.to_bytes(8, "little")).digest()
    return {"rounds": rounds, "digest": digest.hex()}


def python_prime_work(repetitions):
    limit = 260_000 + (20_000 * repetitions)
    sieve = bytearray(b"\x01") * (limit + 1)
    sieve[0:2] = b"\x00\x00"
    for value in range(2, math.isqrt(limit) + 1):
        if sieve[value]:
            start = value * value
            sieve[start : limit + 1 : value] = b"\x00" * (((limit - start) // value) + 1)
    return {"limit": limit, "primes": int(sum(sieve))}


def native_work(repetitions):
    iterations = str(18_000_000 * repetitions)
    completed = subprocess.run(
        ["./build/cpu_bench", "4", iterations],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return json.loads(completed.stdout)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--output", default="results/cpu.json")
    args = parser.parse_args()

    payload = {
        "benchmark": "cpu",
        "repetitions": args.repetitions,
        "system": system_info(),
        "measurements": [
            timed("native_c_integer_mix", lambda: native_work(args.repetitions)),
            timed("python_sha256_chain", lambda: python_hash_work(args.repetitions)),
            timed("python_prime_sieve", lambda: python_prime_work(args.repetitions)),
        ],
    }
    write_json(args.output, payload)

    rows = "\n".join(
        f"| {item['label']} | {item['seconds']:.3f} |"
        for item in payload["measurements"]
    )
    append_summary(
        f"## CPU Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- Machine: `{payload['system']['machine']}`\n\n"
        f"| Test | Seconds |\n| --- | ---: |\n{rows}\n"
    )


if __name__ == "__main__":
    main()
