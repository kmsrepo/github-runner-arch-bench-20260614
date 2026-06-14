import argparse
import json
import subprocess

from bench_utils import append_summary, system_info, timed, write_json


def sustained_simd_work(seconds, threads, block_iterations):
    completed = subprocess.run(
        [
            "./build/sustained_simd_asm_bench",
            str(threads),
            str(seconds),
            str(block_iterations),
        ],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return json.loads(completed.stdout)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=60.0)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--block-iterations", type=int, default=1_000_000)
    parser.add_argument("--output", default="results/sustained-simd-asm.json")
    args = parser.parse_args()

    payload = {
        "benchmark": "sustained_simd_asm",
        "target_seconds": args.seconds,
        "threads": args.threads,
        "block_iterations": args.block_iterations,
        "system": system_info(),
        "measurements": [
            timed(
                "sustained_arch_optimized_simd_asm",
                lambda: sustained_simd_work(
                    args.seconds, args.threads, args.block_iterations
                ),
            ),
        ],
    }
    write_json(args.output, payload)

    result = payload["measurements"][0]["result"]
    append_summary(
        f"## Sustained SIMD Assembly Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- Kernel: `{result['kernel']}`\n"
        f"- Supported: `{result['supported']}`\n"
        f"- Threads: `{result['threads']}`\n"
        f"- Target seconds: `{result['target_seconds']:.3f}`\n"
        f"- Elapsed seconds: `{result['elapsed_seconds']:.3f}`\n"
        f"- Lanes: `{result['lanes']}`\n"
        f"- Lane updates: `{result['lane_updates']:.0f}`\n"
        f"- Lane updates/sec: `{result['lane_updates_per_second']:.3f}`\n"
        f"- Checksum: `{result['checksum']}`\n"
    )


if __name__ == "__main__":
    main()
