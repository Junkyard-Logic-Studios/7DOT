#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_TYPE="Debug"
BUILD_DIR="build"
BUILD_TESTS="ON"
START=0
RUN_MODE=""
NO_PROMPT=0
GENERATE_VSCODE=1
FORCE_VSCODE=0
CLEAN=0
JOBS=""

usage() {
	cat <<'EOF'
Usage: ./build.sh [options]

Options:
  --debug              Configure Debug build. Default.
  --release            Configure Release build.
  --start              Configure Start the application.
  --build-dir DIR      Build directory. Default: build.
  --jobs N             Parallel build jobs. Default: detected CPU count.
  --run normal         Run build/bin/7dot after building.
  --run test           Run tests after building.
  --run none           Do not run anything after building.
  --no-prompt          Never ask interactively.
  --no-tests           Configure with BUILD_TESTS=OFF.
  --vscode             Generate local .vscode launch/tasks files. Default.
  --no-vscode          Skip VS Code file generation.
  --force-vscode       Overwrite existing .vscode/launch.json and tasks.json.
  --clean              Remove the build directory before configuring.
  -h, --help           Print usage.
EOF
}

die() {
	printf 'Error: %s\n' "$*" >&2
	exit 1
}

detect_jobs() {
	local detected=""

	if command -v sysctl >/dev/null 2>&1; then
		detected="$(sysctl -n hw.ncpu 2>/dev/null || true)"
	fi

	if [[ -z "$detected" ]] && command -v nproc >/dev/null 2>&1; then
		detected="$(nproc 2>/dev/null || true)"
	fi

	if [[ -z "$detected" ]] && command -v getconf >/dev/null 2>&1; then
		detected="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
	fi

	if [[ "$detected" =~ ^[0-9]+$ ]] && (( detected > 0 )); then
		printf '%s\n' "$detected"
	else
		printf '4\n'
	fi
}

write_if_missing_or_forced() {
	local path="$1"
	local content="$2"

	if [[ -e "$path" && "$FORCE_VSCODE" -ne 1 ]]; then
		printf 'Keeping existing %s\n' "$path"
		return
	fi

	printf '%s\n' "$content" > "$path"
	printf 'Wrote %s\n' "$path"
}

generate_vscode_files() {
	mkdir -p .vscode

	local tasks_json
	tasks_json="$(cat <<'JSON'
{
	"version": "2.0.0",
	"tasks": [
		{
			"label": "7DOT: build debug",
			"type": "shell",
			"command": "${workspaceFolder}/build.sh",
			"args": ["--debug", "--run", "none", "--no-prompt"],
			"group": {
				"kind": "build",
				"isDefault": true
			},
			"problemMatcher": ["$gcc"]
		},
		{
			"label": "7DOT: test",
			"type": "shell",
			"command": "${workspaceFolder}/build.sh",
			"args": ["--debug", "--run", "test", "--no-prompt"],
			"group": "test",
			"problemMatcher": ["$gcc"]
		},
		{
			"label": "7DOT: build release",
			"type": "shell",
			"command": "${workspaceFolder}/build.sh",
			"args": ["--release", "--run", "none", "--no-prompt"],
			"problemMatcher": ["$gcc"]
		}
	]
}
JSON
)"

	local launch_json
	launch_json="$(cat <<'JSON'
{
	"version": "0.2.0",
	"configurations": [
		{
			"name": "7DOT: Debug Game",
			"type": "cppdbg",
			"request": "launch",
			"program": "${workspaceFolder}/build/bin/7dot",
			"args": [],
			"stopAtEntry": false,
			"cwd": "${workspaceFolder}",
			"environment": [],
			"externalConsole": false,
			"MIMode": "lldb",
			"preLaunchTask": "7DOT: build debug"
		},
		{
			"name": "7DOT: Debug Tests",
			"type": "cppdbg",
			"request": "launch",
			"program": "${workspaceFolder}/build/bin/7dot_test",
			"args": [],
			"stopAtEntry": false,
			"cwd": "${workspaceFolder}",
			"environment": [],
			"externalConsole": false,
			"MIMode": "lldb",
			"preLaunchTask": "7DOT: build debug"
		}
	]
}
JSON
)"

	write_if_missing_or_forced ".vscode/tasks.json" "$tasks_json"
	write_if_missing_or_forced ".vscode/launch.json" "$launch_json"
}

prompt_for_run_mode() {
	if [[ -n "$RUN_MODE" ]]; then
		return
	fi

	if [[ "$NO_PROMPT" -eq 1 || ! -t 0 ]]; then
		RUN_MODE="none"
		return
	fi

	cat <<'EOF'
Run after build?
  1) normal
  2) test
  3) skip
EOF
	read -r -p "Choice [3]: " choice

	case "${choice:-3}" in
		1|normal) RUN_MODE="normal" ;;
		2|test) RUN_MODE="test" ;;
		3|skip|none) RUN_MODE="none" ;;
		*) die "Invalid run choice: $choice" ;;
	esac
}

run_normal() {
	local executable="$BUILD_DIR/bin/7dot"

	if [[ ! -x "$executable" ]]; then
		die "Game executable not found at $executable"
	fi

	"$executable"
}

run_tests() {
	if [[ "$BUILD_TESTS" != "ON" ]]; then
		die "Tests were not built. Re-run without --no-tests."
	fi

	ctest --test-dir "$BUILD_DIR" --output-on-failure
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--debug)
			BUILD_TYPE="Debug"
			shift
			;;
		--release)
			BUILD_TYPE="Release"
			shift
			;;
    --start)
      START=1
      shift
      ;;
		--build-dir)
			[[ $# -ge 2 ]] || die "--build-dir requires a directory"
			BUILD_DIR="$2"
			shift 2
			;;
		--jobs)
			[[ $# -ge 2 ]] || die "--jobs requires a number"
			[[ "$2" =~ ^[0-9]+$ ]] && (( "$2" > 0 )) || die "--jobs must be a positive number"
			JOBS="$2"
			shift 2
			;;
		--run)
			[[ $# -ge 2 ]] || die "--run requires normal, test, or none"
			case "$2" in
				normal|test|none) RUN_MODE="$2" ;;
				*) die "--run must be normal, test, or none" ;;
			esac
			shift 2
			;;
		--no-prompt)
			NO_PROMPT=1
			shift
			;;
		--no-tests)
			BUILD_TESTS="OFF"
			shift
			;;
		--vscode)
			GENERATE_VSCODE=1
			shift
			;;
		--no-vscode)
			GENERATE_VSCODE=0
			shift
			;;
		--force-vscode)
			FORCE_VSCODE=1
			shift
			;;
		--clean)
			CLEAN=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			die "Unknown option: $1"
			;;
	esac
done

if [[ "$START" -eq 1 ]]; then
  run_normal
  exit 1
fi

if [[ -z "$JOBS" ]]; then
	JOBS="$(detect_jobs)"
fi

if [[ "$CLEAN" -eq 1 ]]; then
	printf 'Removing %s\n' "$BUILD_DIR"
	rm -rf "$BUILD_DIR"
fi

if [[ "$GENERATE_VSCODE" -eq 1 ]]; then
	generate_vscode_files
fi

printf 'Configuring %s build in %s with BUILD_TESTS=%s\n' "$BUILD_TYPE" "$BUILD_DIR" "$BUILD_TESTS"
cmake -S . -B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DBUILD_TESTS="$BUILD_TESTS"

printf 'Building with %s parallel job(s)\n' "$JOBS"
cmake --build "$BUILD_DIR" --parallel "$JOBS"

prompt_for_run_mode

case "$RUN_MODE" in
	normal) run_normal ;;
	test) run_tests ;;
	none) ;;
	*) die "Invalid run mode: $RUN_MODE" ;;
esac
