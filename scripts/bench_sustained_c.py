import argparse
import json
import subprocess

from bench_utils import append_summary, system_info, timed, write_json


def sustained_work(seconds, threads):
    completed = subprocess.run(
        ["./build/sustained_c_bench", str(threads), str(seconds)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return json.loads(completed.stdout)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=60.0)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--output", default="results/sustained-c.json")
    args = parser.parse_args()

    payload = {
        "benchmark": "sustained_c",
        "target_seconds": args.seconds,
        "threads": args.threads,
        "system": system_info(),
        "measurements": [
            timed(
                "sustained_mixed_integer_float_c",
                lambda: sustained_work(args.seconds, args.threads),
            ),
        ],
    }
    write_json(args.output, payload)

    result = payload["measurements"][0]["result"]
    append_summary(
        f"## Sustained C Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- Threads: `{result['threads']}`\n"
        f"- Target seconds: `{result['target_seconds']:.3f}`\n"
        f"- Elapsed seconds: `{result['elapsed_seconds']:.3f}`\n"
        f"- Iterations: `{result['iterations']}`\n"
        f"- Iterations/sec: `{result['iterations_per_second']:.3f}`\n"
        f"- Checksum: `{result['checksum']}`\n"
    )


if __name__ == "__main__":
    main()
