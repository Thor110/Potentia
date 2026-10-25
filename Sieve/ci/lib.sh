# Sieve CI: what every step script takes in first (ci/steps/*.sh; ci/README.md).

# The step's ERR trap (its first line) reaches into functions too, so a failing check made with a
# function (q, as the steps define it) says which line it was, rather than stopping silently.
set -E

# Under ci/run.sh, each traced command names its file and line (bash takes no PS4 from outside).
[ -n "$SIEVE_CI_TRACE" ] && PS4='+ ${BASH_SOURCE##*/}:$LINENO: '

# python: the oracle and the checks call `python`. Where that is not Python 3 (Windows' Store
# alias, or none), the py launcher stands in for it, as make_release.bat finds Python.
if ! python -c "import sys; sys.exit(sys.version_info < (3, 8))" > /dev/null 2>&1; then
    if py -3 -c "import sys" > /dev/null 2>&1; then
        python() { py -3 "$@"; }
        export -f python
    fi
fi

# SDL with no screen: the drivers the workflow sets for the hallway's steps, unless already set
# (on a Mac the offscreen driver asks for OpenGL, which a runner has not got).
sieve_ci_no_display() {
    if [ -z "$SDL_VIDEO_DRIVER" ]; then
        if [ "$(uname)" = Darwin ]; then export SDL_VIDEO_DRIVER=dummy; else export SDL_VIDEO_DRIVER=offscreen; fi
    fi
    export SDL_RENDER_DRIVER="${SDL_RENDER_DRIVER:-software}"
}

# ctest and cmake: on a runner they are on the PATH. On Windows, Git's bash often has neither (the
# copy Visual Studio brings lives inside its own folder), so they are taken from the build's
# CMakeCache.txt, which names the very programs that configured it.
sieve_ci_tool_from_cache() { # $1 the command, $2 its CMakeCache.txt entry
    command -v "$1" > /dev/null 2>&1 && return 0
    [ -f build/CMakeCache.txt ] || return 0
    local p
    p=$(sed -n "s/^$2:INTERNAL=//p" build/CMakeCache.txt | tr -d '\r' | head -n 1)
    [ -n "$p" ] || return 0
    command -v cygpath > /dev/null 2>&1 && p=$(cygpath -u "$p")
    [ -x "$p" ] || [ -x "$p.exe" ] || return 0
    PATH="$(dirname "$p"):$PATH"
    export PATH
}
sieve_ci_tool_from_cache ctest CMAKE_CTEST_COMMAND
sieve_ci_tool_from_cache cmake CMAKE_COMMAND
