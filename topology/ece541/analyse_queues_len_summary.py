from collections import namedtuple
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys


if(len(sys.argv) < 5):
   print "Arguments: <summary file> <lower bound sw> <upper bound sw> <outfile>"
   sys.exit()


qlensall = dict()

summary_file = sys.argv[1]
lsw = int(sys.argv[2])
usw = int(sys.argv[3])
outfile = sys.argv[4]
with open(summary_file) as summf:
   for f in summf:
      tokens = f.split()
      ts_tok = tokens[0].split("/")
      device = int(ts_tok[2])
      port = int(ts_tok[4])
      val = [float(x.replace("[","").replace(",","").replace("]","")) for x in  tokens[1:]]
      lens  = val[:len(val)/2]
      times = val[len(val)/2:]
      if(device not in qlensall):
         qlensall[device] = []
      qlensall[device].append((lens, times))
      #print t2-t1
   summf.close()

devices = qlensall.keys()
for device in devices:
   mlist = []
   for (qlen,times) in qlensall[device]:
      m_inds = np.argsort(times)
      m_times = [times[i] for i in m_inds]
      m_qlens = [qlen[i] for i in m_inds]
      mlist.append((m_qlens, m_times))
      for i in range(1, len(m_times)):
         assert(m_times[i] >= m_times[i-1])
   qlensall[device] = mlist

def check_sorted_times():
   for device in qlensall:
      #print len(qlensall[device])
      for ctr in range(len(qlensall[device])):
         #print device, qlen
         qlens, times = qlensall[device][ctr]
         for i in range(1, len(times)):
            assert(times[i] >= times[i-1])

check_sorted_times()

#filter out server level queues
qlens = []
devices=[]
for device in qlensall:
   if(device >= lsw and device < usw and len(qlensall[device])>0):
      ctr=0
      for (qlen,times) in qlensall[device]:
         #print device, qlen
         avg = sum(qlen)/len(qlen)
         devices.append((device,ctr))
         qlens.append(avg)
         ctr+=1


########################## plot queue buildup ##########################

fig, ax = plt.subplots()
def plotQBuildup(qlens, time, pcolor, label):
   #fig, ax = plt.subplots()
   #fig = plt.figure(figsize=(12,3))
   #ax.plot(time[0::20], qlens[0::20], '-', color=pcolor,  lw=2.5,  marker='x', mew = 2, markersize = 10, markerfacecolor='none', markeredgecolor=pcolor, dash_capstyle='round', label='RRG')
   ax.plot(time[0:len(time)], qlens[0:len(qlens)], '.', color=pcolor,  lw=0.5,  dash_capstyle='round', label=label, markersize=0.5)
   #label_fontsize = 20
   #leg = ax.legend(bbox_to_anchor=(0.20, 0.83), borderaxespad=0, loc=2, numpoints=2, handlelength=2, fontsize=label_fontsize)
   #leg.get_frame().set_linewidth(0.0)
   #leg.get_frame().set_alpha(0.1)
   #ax.set_xlim(xmax=1.30)
   #plt.tick_params(labelsize=label_fontsize)
   #plt.savefig(outfile+ "_" + label + ".pdf",orientation='landscape')

def plotQDist(qlens, times, pcolor, label):
   max_queue_size = int(max(qlens)) + 1
   qlens_freq = np.zeros(max_queue_size)
   tot_time = times[-1] - times[0]
   for i in range(1, len(times)):
      qlens_freq[int(qlens[i])] += (times[i] - times[i-1])/tot_time
      assert (times[i] >= times[i-1])
   #ax.plot(range(len(qlens_freq)), qlens_freq, '.', color=pcolor,  lw=0.5,  dash_capstyle='round', label=label, markersize=0.5)
  # print qlens_freq
   ax.bar(range(len(qlens_freq)), qlens_freq)


#print qlens
colors = ['r', 'g', 'y', 'black', 'blue', 'grey']
ndevices=1
busy_devices=np.argsort(np.array(qlens))[-5:]
#print busy_devices
itr=0
check_sorted_times()
for i in busy_devices:
   device,ctr = devices[i]
   #print "Helo", device,ctr, len(qlensall[device])
   check_sorted_times()
   lens,times = qlensall[device][ctr]
   label = 'q' + str(5-itr)
   pcolor = colors[itr] #'black'
   plotQDist(lens, times, pcolor, label)

   avg_len = 0.0 #caclculate using area
   avg_len_lb = 0.0 #caclculate using area
   for i in range(1, len(times)):
      avg_len += lens[i] * (times[i] - times[i-1])
      avg_len_lb += lens[i-1] * (times[i] - times[i-1])
   avg_len /= times[-1] - times[0]
   avg_len_lb /= times[-1] - times[0]
   print "Avg. Queue len at device: ", device, avg_len
   #plotQBuildup(lens, times, pcolor, label)
   itr = itr+1

label_fontsize = 20
leg = ax.legend(bbox_to_anchor=(0.72, 0.83), borderaxespad=0, loc=2, numpoints=50, handlelength=2, fontsize=label_fontsize)
leg.get_frame().set_linewidth(0.0)
#leg.get_frame().set_alpha(0.1)
ax.set_ylim(ymax=0.02)
ax.set_xlim(xmax=100)
plt.ylabel('Empirical Probability', fontsize=label_fontsize)
plt.xlabel('Queue size (# packets)', fontsize=label_fontsize)
plt.tick_params(labelsize=label_fontsize)
plt.tight_layout()
plt.savefig(outfile+ ".png",orientation='landscape')
#######################################################################

sys.exit()




def get_p_ile(arr, ile):
   #print len(arr)
   return arr[int((len(arr)-1)*ile)]

qlens.sort()
#print qlens

iles = [0.50, 0.90, 0.95, 0.99, 0.999]
for ile in iles:
   print ile, get_p_ile(qlens, ile)
print "Max ", qlens[-1]
print "Avg ", sum(qlens)/len(qlens)
print "NPoints ", len(qlens)

#print qlens[-100::5]
#for f in fcts:
#   print f


