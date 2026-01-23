#!/usr/bin/env python3
import sys

def extract_total_area(text: str) -> str:
    # We want the number at the end of:
    # "Chip area for module '\cpu': 21712.040000"
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("Chip area for module") and ":" in s:
            right = s.split(":", 1)[1].strip()   # "21712.040000"
            # take first token in case anything follows
            return right.split()[0]
    raise ValueError("Could not find chip area line (Chip area for module ...: <number>).")

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py path/to/file.txt", file=sys.stderr)
        sys.exit(2)

    path = sys.argv[1]
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    print(extract_total_area(text))

if __name__ == "__main__":
    main()
