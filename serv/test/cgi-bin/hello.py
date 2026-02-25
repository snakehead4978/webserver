#!/usr/bin/env python3
import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET")
query  = os.environ.get("QUERY_STRING", "")

body = "<html><body>"
body += "<h1>CGI works</h1>"
body += "<p>METHOD: " + method + "</p>"
body += "<p>QUERY_STRING: " + query + "</p>"

if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", 0) or 0)
    post_data = sys.stdin.read(length) if length else ""
    body += "<p>POST body: " + post_data + "</p>"

body += "</body></html>"

print("Content-Type: text/html")
print("Content-Length: " + str(len(body)))
print("")
print(body, end="")
