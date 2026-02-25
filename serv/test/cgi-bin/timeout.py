#!/usr/bin/env python3
import time
import os

# sleeps longer than the 30s CGI timeout — should produce a 504
time.sleep(60)

body = "<html><body><h1>you should never see this</h1></body></html>"
print("Content-Type: text/html")
print("Content-Length: " + str(len(body)))
print("")
print(body, end="")
