import xml.etree.ElementTree as ET
import sys

if(len(sys.argv) < 2):
   print "Usage: python analyse.py <xml file>"
   sys.exit()

xmlfile = sys.argv[1]

tree = ET.parse(xmlfile)
root = tree.getroot()

bg_flowids = []
fg_flowids = []

for child in root:
   #print child.tag, child.attrib
   if(child.tag == 'Ipv4FlowClassifier'):
      for flow in child:
         #print flow.tag, flow.attrib
         if(flow.attrib['sourcePort']=='10'):
            fg_flowids.append(flow.attrib['flowId'])
         elif(flow.attrib['sourcePort']=='9'):
            bg_flowids.append(flow.attrib['flowId'])

#print "|bg_flows|: ", len(bg_flowids)
#print "|fg_flows|: ", len(fg_flowids)

#print "done 1"

def getTimeFromLiteralInNS(lit):
   lit = lit.replace("ns","")
   lit = lit.replace("+", "")
   return float(lit)

#print "done 2"

flow_times = []
for child in root:
   if(child.tag == 'FlowStats'):
      for flow in child:
         if(flow.attrib['flowId'] in fg_flowids):
           first_tx_time = flow.attrib['timeFirstTxPacket'] 
           last_rx_time = flow.attrib['timeLastRxPacket']
           fctInMicrosec = getTimeFromLiteralInNS(last_rx_time) / 1000.0
           flow_times.append(fctInMicrosec)
           #fctInMicrosec =  (getTimeFromLiteralInNS(last_rx_time) - getTimeFromLiteralInNS(first_tx_time))/1000.0
           #print fctInMicrosec, " usec"
           #if(fctInMicrosec > app_completion_time):
           #    app_completion_time = fctInMicrosec

flow_times.sort()
app_completion_time = flow_times[-1]
ile_99 = flow_times[(99 * len(flow_times) + 99)/100 - 1]
avg_fct = sum(flow_times)/len(flow_times)
print len(flow_times), avg_fct, ile_99, app_completion_time
#print "Numflows: ", len(flow_times)
#print "100 %ile time: ", app_completion_time
#print "99 %ile time: ", ile_99


