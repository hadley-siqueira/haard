import os
import filecmp

def get_inputs():
    files = [f for f in os.listdir(".") if os.path.isfile(os.path.join(".", f))]
    files = [f for f in files if f.endswith(".hd")]

    return files

inputs = get_inputs()

print("Testing pretty printer...")
for i in inputs:
    print("\tTesting file " + i, end="")
    os.system("hdc --pretty --no-logs " + i + " > " + i + ".out")

    if filecmp.cmp("expected/" + i, i + ".out"):
        print("\t[\033[92mOk\033[0m]")
    else:
        print("\t[\033[91mFail\033[0m]")

    os.system("rm " + i + ".out")

