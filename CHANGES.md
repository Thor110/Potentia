# Thumbnails without restarting Explorer (5 November 2026)

Commit 9004ed3 (local, not pushed), on top of c18f05f.

Your test showed the handler worked, but Explorer didn't use it until it was restarted. It hadn't re-read which handlers belong to which kinds of file.

## The change

- `install_context.bat` and `uninstall_context.bat` now end by telling Explorer that the kinds of file changed. This is the signal installers send (`SHChangeNotify` with `SHCNE_ASSOCCHANGED`).
- It is sent through PowerShell, which every Windows 10 and 11 has, and takes a second or two. If it fails, it fails silently, and things stay as they were before.
- `context/README.md` says so.

## To test it

1. Run `uninstall_context.bat`. The thumbnails should go back to the viewer's icon without a restart.
2. Run `install_context.bat`. The pictures should come back, again without a restart.

## Notes

- A file Explorer has already shown with the icon may keep it. Disk Cleanup's "Thumbnails" clears those.
- The `.bat` files have CRLF line endings.
