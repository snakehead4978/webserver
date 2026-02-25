#!/usr/bin/env python3
import sys

print("Content-Type: text/html")
print("")
print("<html><body><h1>no content-length response</h1></body></html>", end="")
sys.stdout.flush()