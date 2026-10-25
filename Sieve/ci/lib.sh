# Sieve CI: what every step script takes in first (ci/steps/*.sh; ci/README.md).

# The step's ERR trap (its first line) reaches into functions too, so a failing check made with a
# function (q, as the steps define it) says which line it was, rather than stopping silently.
set -E

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
