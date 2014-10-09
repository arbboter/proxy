import os
import time

def isSourceCode(path):
    if not os.path.isfile(path):
        return False
    filename = (os.path.split(path))[-1]
    tmp = filename.split(".")
    suffix = tmp[-1]
    if suffix == "h" or suffix == "H":
        return True
    if suffix == "cpp" or suffix == "c":
        return True
    if suffix == "py":
        return True
    return False


def fileLine(path):
    if not isSourceCode(path):
        return 0
    
    sourcefile = open(path,"r")
    lines = sourcefile.readlines()
    num = 0
    for line in lines:
        num = num + 1
        
    sourcefile.close()
    print path
    print num
    return num


def totalFileName(path):
    if os.path.isfile(path):
        return fileLine(path)
    else:
        num = 0
        for filename in os.listdir(path):
            newPath = os.path.join(path,filename)
            num = num + totalFileName(newPath)

        return num


num = totalFileName("./")
print "total line"
print num
while True:
    time.sleep(10)
