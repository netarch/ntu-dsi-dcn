from collections import namedtuple
import sys

if(len(sys.argv) < 2):
   print "Arguments: <summary file>"
   sys.exit()


summary_file = sys.argv[1]
tot_fct = 0.0
nflows = 0
fcts = []
with open(summary_file) as summf:
   for f in summf:
      tokens = f.split()
      srcip = tokens[1] 
      dstip = tokens[2]
      t1 = float(tokens[5])
      t2 = float(tokens[6])
      tot_fct += t2-t1
      fcts.append(t2-t1)
      #print t2-t1
      nflows += 1
   summf.close()

def get_p_ile(arr, ile):
   return arr[int((len(arr)-1)*ile)]

fcts.sort()

iles = [0.50, 0.90, 0.95, 0.99, 0.999]
for ile in iles:
   print ile, get_p_ile(fcts, ile)
print "Max FCT: ", fcts[-1]
print "Avg FCT: ", tot_fct/nflows
#for f in fcts:
#   print f


