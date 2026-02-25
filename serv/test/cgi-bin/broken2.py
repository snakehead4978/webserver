#!/usr/bin/env python3

# crash immediately, no headers printed at all
result = 1 / 0

print("Content-Type: text/html")
print()
print("<html><body><h1>You won't see this</h1></body></html>")