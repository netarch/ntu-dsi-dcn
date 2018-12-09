import xml.etree.ElementTree as ET
import sys

if(len(sys.argv) < 2):
   print "Usage: python analyse.py <xml file> <output cdf (optional)> <'bg_flows', optional, default='fg_flows')"
   sys.exit()

xmlfile = sys.argv[1]

outputcdf = False
if(len(sys.argv) > 2 and sys.argv[2] == "cdf"):
   outputcdf = True

consider_bg_flows = False
if(len(sys.argv) > 3 and sys.argv[3] == "bg_flows"):
   consider_bg_flows = True

tree = ET.parse(xmlfile)
root = tree.getroot()

bg_flowids = []
fg_flowids = []


bg_port = '9'
fg_port = '10'

for child in root:
   if(child.tag == 'Ipv4FlowClassifier'):
      for flow in child:
         #print flow.tag, flow.attrib
         if(flow.attrib['sourcePort']==fg_port):
            fg_flowids.append(flow.attrib['flowId'])
         elif(flow.attrib['sourcePort']==bg_port):
            bg_flowids.append(flow.attrib['flowId'])

print "|bg_flows|: ", len(bg_flowids)
print "|fg_flows|: ", len(fg_flowids)

#print "done 1"

def getTimeFromLiteralInNS(lit):
   lit = lit.replace("ns","")
   lit = lit.replace("+", "")
   return float(lit)

#print "done 2"
flow_ids = fg_flowids
if(consider_bg_flows == True):
   flow_ids = bg_flowids

flow_times = []
for child in root:
   if(child.tag == 'FlowStats'):
      for flow in child:
         if(flow.attrib['flowId'] in flow_ids):
           first_tx_time = flow.attrib['timeFirstTxPacket'] 
           last_rx_time = flow.attrib['timeLastRxPacket']
           fctInMicrosec = getTimeFromLiteralInNS(last_rx_time) / 1000.0
           flow_times.append(fctInMicrosec)
           #fctInMicrosec =  (getTimeFromLiteralInNS(last_rx_time) - getTimeFromLiteralInNS(first_tx_time))/1000.0
           #print fctInMicrosec, " usec"
           #if(fctInMicrosec > app_completion_time):
           #    app_completion_time = fctInMicrosec

flow_times.sort()
if(outputcdf):
   flow_times = [x/1000.0 - 1000.0 for x in flow_times]
   for x in flow_times:
      print x
else:
   app_completion_time = flow_times[-1]
   ile_50 = (flow_times[(50 * len(flow_times) + 50)/100 - 1] - 1000000.0)/1000.0
   ile_90 = (flow_times[(90 * len(flow_times) + 90)/100 - 1] - 1000000.0)/1000.0
   ile_99 = (flow_times[(99 * len(flow_times) + 99)/100 - 1] - 1000000.0)/1000.0
   avg_fct = (sum(flow_times)/len(flow_times) - 1000000.0)/1000.0
   #print len(flow_times), avg_fct, ile_50, ile_99, app_completion_time
   print "num flows: ", len(flow_times)
   print "50%ile fct (us): ", ile_50
   print "90%ile fct (us): ", ile_90
   print "99%ile fct (us): ", ile_99
   print "Avg fct (us): ", avg_fct

#print "Numflows: ", len(flow_times)
#print "100 %ile time: ", app_completion_time
#print "99 %ile time: ", ile_99


