import argparse
import json
import os
import subprocess
import time
from pathlib import Path

from bench_utils import append_summary, system_info, write_json


def run(command):
    start = time.perf_counter()
    completed = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    elapsed = time.perf_counter() - start
    return {"command": command, "seconds": elapsed, "output": completed.stdout.strip()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--builds", type=int, default=20)
    parser.add_argument("--output", default="results/toolchain.json")
    args = parser.parse_args()

    Path("build").mkdir(exist_ok=True)
    measurements = []
    for build in range(args.builds):
        output = f"build/cpu_bench_{build}"
        measurements.append(
            run(["cc", "-O3", "-pthread", "src/cpu_bench.c", "-o", output])
        )
        os.remove(output)

    run_binary = "build/cpu_bench_final"
    compile_final = run(["cc", "-O3", "-pthread", "src/cpu_bench.c", "-o", run_binary])
    execute_final = run([f"./{run_binary}", "4", "12000000"])

    payload = {
        "benchmark": "toolchain",
        "builds": args.builds,
        "system": system_info(),
        "compile_measurements": measurements,
        "final_compile": compile_final,
        "final_execute": {
            **execute_final,
            "parsed_output": json.loads(execute_final["output"]),
        },
    }
    write_json(args.output, payload)

    compile_total = sum(item["seconds"] for item in measurements)
    compile_avg = compile_total / len(measurements)
    append_summary(
        f"## Toolchain Benchmark\n\n"
        f"- Runner arch: `{payload['system']['runner_arch']}`\n"
        f"- Builds: `{args.builds}`\n"
        f"- Total repeated compile time: `{compile_total:.3f}s`\n"
        f"- Average compile time: `{compile_avg:.3f}s`\n"
        f"- Final binary run time: `{payload['final_execute']['seconds']:.3f}s`\n"
    )


if __name__ == "__main__":
    main()
