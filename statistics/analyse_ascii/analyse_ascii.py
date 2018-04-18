from collections import namedtuple
import sys

if(len(sys.argv) < 3):
   print "Arguments: <File containing list of all ascii trace files> <output file>"
   sys.exit()

tfile_names = sys.argv[1]
outfile = sys.argv[2]


HashTuple = namedtuple("HashTuple", "proto srcip dstip srcport dstport")

class FlowEntry(object):
    def __init__(self, minenq, maxrecv, nenq, nrecv):
         self.minEnqueue = minenq
         self.maxReceive = maxrecv
         self.nEnqueue = nenq
         self.nReceive = nrecv

flowdicts = dict()

def addEnqueuePacket(flow, time):
   if(flow not in flowdicts):
      flowdicts[flow] = FlowEntry(100000000.0, 0.0, 0, 0)
   f = flowdicts[flow]
   f.minEnqueue = min(f.minEnqueue, time)
   f.nEnqueue = f.nEnqueue + 1
   flowdicts[flow] = f

def addReceivePacket(flow, time):
   if(flow not in flowdicts):
      flowdicts[flow] = FlowEntry(100000000.0, 0.0, 0, 0)
   f = flowdicts[flow]
   f.maxReceive = max(f.maxReceive, time)
   f.nReceive = f.nReceive + 1
   flowdicts[flow] = f

with open(tfile_names) as tf:
   enqd = 0
   dqd = 0
   dropped = 0
   recvd = 0
   for tfile in tf:
      tfile = tfile.rstrip()
      print "Opening ascii trace file",tfile
      num_lines = sum(1 for line in open(tfile))
      with open(tfile) as logs:
         ctr = 0
         for log in logs:
            if(ctr%300000 == 0):
               print enqd, dqd, dropped, recvd, ctr
            ctr +=1
            tokens = log.split()
            offset = 20 #tokens[0] = +, -
            if(tokens[0] == 'r'):
               offset = 15
            #print tokens[1], tokens[offset], tokens[offset+8], tokens[offset+10], tokens[offset+12], tokens[offset+14]
            time = float(tokens[1])
            if(offset+14 >= len(tokens) or (tokens[0] != 'r' and tokens[0] != '+' and tokens[0] != '-')):
               print log
               print tokens
               continue
               #sys.exit()
            proto = tokens[offset]
            srcip = tokens[offset+8]
            dstip = tokens[offset+10].replace(')', "")
            srcport = int(tokens[offset+12].replace('(', ""))
            dstport = int(tokens[offset+14])
            #print time, proto, srcip, dstip, srcport, dstport
            flow = HashTuple(proto, srcip, dstip, srcport, dstport)
            if(tokens[0] == '+'):
               enqd += 1
               addEnqueuePacket(flow, time)
               #print tokens[1], tokens[20], tokens[28], tokens[30], tokens[32], tokens[34]
            elif(tokens[0] == '-'):
               dqd += 1
            elif(tokens[0] == 'd'):
               dropped += 1
               sys.exit()
            elif(tokens[0] == 'r'):
               recvd += 1
               addReceivePacket(flow, time)
            else:
               print "token", tokens[0], "not recognised, exiting.."
               sys.exit()
         print enqd, dqd, dropped, recvd


print len(flowdicts)

with open(outfile, 'w') as outf:
   for f in flowdicts:
      flow = flowdicts[f]
      outf.write(str(f.proto)+" "+str(f.srcip)+" "+str(f.dstip)+" " +str(f.srcport)+" "+str(f.dstport))
      outf.write(" "+str(flow.minEnqueue)+" "+str(flow.maxReceive)+" "+str(flow.nEnqueue)+" "+str(flow.nReceive)+"\n")
   outf.close()




