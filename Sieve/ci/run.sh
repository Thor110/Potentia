#!/usr/bin/env bash
# Runs Sieve's CI steps (ci/steps/*.sh) here, any of them or all, each as GitHub runs it
# (ci/README.md):
#
#   ci/run.sh                      every step (those marked "only: Linux" only on Linux)
#   ci/run.sh 10 web               steps by number, or by a word of their file's name
#   ci/run.sh --list               the steps, and what each checks
#   ci/run.sh --build DIR 03       the build to use (default: build/, else out/build/x64-Release,
#                                  else out/build/linux-release)
#   ci/run.sh --keep 10            keep the folder it ran in (it is kept anyway when a step fails)
#
# The steps write files where they run, so they run in a fresh copy of the Sieve folder (made
# once for all the steps asked for), with the build copied in as build/. Each step's output goes to
# a log; a failed step's last lines are shown, with where its log is. Each step's commands go to
# a .trace beside its log as they start, so the last line of one says where a step is.
set -o pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
build="" keep=0 list=0 asked=()
while [ $# -gt 0 ]; do
    case "$1" in
        --build) build="$2"; shift 2 ;;
        --keep) keep=1; shift ;;
        --list) list=1; shift ;;
        -h|--help) sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) asked+=("$1"); shift ;;
    esac
done
steps=("$here"/ci/steps/*.sh)
if [ "$list" = 1 ]; then
    for s in "${steps[@]}"; do
        printf '%-22s %s%s\n' "$(basename "$s" .sh)" "$(sed -n '2s/^# Sieve CI, step [0-9]*: //p' "$s")" "$(grep -q '^# only: Linux' "$s" && echo '  (Linux only)')"
    done
    exit 0
fi
# Which steps: every one, or those whose number or name the words give.
chosen=()
for s in "${steps[@]}"; do
    n=$(basename "$s" .sh)
    if [ ${#asked[@]} -eq 0 ]; then
        if grep -q '^# only: Linux' "$s" && [ "$(uname)" != Linux ]; then
            echo "skipped   $n (Linux only)"
            continue
        fi
        chosen+=("$n")
        continue
    fi
    for w in "${asked[@]}"; do
        if [ "${n%%-*}" = "$w" ] || [ "${n%%-*}" = "0$w" ] || [[ "$n" == *"$w"* ]]; then chosen+=("$n"); break; fi
    done
done
[ ${#chosen[@]} -gt 0 ] || { echo "no step matches: ${asked[*]} (ci/run.sh --list)"; exit 2; }
# The build: given, or found.
if [ -z "$build" ]; then
    for b in "$here/build" "$here/out/build/x64-Release" "$here/out/build/linux-release"; do
        if [ -e "$b/sieve" ] || [ -e "$b/sieve.exe" ]; then build="$b"; break; fi
    done
fi
[ -n "$build" ] && { [ -e "$build/sieve" ] || [ -e "$build/sieve.exe" ]; } || { echo "no build with sieve in it: build Release, or give --build DIR"; exit 2; }
build="$(cd "$build" && pwd)"
# A fresh copy of the folder, less its builds, and the build's programs and the folders it copies
# beside them (not its own clutter: CMakeFiles, fetched sources, objects).
work="$(mktemp -d "${TMPDIR:-/tmp}/sieve-ci-XXXXXX")"
echo "copying   the Sieve folder and $build to $work"
(cd "$here" && tar -cf - --exclude=./build --exclude='./build-*' --exclude=./out --exclude=./release --exclude=./.vs --exclude='./ci/work*' .) | (mkdir -p "$work/Sieve" && cd "$work/Sieve" && tar -xf -)
mkdir -p "$work/Sieve/build" "$work/logs"
for e in "$build"/* "$build"/.[!.]*; do
    [ -e "$e" ] || continue
    # Nor what the programs saved there while you used them (the hallway's settings and filters,
    # the music room's, the chat logs): the steps run from the defaults, as on GitHub's runners.
    case "$(basename "$e")" in CMakeFiles|_deps|Testing|*.dir|x64|Win32|.ninja*|build.ninja|Makefile|*.vcxproj*|*.sln|sieve-hallway.ini|sieve-filters.ini|sieve-music.ini|sieve-favourites.sieve|chat-logs) continue ;; esac
    cp -R "$e" "$work/Sieve/build/"
done
failed=()
for n in "${chosen[@]}"; do
    start=$(date +%s)
    echo "running   $n (log: $work/logs/$n.log)"
    # Each command the step runs, as it starts, goes to its .trace (on its own descriptor, so no
    # check's output changes): when a step seems stuck, the trace's last line is where.
    if (cd "$work/Sieve" && BASH_XTRACEFD=9 SIEVE_CI_TRACE=1 bash -x "ci/steps/$n.sh" 9> "$work/logs/$n.trace") < /dev/null > "$work/logs/$n.log" 2>&1; then
        echo "ok        $n ($(( $(date +%s) - start )) s)"
    else
        echo "FAILED    $n ($(( $(date +%s) - start )) s): its last lines, from $work/logs/$n.log"
        tail -8 "$work/logs/$n.log" | sed 's/^/          /'
        echo "          the last command it ran: $(grep -v '::error::' "$work/logs/$n.trace" 2>/dev/null | tail -n 1)"
        failed+=("$n")
    fi
done
if [ ${#failed[@]} -eq 0 ] && [ "$keep" = 0 ]; then
    rm -rf "$work"
    echo "all ${#chosen[@]} passed"
    exit 0
fi
echo "${#failed[@]} of ${#chosen[@]} failed; the folder it ran in is kept: $work"
[ ${#failed[@]} -eq 0 ]
