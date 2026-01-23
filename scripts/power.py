#!/usr/bin/env python3
import sys

def extract_total_power(text: str) -> str:
    # We want the line that starts with "Total Power" and contains "=="
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("Total Power") and "==" in s:
            # Example: "Total Power   ==  6.998e+00 W"
            right = s.split("==", 1)[1].strip()  # "6.998e+00 W"
            first_token = right.split()[0]       # "6.998e+00"
            return first_token
    raise ValueError("Could not find 'Total Power == ...' line.")

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py path/to/file.txt", file=sys.stderr)
        sys.exit(2)

    path = sys.argv[1]
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    print(extract_total_power(text))

if __name__ == "__main__":
    main()
