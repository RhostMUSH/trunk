#!/usr/bin/env python
import sys, getopt, json

def is_json(myjson):
  try:
    json_object = json.loads(myjson)
  except ValueError, e:
    return 0
  return 1

print(is_json(sys.argv[1]))
