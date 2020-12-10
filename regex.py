import re
import sys
# (?<=\W)b((?=\W)|$)
print(re.sub(sys.argv[1], sys.argv[2], sys.argv[3]))