# $Id$

import regsub, time
 
#// Please note the two fundamentally different time formats
#// which are used in the following description:
#// <UL>
#// <LI><B>secs:</B><BR>
#//		Time in seconds since some epoch as used by the TCL <I>clock</I> command
#//        (EXCEPT: these seconds are doubles,
#//         whereas the clock command accepts only integers)
#// <LI><B>stamp:</B><BR>
#//		Time string in "YYYY/MM/DD hh:mm:ss" format
#//       as used by the archiver,
#//       with 24h hours and (maybe) fractional seconds
#//		up to the nanosecond level.
#//     <BR>
#//		This string time stamp is certainly slower to handle,
#//		but it is user-readable and does not require knowledge
#//		of a specific epoch, offsets between epochs, leap seconds etc.
#//     The order year/month/day was chosen to have an ASCII-sortable
#//     date.
#// </UL>
#// The preferred format should be "stamp" because it's user-readable
#// and can as well be used in time comparisons.
#// The "secs" format is useful for calculations based on <I>differences</I> in time,
#// but no assumption should be made as far as the absolute epoch is concerned.

def formatValue (v):
	"Return printable string for value"
	if v.valid():
		if v.isInfo():
			return "%s\t%s" % (v.time(), v.status())
		else:
			return "%s\t%s\t%s" % (v.time(), v.text(), v.status())
	else:
		return "<invalid value>"

def stampNow ():
	return secs2stamp(time.time())

def secs2stamp (secs):
	"Convert seconds into time stamp"
	nano = int((secs - int(secs))*1000000000)
	v=time.localtime(secs)
	return values2stamp ((v[0],v[1],v[2],v[3],v[4],v[5],nano))

def stamp2values (stamp):
	"Returns (Year, Month, Day, Hours, Minutes, Seconds, Nano)"
	values=regsub.split (stamp, '[/ :.]')
	if len(values)==6:
		(y,m,d,H,M,S)=map(int,values)
		n=0
	else:
		(y,m,d,H,M,S,n)=map(int,values)
	return (y, m, d, H, M, S, n)

def values2stamp (values):
	"Turn (Year, Month, Day, Hours, Minutes, Seconds, Nano) into time stamp"
	if values[6]:
		return "%4d/%02d/%02d %02d:%02d:%02d.%09d" % values
	else:
		return "%4d/%02d/%02d %02d:%02d:%02d" % values[0:6]
	
def stamp2secs (stamp):
	"Convert stamp into seconds"
	(y,m,d,H,M,S,n) = stamp2values (stamp)
	full=time.mktime((y,m,d,H,M,S,0,0,-1))
	return full + n/1e9

# Test code
if __name__=="__main__":
	print "This module is meant to be imported, not run standalone..."
	print "Tests:"
	print "Timestamp Now: ", stampNow()
	secs=time.time()
	print "Now as seconds:", secs
	stamp=secs2stamp(secs)
	print "Now as stamp:  ", stamp
	print "Values:        ", stamp2values(stamp)
	print "Back to secs:  ", stamp2secs (stamp)













