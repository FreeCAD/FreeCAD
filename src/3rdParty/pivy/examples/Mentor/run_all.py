from __future__ import print_function
import os

examples = set()
for f in os.listdir(os.path.dirname(os.path.abspath(__file__))):
    name, ext = os.path.splitext(f)
    if ext == ".py" and (name != "run_all"):
        examples.add(f)

examples = sorted(examples)

print(examples)

for example in examples:
    os.system("python " + example)
