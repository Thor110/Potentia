import os
import sys
from pathlib import Path

def inspect_folder(target_path="."):
    path = Path(target_path)
    
    if not path.exists() or not path.is_dir():
        print(f"Error: '{target_path}' is not a valid directory.")
        return

    for item in sorted(path.iterdir()):
        if item.is_file():
            try:
                stat = item.stat()
                
                # Logical size in bytes
                logical_bytes = stat.st_size
                
                # Bytes on disk (st_blocks represents 512-byte blocks allocated on POSIX systems)
                if hasattr(stat, 'st_blocks'):
                    disk_bytes = stat.st_blocks * 512
                else:
                    # Windows fallback approximation (rounded up to nearest 4KB cluster size, or logical size)
                    cluster_size = 4096
                    disk_bytes = ((logical_bytes + cluster_size - 1) // cluster_size) * cluster_size if logical_bytes > 0 else 0

                # Format with comma separators for thousands
                logical_str = f"{logical_bytes:,}"
                disk_str = f"{disk_bytes:,}"
                
                print(f"{item.name}\t: {logical_str} bytes ({disk_str} bytes on disk)")
                
            except Exception as e:
                print(f"{item.name}\t: Error reading file stats ({e})")

if __name__ == "__main__":
    # Pass directory as an argument, or default to current directory
    directory = sys.argv[1] if len(sys.argv) > 1 else "."
    inspect_folder(directory)