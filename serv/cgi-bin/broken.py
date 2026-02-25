#!/usr/bin/env python3

print("Content-Type: text/html")
print()

# this will blow up with a ZeroDivisionError
result = 1 / 0

print("<html><body><h1>You won't see this</h1></body></html>")