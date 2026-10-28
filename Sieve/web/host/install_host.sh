#!/bin/sh
# Registers Sieve as the web plugin's host (IDEAS 19.92, web/README.md), for this user only:
# writes com.potentia.sieve.json, naming sieve and the extension allowed to start it, where Chrome,
# Chromium and Edge look for it. Nothing else is changed.
#
#   web/host/install_host.sh                 sieve from this installation's tools/, else build/
#   web/host/install_host.sh /path/to/sieve  that sieve
set -e
here=$(cd "$(dirname "$0")" && pwd)
if [ -n "$1" ]; then sieve=$1
elif [ -x "$here/../../tools/sieve" ]; then sieve=$here/../../tools/sieve
else sieve=$here/../../build/sieve
fi
[ -x "$sieve" ] || { echo "sieve is not at $sieve: give its path"; exit 1; }
sieve=$(cd "$(dirname "$sieve")" && pwd)/$(basename "$sieve")
# The path goes into JSON as a string: a backslash or a double quote in it is escaped, and a path
# with a control character (a line break, a tab) is refused, as JSON would need it written as one.
case "$sieve" in
  *[[:cntrl:]]*) echo "sieve's path has a control character in it (a line break or a tab): move it"; exit 1 ;;
esac
json_path=$(printf '%s' "$sieve" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g')
case "$(uname)" in
  Darwin) dirs="$HOME/Library/Application Support/Google/Chrome/NativeMessagingHosts
$HOME/Library/Application Support/Chromium/NativeMessagingHosts
$HOME/Library/Application Support/Microsoft Edge/NativeMessagingHosts" ;;
  *) dirs="$HOME/.config/google-chrome/NativeMessagingHosts
$HOME/.config/chromium/NativeMessagingHosts
$HOME/.config/microsoft-edge/NativeMessagingHosts" ;;
esac
echo "$dirs" | while IFS= read -r d; do
  mkdir -p "$d"
  cat > "$d/com.potentia.sieve.json" <<JSON
{
  "name": "com.potentia.sieve",
  "description": "Sieve: decodes the .sieve files a page links to, for the Sieve extension",
  "path": "$json_path",
  "type": "stdio",
  "allowed_origins": ["chrome-extension://bnigebolckfoapimidlcmbilcbhjbmjg/"]
}
JSON
  echo "registered $sieve in $d"
done
