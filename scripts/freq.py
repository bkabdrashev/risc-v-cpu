#!/usr/bin/env python3
import sys

def extract_total_area(text: str) -> str:
    # Preferred: a dedicated total line, if present
    for line in text.splitlines():
        s = line.strip()
        if s.lower().startswith("total area"):
            parts = s.split()
            # "Total area 123.45" -> take last token if it's not just "area"
            if len(parts) >= 3:
                return parts[-1]
            # If it's just "Total area" with no number, keep searching
            break

    # Fallback: take the chip area line (common in stats dumps)
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("Chip area for module") and ":" in s:
            right = s.split(":", 1)[1].strip()
            return right.split()[0]

    raise ValueError("Could not find total area value.")

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
