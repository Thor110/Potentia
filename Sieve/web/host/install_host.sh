#!/bin/sh
# Registers Sieve as the web plugin's host (IDEAS 19.92, web/README.md), for this user only:
# writes com.potentia.sieve.json, naming sieve and the extension allowed to start it, where Chrome,
# Chromium and Edge look for it. Nothing else is changed.
#
#   web/host/install_host.sh                 sieve from build/
#   web/host/install_host.sh /path/to/sieve  that sieve
set -e
here=$(cd "$(dirname "$0")" && pwd)
sieve=${1:-$here/../../build/sieve}
[ -x "$sieve" ] || { echo "sieve is not at $sieve: give its path"; exit 1; }
sieve=$(cd "$(dirname "$sieve")" && pwd)/$(basename "$sieve")
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
  "path": "$sieve",
  "type": "stdio",
  "allowed_origins": ["chrome-extension://bnigebolckfoapimidlcmbilcbhjbmjg/"]
}
JSON
  echo "registered $sieve in $d"
done
