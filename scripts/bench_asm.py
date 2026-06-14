import argparse
import json
import subprocess

from bench_utils import append_summary, system_info, timed, write_json


def asm_work(repetitions):
    iterations = str(120_000_000 * repetitions)
    completed = subprocess.run(
        ["./build/asm_bench", "4", iterations],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return json.loads(completed.stdout)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--output", default="results/asm.json")
    args = parser.parse_args()

    payload = {
        "benchmark": "asm",
        "repetitions": args.repetitions,
        "system": system_info(),
        "measurements": [
            timed("architecture_optimized_simd_asm", lambda: asm_work(args.repetitions)),
        ],
    }
    write_json(args.output, payload)

    result = payload["measurements"][0]["result"]
    append_summary(
        f"## Assembly SIMD Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- Kernel: `{result['kernel']}`\n"
        f"- Threads: `{result['threads']}`\n"
        f"- Lane updates: `{result['lane_updates']:.0f}`\n"
        f"- Harness seconds: `{payload['measurements'][0]['seconds']:.3f}s`\n"
        f"- Kernel seconds: `{result['seconds']:.3f}s`\n"
        f"- Lane updates/sec: `{result['lane_updates_per_second']:.3f}`\n"
    )


if __name__ == "__main__":
    main()
