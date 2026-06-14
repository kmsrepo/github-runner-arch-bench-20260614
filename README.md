# GitHub Runner Architecture Benchmark

Small public GitHub Actions benchmark comparing GitHub-hosted Linux x64 and arm64 runners.

The workflows compare:

- `ubuntu-24.04` for x64
- `ubuntu-24.04-arm` for arm64

Both labels are standard GitHub-hosted public repository runners. The workflows collect machine metadata, run deterministic workloads, publish a Markdown job summary, and upload JSON artifacts.

## Workflows

- `bench-cpu.yml`: native C integer workload, Python CPU workload, and startup metadata.
- `bench-io.yml`: file write, read, and random-read checks against runner temp storage.
- `bench-toolchain.yml`: repeated native C compile timing and binary run timing.
- `bench-asm.yml`: architecture-specific optimized assembly kernels using x86_64 SSE2 and AArch64 NEON.
- `bench-sustained-c.yml`: about one minute of sustained optimized C mixed integer and floating-point calculation.

## Run With gh

```sh
gh workflow run bench-cpu.yml --field repetitions=3
gh workflow run bench-asm.yml --field repetitions=3
gh workflow run bench-sustained-c.yml --field seconds=60 --field threads=4
gh workflow run bench-io.yml --field size_mb=256 --field repetitions=3
gh workflow run bench-toolchain.yml --field builds=20
```

Watch the latest runs:

```sh
gh run list --limit 10
gh run watch
```

Download artifacts:

```sh
mkdir -p artifacts
gh run download <run-id> --dir artifacts
```

## Notes

These are CI microbenchmarks, not controlled hardware benchmarks. Results can vary with host placement, runner image updates, network/storage conditions, and GitHub Actions queueing.
