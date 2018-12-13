from collections import deque
from collections import namedtuple
import sys
import math

if(len(sys.argv) < 3):
   print "Arguments: <ascii trace file> <output file>"
   sys.exit()

tfile = sys.argv[1]
outfile = sys.argv[2]


HashTuple = namedtuple("HashTuple", "proto srcip dstip srcport dstport")

delaydicts = dict()
delaytimedicts = dict()
queuelen = dict()

#time in terms of packetnum
def addEnqueuePacket(tracesrc, flow, time):
   if(tracesrc not in queuelen):
      queuelen[tracesrc] = 0
   queuelen[tracesrc] += 1

   if(tracesrc not in delaydicts):
      delaydicts[tracesrc] = [] 
      delaytimedicts[tracesrc] = []
   delaydicts[tracesrc].append(queuelen[tracesrc])
   delaytimedicts[tracesrc].append(time)


def addDequeuePacket(tracesrc, flow, time):
   assert (tracesrc in queuelen)
   queuelen[tracesrc] -= 1
   delaydicts[tracesrc].append(queuelen[tracesrc])
   delaytimedicts[tracesrc].append(time)


enqd = 0
dqd = 0
dropped = 0
recvd = 0
max_gap = 0
tfile = tfile.rstrip()

print "Opening ascii trace file",tfile
num_lines = sum(1 for line in open(tfile))
print "num lines in file: ", num_lines
with open(tfile) as logs:
   lasttime = 0
   ctr = 0
   for log in logs:
      if(ctr%30000 == 0):
         print enqd, dqd, dropped, recvd, ctr, max_gap, lasttime
      ctr +=1
      if("UdpHeader" not in log):
         continue
      tokens = log.split()
      offset = 20 #tokens[0] = +, -
      if(tokens[0] == 'r'):
         offset = 15
      #print tokens[0], tokens[offset], tokens[offset+8], tokens[offset+10], tokens[offset+12], tokens[offset+14]
      #print "LOG***** ", log, " *****"
      time = float(tokens[1])
      #if(offset+14 >= len(tokens) or (tokens[0] != 'r' and tokens[0] != '+' and tokens[0] != '-')):
         #print log
         #print tokens
         #continue
         #sys.exit()
      tracesrc = tokens[2].split("$ns3")[0]
      proto = tokens[offset]
      srcip = tokens[offset+8]
      dstip = tokens[offset+10].replace(')', "")
      srcport = int(tokens[offset+14].replace('(', ""))
      dstport = int(tokens[offset+16].replace(")",""))
      #print tokens[0], tracesrc, time, proto, srcip, dstip, srcport, dstport
      flow = HashTuple(proto, srcip, dstip, srcport, dstport)
      if(time > 1.10):
         continue
      if (lasttime == 0):
         lasttime = time
      max_gap = max(max_gap, time - lasttime)
      lasttime = time
      if(tokens[0] == '+'):
         enqd += 1
         addEnqueuePacket(tracesrc, flow, time)
         #print tokens[1], tokens[20], tokens[28], tokens[30], tokens[32], tokens[34]
      elif(tokens[0] == '-'):
         dqd += 1
         addDequeuePacket(tracesrc, flow, time)
      elif(tokens[0] == 'd'):
         dropped += 1
         continue
         #sys.exit()
      elif(tokens[0] == 'r'):
         recvd += 1
      else:
         print "token", tokens[0], "not recognised, exiting.."
         sys.exit()
   print enqd, dqd, dropped, recvd, max_gap, lasttime


print len(delaydicts)

with open(outfile, 'w') as outf:
   for ts in delaydicts:
      outf.write(str(ts)+" "+str(delaydicts[ts]+ delaytimedicts[ts])+"\n") 
      #outf.write(str(ts)+" "+str(sum(delaydicts[ts])/len(delaydicts[ts]))+" "+str(len(delaydicts[ts]))+"\n") 
   outf.close()




