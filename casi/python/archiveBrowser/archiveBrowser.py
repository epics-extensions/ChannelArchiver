# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# Major modifications/extensions by Bob Hall, 2/14/01 (rdh@slac.stanford.edu)
# --------------------------------------------------------
#
# Example for a scalar browser.
# Certainly not done, but I'm working on it.
# If you have ideas or - even better - code snippets,
# let me know.
#
# If nothing else this is a benchmark:
# 800MHz Win. NT  :  2550 values/sec
# 500MHz Linux box:  1430 values/sec
# 266MHz Linux box:   785 values/sec
# L8 "beowolf"    :   416 values/sec

from Tkinter import *
import Tkinter
import tkFileDialog
import sys
#sys.path[:0] = ['/afs/slac.stanford.edu/package/epics/vers3/R3.13.2/extensions/src']

import Pmw
import string, time, casi
import os
import re
import signal
import calendar
from casiTools import *
from ErrorDlg import ErrorDlg
import ListSelectedChannels
import ListSelectedFilteredChannels


# Global for next color index used by ChannelInfo
# (until I know how to use class 'static' vars)
_next_color=0
_prevTimeLabelTime = 0.0
_prevLogTimeSpanTime = 0.0
_timeLabels = []

class ChannelInfo:
    "Helper class for Plot: hold info on single channel"

    def __init__ (self, name):
        self.name = name

    def deletePlot (self, graph):
        # Delete existing:
        for el in graph.element_names ("%s *" % self.name):
#            print "deletePlot self.name = ", self.name
#            print "el = ", el 
            graph.element_delete (el)

    def dataAvailableToPlot (self, root, archive, channel, value, start, end):
        """ This routine determines if there is enough data available to plot
            for a specified channel.  This routine should be called for each 
            channel to be plotted before attempting to plot data to avoid the
            situation where there is not enough data available to plot for
            any of the channels to be plotted, resulting in bad time (X) axis
            labels.
        """
        if not archive.findChannelByName (self.name, channel):
            ErrorDlg (root, "Cannot find %s" % self.name)
            return
        if not channel.getValueAfterTime (start, value):
            return

        """ Get values for the specified channel from the specified start time
            to the specified end time until enough values (e.g., 3) are found
            for plotting the channel values or the end time is reached.
        """ 
        values =[]
        valueCount = 0
        while value.valid()  and  value.time() <= end and valueCount < 3:
            if value.isInfo() and len(values) :
                valueCount = valueCount + len(values)
                values =[]
            else:
                values.append (value.get())
            value.next()

        if len(values):
            valueCount = valueCount + len(values)

        if valueCount < 3:
            data_available = 0
        else:
            data_available = 1

        return data_available 

    def loadAndPlot (self, root, graph, archive, channel, value, acolor,
                     linewidth_value, dashes_value, symbol_value, fill_value, start, end):
        """Add data for channel from start to end stamp
           * root window
           * BLT graph
           * archive, channel, value form casi
           * color of graph line
           * start/end stamp
        """

#        print "Entering loadAndPlot"
#        print "start = ", start
#        print "end = ", end

        # Delete existing:
        for el in graph.element_names ("%s *" % self.name):
            graph.element_delete (el)

        # Create new:
        if not archive.findChannelByName (self.name, channel):
            ErrorDlg (root, "Cannot find %s" % self.name)
            return
        label = self.name
#        print "label = "
#        print label

        smallest_value = 0.0
        largest_value = 0.0
        times, values =[], []
        all_times = []
        all_values = []
        all_isInfos = []
        segment, valueCount = 0, 0
        first_value = 1

        if not channel.getValueAfterTime (start, value):
            return (valueCount, smallest_value, largest_value, all_times, all_values, all_isInfos)

        value_valid = value.valid()

        cur_time_stamp = value.getDoubleTime()

        end_time = stamp2secs(end)
        while value_valid  and  cur_time_stamp <= end_time:
            all_times.append (cur_time_stamp)

            cur_value = value.get()

            all_values.append(cur_value)

            cur_isInfo = value.isInfo()
            all_isInfos.append(cur_isInfo)

            if cur_isInfo and len(values) : # start new segment
#                print "creating line for"
#                print self.name
#                print "times = "
#                print times
#                print "values = "
#                print values

                graph.line_create ("%s %d" % (self.name, segment),
                                   label=label, color=acolor, linewidth=linewidth_value,
                                   dashes=dashes_value, symbol=symbol_value, fill=fill_value,
                                   xdata=tuple(times), ydata=tuple(values))
                segment = segment + 1
                label = ''
                valueCount = valueCount + len(values)
                times, values =[], []
            else:
                times.append (cur_time_stamp)
                if first_value:
                    first_value = 0
                    smallest_value = cur_value
                    largest_value = cur_value

                else:
                    if cur_value < smallest_value:
                        smallest_value = cur_value

                    if cur_value > largest_value:
                        largest_value = cur_value
 
                values.append (cur_value)

            value.next()

            value_valid = value.valid()

            if value_valid:
                cur_time_stamp = value.getDoubleTime()


        if len(values):
#            print "creating last line for"
#            print self.name
#            print "times = "
#            print times
#            print "values = "
#            print values
            graph.line_create ("%s %d" % (self.name, segment),
                               label=label, color=acolor, linewidth=linewidth_value,
                               dashes=dashes_value, symbol=symbol_value, fill=fill_value,
                               xdata=tuple(times), ydata=tuple(values))
            valueCount = valueCount + len(values)

        return (valueCount, smallest_value, largest_value, all_times, all_values, all_isInfos)

    def loadData (self, root, archive, channel, value, start, end):

        if not archive.findChannelByName (self.name, channel):
            ErrorDlg (root, "Cannot find %s" % self.name)
            return

        smallest_value = 0.0
        largest_value = 0.0
        times, values =[], []
        all_times = []
        all_values = []
        all_isInfos = []
        valueCount = 0
        first_value = 1

        if not channel.getValueAfterTime (start, value):
            return (valueCount, smallest_value, largest_value, all_times, all_values, all_isInfos)

        value_valid = value.valid()

        cur_time_stamp = value.getDoubleTime()

        end_time = stamp2secs(end)
        while value_valid  and  cur_time_stamp <= end_time:
            all_times.append (cur_time_stamp)

            cur_value = value.get()
            all_values.append(cur_value)

            cur_isInfo = value.isInfo()
            all_isInfos.append(cur_isInfo)

            if cur_isInfo and len(values) : # start new segment
                valueCount = valueCount + len(values)
                times, values =[], []
            else:
                times.append (cur_time_stamp)
                if first_value:
                    first_value = 0
                    smallest_value = cur_value
                    largest_value = cur_value

                else:
                    if cur_value < smallest_value:
                        smallest_value = cur_value

                    if cur_value > largest_value:
                        largest_value = cur_value
 
                values.append (cur_value)

            value.next()
            value_valid = value.valid()
            if value_valid:
                cur_time_stamp = value.getDoubleTime()


        if len(values):
            valueCount = valueCount + len(values)

        return (valueCount, smallest_value, largest_value, all_times, all_values, all_isInfos)

    def plotData (self, root, graph, channel_times, channel_values, channel_is_infos, acolor,
                  linewidth_value, dashes_value, symbol_value, fill_value, start, end):

        start_stamp = stamp2secs(start)
        end_stamp = stamp2secs(end)
        num_times = len(channel_times)
#        print "num_times = ", num_times

        found = 0
        i = 0
        while i < num_times and not found:
            if channel_times[i] >= start_stamp:
                found = 1
            else:
                i = i + 1

        if not found: 
#            ErrorDlg (root, "No data for channel %s" % self.name)
            return

        # Delete existing:
        for el in graph.element_names ("%s *" % self.name):
            graph.element_delete (el)

        label = self.name

        times, values =[], []
        segment, valueCount = 0, 0
        while i < num_times and channel_times[i] <= end_stamp:
            if channel_is_infos[i] and len(values): # start new segment
                graph.line_create ("%s %d" % (self.name, segment),
                                   label=label, color=acolor, linewidth=linewidth_value,
                                   dashes=dashes_value, symbol=symbol_value, fill=fill_value,
                                   xdata=tuple(times), ydata=tuple(values))
                segment = segment + 1
                label = ''
                valueCount = valueCount + len(values)
                times, values =[], []
            else:
                times.append (channel_times[i])
 
                values.append (channel_values[i])

            i = i + 1 

        if len(values):
            graph.line_create ("%s %d" % (self.name, segment),
                               label=label, color=acolor, linewidth=linewidth_value,
                               dashes=dashes_value, symbol=symbol_value, fill=fill_value,
                               xdata=tuple(times), ydata=tuple(values))
            valueCount = valueCount + len(values)

class Plot:

    def _channels (self):
        "Return list of channel names currently displayed"
        return map (lambda info: info.name, self._channelInfos)

    def _findLastDay (self):
        "Find last day that has values for current channels"
        end=''
        for name in self._channels ():
            if self._archive.findChannelByName (name, self._channel):
                end = max (end, self._channel.getLastTime ())

        # Last day
	(Year, Month, Day, H, M, S, N) = stamp2values (end)
        start=values2stamp ((Year, Month, Day-1, 0, 0, 0, 0))
        end=values2stamp ((Year, Month, Day, 23, 59, 59, 0))
#        print 'data start = ', start
        return (start, end)

    def _findDataTimeRange (self, channel_list):
        "Find data time range for channels currently displayed"

        start_stamp = ''
        end_stamp = ''
        i = 0
        for name in channel_list:
            if self._archive.findChannelByName (name, self._channel):
                if i == 0:
                    start_stamp = self._channel.getFirstTime()
                else:
                    start_stamp = min (start_stamp, self._channel.getFirstTime ())

                end_stamp = max (end_stamp, self._channel.getLastTime ())
            i = i + 1

        start = start_stamp[:19]
        end = end_stamp[:19]

        return (start, end)

    def _findDataTimeRange2 (self, channel_list):
        "Find data time range for channels currently displayed"

        start_stamp = ''
        end_stamp = ''
        i = 0
        for name in channel_list:
            command_string = 'ArchiveManager -c ' + name + ' -i ' + self._curOpenArchive + ' > /tmp/time_range.txt'
#            print "command_string = ", command_string

            os.system(command_string)

            try:
                f = open("/tmp/time_range.txt", "r")
#                print "opened time_range.txt" 
            except: 
                ErrorDlg (root, "Cannot open time_range.txt")
                return 0

            aline = f.readline()
            if i == 0:
                start_stamp = aline[7:35]
            else:
                start_stamp = min (start_stamp, aline[7:35])

#            print "start_stamp = ", aline[7:35]

            aline = f.readline()
            end_stamp = max (end_stamp, aline[7:35])

#            print "end_stamp =   ", aline[7:35]

            f.close()

            os.unlink("/tmp/time_range.txt")

            i = i + 1

        start = start_stamp[6:10] + '/' + start_stamp[:2] + '/' + start_stamp[3:5] + ' ' + start_stamp[11:19]
        end = end_stamp[6:10] + '/' + end_stamp[:2] + '/' + end_stamp[3:5] + ' ' + end_stamp[11:19]

        return (start, end)

    def _currentTimeRange (self):
        "Returns current start...end time stamps"
        return map (secs2stamp, self._graph.xaxis_limits())

    def _YmdStrptime(self, date_time_str):
        year = int(date_time_str[0:4])
        month = int(date_time_str[5:7])
        day = int(date_time_str[8:10])

        (hour, minute, second, weekday, Julian_day, dst_flag) = self._strptimeComplete(year, month, day, date_time_str)
        return (year, month, day, hour, minute, second, weekday, Julian_day, dst_flag)

    def _mdYStrptime(self, date_time_str):
        month = int(date_time_str[0:2])
        day = int(date_time_str[3:5])
        year = int(date_time_str[6:10])

        (hour, minute, second, weekday, Julian_day, dst_flag) = self._strptimeComplete(year, month, day, date_time_str)
        return (year, month, day, hour, minute, second, weekday, Julian_day, dst_flag)

    def _strptimeComplete(self, year, month, day, date_time_str):
        hour = int(date_time_str[11:13])
        minute = int(date_time_str[14:16])
        second = int(date_time_str[17:])

        weekday = calendar.weekday(year, month, day)

        prev_days = 0
        i = 0
        while i < month - 1:
            prev_days = prev_days + self._nonLeapYearMonthDays[i]
            i = i + 1

        Julian_day = prev_days + day
        if month >= 3 and calendar.isleap(year):
            Julian_day = Julian_day + 1

        dst_flag = -1

        return(hour, minute, second, weekday, Julian_day, dst_flag)

    def _timeIn (self):
        """ Callback function for "Time In" button to implement time zoom in
        """
#        print "Entering _timeIn"
        (x0, x1) = self._graph.xaxis_limits()
#        print "x0 = ", x0
#        print "x1 = ", x1

        """ Use 1/20th of current time interval as a reasonable delta x
        """ 
        dx = (x1 - x0) / 20 

        new_start_time = x0 + dx
        new_end_time = x1 - dx

#        if dx < 0.0000001:
#            return

        self._graph.xaxis_configure(min=new_start_time, max=new_end_time)

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_start_time))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_end_time))
#        print "end_str = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        self.logTimeSpanCheck()

    def _timeInPressed(self, event):
        self._timeIn()
        signal.signal(signal.SIGALRM, self._timeInHandler)
        signal.alarm(self._button_down_action_interval)

    def _timeInHandler(self, signum, frame):
        self._timeIn()
        signal.alarm(self._button_down_action_interval)

    def _timeInReleased(self, event):
        signal.alarm(0)

    def _timeOut (self):
        """ Callback function for "Time Out" button to implement time zoom out 
        """
#        print "Entering _timeOut"

        Pmw.showbusycursor()

        (x0, x1) = self._graph.xaxis_limits()
#        print "x0 = ", x0
#        print "x1 = ", x1

        """ Use 1/20th of current time interval as a reasonable delta x
        """ 
        dx = -(x1 - x0) / 20 

        new_start_time = x0 + dx
        new_end_time = x1 - dx

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_start_time))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_end_time))
#        print "end_str = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        if self._retrievedDataStartDateTime != '':
            loaded_new_data = 0
            data_start_str = self._retrievedDataStartDateTime 
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_start_str) 
            data_start_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if new_start_time < data_start_time:
#                print "new start time < data start time"

                is_before_data = 1
                start = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + ' ' + self._curStartTime
                old_data_start = self._retrievedDataStartDateTime
                self._loadChannels(self._channelInfos, 0, start, old_data_start, is_before_data)

                self._retrievedDataStartDateTime = start
                loaded_new_data = 1

            data_end_str = self._retrievedDataEndDateTime
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_end_str) 
            data_end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if new_end_time > data_end_time:
#                print "new end time > data end time"
                is_before_data = 0 
                now = time.time()
                if new_end_time > now:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))
                else:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(new_end_time))

                self._loadChannels(self._channelInfos, 0, data_end_str, end, is_before_data)
                self._retrievedDataEndDateTime = end
                loaded_new_data = 1

            if loaded_new_data:
                self._plotChannels(self._channelInfos, 0, self._retrievedDataStartDateTime, self._retrievedDataEndDateTime)

        self._graph.xaxis_configure(min=new_start_time, max=new_end_time)

        self.logTimeSpanCheck()

        Pmw.hidebusycursor()

    def _timeOutPressed(self, event):
        self._timeOut()
        signal.signal(signal.SIGALRM, self._timeOutHandler)
        signal.alarm(self._button_down_action_interval)

    def _timeOutHandler(self, signum, frame):
        self._timeOut()
        signal.alarm(self._button_down_action_interval)

    def _timeOutReleased(self, event):
        signal.alarm(0)

    def _panLeft (self):
        """ Callback function for "Pan Left" button
        """
#        print "Entering _panLeft"

        Pmw.showbusycursor()

        (x0, x1) = self._graph.xaxis_limits()
#        print "x0 = ", x0
#        print "x1 = ", x1

        """ Use 1/20th of current time interval as a reasonable delta x
        """ 
        dx = -(x1 - x0) / 20 

        new_start_time = x0 + dx
        new_end_time = x1 + dx

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_start_time))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_end_time))
#        print "end_str = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        if self._retrievedDataStartDateTime != '':
            data_start_str = self._retrievedDataStartDateTime 
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_start_str) 
            data_start_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if new_start_time < data_start_time:
#                print "new start time < data start time"

                is_before_data = 1
                start = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + ' ' + self._curStartTime
                old_data_start = self._retrievedDataStartDateTime
                self._loadChannels(self._channelInfos, 0, start, old_data_start, is_before_data)

                self._retrievedDataStartDateTime = start

                self._plotChannels(self._channelInfos, 0, self._retrievedDataStartDateTime, self._retrievedDataEndDateTime)

        self._graph.xaxis_configure(min=new_start_time, max=new_end_time)

        self.logTimeSpanCheck()

        Pmw.hidebusycursor()

    def _panLeftPressed(self, event):
        self._panLeft()
        signal.signal(signal.SIGALRM, self._panLeftHandler)
        signal.alarm(self._button_down_action_interval)

    def _panLeftHandler(self, signum, frame):
        self._panLeft()
        signal.alarm(self._button_down_action_interval)

    def _panLeftReleased(self, event):
        signal.alarm(0)

    def _panRight (self):
        """ Callback function for "Pan Right" button
        """
#        print "Entering _panRight"

        Pmw.showbusycursor()

        (x0, x1) = self._graph.xaxis_limits()
#        print "x0 = ", x0
#        print "x1 = ", x1

        """ Use 1/20th of current time interval as a reasonable delta x
        """ 
        dx = (x1 - x0) / 20 

        new_start_time = x0 + dx
        new_end_time = x1 + dx

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_start_time))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(new_end_time))
#        print "end_str = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        if self._retrievedDataStartDateTime != '':
            data_end_str = self._retrievedDataEndDateTime
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_end_str) 
            data_end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if new_end_time > data_end_time:
#                print "new end time > data end time"
                is_before_data = 0 
                now = time.time()
                if new_end_time > now:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))
                else:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(new_end_time))

#                print "pan right loadChannels start = ", data_end_str
#                print "pan right loadChannels end =   ", end

                self._loadChannels(self._channelInfos, 0, data_end_str, end, is_before_data)
                self._retrievedDataEndDateTime = end

                self._plotChannels(self._channelInfos, 0, self._retrievedDataStartDateTime, self._retrievedDataEndDateTime)

        self._graph.xaxis_configure(min=new_start_time, max=new_end_time)

        self.logTimeSpanCheck()

        Pmw.hidebusycursor()

    def _panRightPressed(self, event):
        self._panRight()
        signal.signal(signal.SIGALRM, self._panRightHandler)
        signal.alarm(self._button_down_action_interval)

    def _panRightHandler(self, signum, frame):
        self._panRight()
        signal.alarm(self._button_down_action_interval)

    def _panRightReleased(self, event):
        signal.alarm(0)

    def _yIn (self):
        """ Callback function for "Y In" button
        """
        (y0, y1) = self._graph.yaxis_limits()

        """ Use 1/4th of current Y interval as a reasonable delta y 
        """ 
        dy = (y1-y0)/4
#        if dy > 0.0000001: # prevent BLT crash
        self._graph.yaxis_configure(min=y0+dy, max=y1-dy)
    
    def _yOut (self):
        """ Callback function for "Y Out" button
        """
        (y0, y1) = self._graph.yaxis_limits()

        """ Use 1/4th of current Y interval as a reasonable delta y 
        """ 
        dy = -(y1-y0)/4
        self._graph.yaxis_configure(min=y0+dy, max=y1-dy)
    
    def _up (self):
        """ Callback function for "Up" button
        """
        (y0, y1) = self._graph.yaxis_limits()

        """ Use 1/2 of current Y interval as a reasonable delta y 
        """ 
        dy = (y1-y0)/2
        self._graph.yaxis_configure(min=y0+dy, max=y1+dy)
    
    def _down (self):
        """ Callback function for "Down" button
        """
        (y0, y1) = self._graph.yaxis_limits()

        """ Use 1/2 of current Y interval as a reasonable delta y 
        """ 
        dy = -(y1-y0)/2
        self._graph.yaxis_configure(min=y0+dy, max=y1+dy)

    def _autoScale (self):
        if len(self._channelNames) > 0:

            if self._retrievedDataStartDateTime != '':
                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(self._retrievedDataStartDateTime) 
                x_min = time.mktime((Year, Month, Day, H, M, S, W, J, D))

                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(self._retrievedDataEndDateTime) 
                x_max = time.mktime((Year, Month, Day, H, M, S, W, J, D))

                self._graph.xaxis_configure(min=x_min, max=x_max)

#            print "auto scale new min = ", self._smallestValue, " new max = ", self._largestValue 

            self._graph.yaxis_configure(min = self._smallestValue, max = self._largestValue)

            if self._retrievedDataStartDateTime != '':
                self._curStartDate = self._retrievedDataStartDateTime[:4] + '-' + self._retrievedDataStartDateTime[5:7] + '-' + self._retrievedDataStartDateTime[8:10]
                self._curStartTime = self._retrievedDataStartDateTime[11:19]
                self._curEndDate = self._retrievedDataEndDateTime[:4] + '-' + self._retrievedDataEndDateTime[5:7] + '-' + self._retrievedDataEndDateTime[8:10]
                self._curEndTime = self._retrievedDataEndDateTime[11:19]

            self.logTimeSpan()

    def _xAutoScale (self):
        if len(self._channelNames) > 0:

            if self._retrievedDataStartDateTime != '':
                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(self._retrievedDataStartDateTime) 
                x_min = time.mktime((Year, Month, Day, H, M, S, W, J, D))

                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(self._retrievedDataEndDateTime) 
                x_max = time.mktime((Year, Month, Day, H, M, S, W, J, D))

                self._graph.xaxis_configure(min=x_min, max=x_max)

                self._curStartDate = self._retrievedDataStartDateTime[:4] + '-' + self._retrievedDataStartDateTime[5:7] + '-' + self._retrievedDataStartDateTime[8:10]
                self._curStartTime = self._retrievedDataStartDateTime[11:19]
                self._curEndDate = self._retrievedDataEndDateTime[:4] + '-' + self._retrievedDataEndDateTime[5:7] + '-' + self._retrievedDataEndDateTime[8:10]
                self._curEndTime = self._retrievedDataEndDateTime[11:19]

            self.logTimeSpan()

    def _yAutoScale (self):
        if len(self._channelNames) > 0:

#            print "auto scale new min = ", self._smallestValue, " new max = ", self._largestValue 

            self._graph.yaxis_configure(min = self._smallestValue, max = self._largestValue)

            self.logTimeSpan()


    def _zoom (self, x0, y0, x1, y1):
        "Rubberband-zoom-helper"
        self._graph.xaxis_configure(min=x0, max=x1)
        self._graph.yaxis_configure(min=y0, max=y1)
    
    def _mouseUp (self, event):
        "Rubberband-zoom-helper"
        if not self._rubberbanding:
            return
#        self._rubberbanding = 0
#        self._graph.marker_delete("marking rectangle")
        
#        if self._x0 <> self._x1 and self._y0 <> self._y1:
            # make sure the coordinates are sorted
#            if self._x0 > self._x1: self._x0, self._x1 = self._x1, self._x0
#            if self._y0 > self._y1: self._y0, self._y1 = self._y1, self._y0
     
#            if event.num == 1:
#               self._zoom (self._x0, self._y0, self._x1, self._y1) # zoom in
#            else:
#               (X0, X1) = self._graph.xaxis_limits()
#               k  = (X1-X0)/(self._x1-self._x0)
#               x0 = X0 -(self._x0-X0)*k
#               x1 = X1 +(X1-self._x1)*k
               
#               (Y0, Y1) = self._graph.yaxis_limits()
#               k  = (Y1-Y0)/(self._y1-self._y0)
#               self._y0 = Y0 -(self._y0-Y0)*k
#               self._y1 = Y1 +(Y1-self._y1)*k
               
#               self._zoom(self._x0, self._y0, self._x1, self._y1) # zoom out
        self._graph.crosshairs_on ()

    def _mouseMove (self, event):
        "Rubberband-zoom-helper"
        (self._x1, self._y1) = self._graph.invtransform (event.x, event.y)
        if self._rubberbanding:
            coords = (self._x0, self._y0, self._x1, self._y0,
                      self._x1, self._y1, self._x0, self._y1, self._x0, self._y0)
            self._graph.marker_configure("marking rectangle", coords = coords)
        else:
            self._totalPoints = 0
            for i in range(len(self._channelPoints)):
                self._totalPoints = self._totalPoints + self._channelPoints[i]

            pos = "@" +str(event.x) +"," +str(event.y)
            self._graph.crosshairs_configure(position = pos)
            pos = "Time: %s,  Value: %g, Points: %d" % (secs2stamp(self._x1), self._y1, self._totalPoints)
            self._messagebar.message ('state', pos)
                            
    def _mouseDown (self, event):
        "Rubberband-zoom-helper"
#        self._rubberbanding = 0
#        if self._graph.inside (event.x, event.y):
#            self._graph.crosshairs_off ()
#            (self._x0, self._y0) = self._graph.invtransform (event.x, event.y)
#            self._graph.marker_create ("line", name="marking rectangle",
#                                      dashes=(2, 2))
#            self._rubberbanding = 1

    def timeLabel (graph, noclue, xvalue):
        "Helper for axis: Format xvalue from seconds to time"

        global _prevTimeLabelTime
        global _timeLabels
 
        now = time.time()
        call_interval = now - _prevTimeLabelTime
        if call_interval > 5.0:
#            print "resetting _timeLabels to []" 
            _timeLabels = []
        _prevTimeLabelTime = now 

        date_time_label = secs2stamp(float(xvalue))
#        print "date_time_label = ", date_time_label

        time_label = date_time_label[11:]
#        print "time_label = ", time_label

        _timeLabels.append(time_label) 

        xlabel = string.replace (date_time_label, " ", "\n")
        return xlabel

    def _loadAndPlotChannels(self, channel_info_list, first_offset, start_time, end_time):
        """ This routine plots data for each previously specified channel graph
            for a specified time interval.
        """

        """ First check whether there is data available to plot for any specified
            channel for the specified time interval.  If not, we want to avoid
            calling the "loadAndPlot" routine later because this could result in
            bad time axis labels.
        """
        data_available = 0
        for info in channel_info_list:
            data_available = info.dataAvailableToPlot(self._root, self._archive,
                                                      self._channel, self._value,
                                                      start_time, end_time)
            if data_available:
                break


        if data_available:
            """ Data is available for at least one of the specified channels during
                the specified interval.  Plot each specified channel, using the
                next available color for each graph line.
            """
            self._plotStartDateTime = start_time[:4] + '-' + start_time[5:7] + '-' + start_time[8:]
            self._plotEndDateTime = end_time[:4] + '-' + end_time[5:7] + '-' + end_time[8:]

            t0 = time.clock()
            total = 0
            i = first_offset 
            for info in channel_info_list:
                aColor = self._channelColors[i]

                linewidth_value = int(self._channelLinewidths[i])

                if self._channelDashVars[i].get():
                    dashes_value = 1
                else:
                    dashes_value = 0

                if self._channelDataSymbols[i] == 'None':
                    symbol_value = ''
                else:
                    symbol_value = self._channelDataSymbols[i]

                if self._channelSymbolFillVars[i].get():
                    fill_value = 'defcolor'
                else:
                    fill_value = ''

                (num_values, smallest, largest,
                  self._channelTimes[i], self._channelValues[i], self._channelIsInfos[i]) = info.loadAndPlot(self._root,
                                                                                                     self._graph,
                                                                                                     self._archive,
                                                                                                     self._channel,
                                                                                                     self._value,
                                                                                                     aColor,
                                                                                                     linewidth_value,
                                                                                                     dashes_value,
                                                                                                     symbol_value,
                                                                                                     fill_value,
                                                                                                     start_time,
                                                                                                     end_time)

#                print "len self._channelTimes[%d]  = %s" % (i, len(self._channelTimes[i]))
#                print "len self._channelValues[%d] = %s" % (i, len(self._channelValues[i]))

                self._channelPoints[i] = num_values
                total = total + num_values

#                print "i = " + str(i) + " smallest = " + str(smallest) + " largest = " + str(largest) 
                if smallest < self._smallestValue:
                    self._smallestValue = smallest

                if largest > self._largestValue:
                    self._largestValue = largest

                i = i + 1

            """ Output an informational message showing the number of values that were
                plotted per second.
            """
            dt = time.clock()-t0
            if dt:
                self._totalPoints = 0
                for j in range(len(self._channelPoints)):
                    self._totalPoints = self._totalPoints + self._channelPoints[j]

                self._messagebar.message ('state', "%f values/second, Points: %d" % (total/dt, self._totalPoints))
                (self._axisStartTime, self._axisEndTime) = self._currentTimeRange()

        return data_available

    def _loadChannels(self, channel_info_list, first_offset, start_time, end_time, is_before_old_data):

        t0 = time.clock()
        total = 0
        i = first_offset 
        for info in channel_info_list:
            times = []
            values = [] 
            (num_values, smallest, largest, times, values, isInfos) = info.loadData(self._root,
                                                                          self._archive,
                                                                          self._channel,
                                                                          self._value,
                                                                          start_time,
                                                                          end_time)
            if num_values > 0:
                if is_before_old_data:
                    self._channelTimes[i] = times + self._channelTimes[i]
                    self._channelValues[i] = values + self._channelValues[i]
                    self._channelIsInfos[i] = isInfos + self._channelIsInfos[i]
                else:
                    self._channelTimes[i] = self._channelTimes[i] + times
                    self._channelValues[i] = self._channelValues[i] + values
                    self._channelIsInfos[i] = self._channelIsInfos[i] + isInfos
 
#                print "len self._channelTimes[%d]  = %s" % (i, len(self._channelTimes[i]))
#                print "len self._channelValues[%d] = %s" % (i, len(self._channelValues[i]))

                self._channelPoints[i] = self._channelPoints[i] + num_values

#                print "i = " + str(i) + " smallest = " + str(smallest) + " largest = " + str(largest) 
                if smallest < self._smallestValue:
                    self._smallestValue = smallest

                if largest > self._largestValue:
                    self._largestValue = largest

            i = i + 1


        """ Output an informational message showing the number of values that were
            plotted per second.
        """
        dt = time.clock()-t0
        if dt:
            self._totalPoints = 0
            for j in range(len(self._channelPoints)):
                self._totalPoints = self._totalPoints + self._channelPoints[j]

            self._messagebar.message ('state', "%f values/second, Points: %d" % (total/dt, self._totalPoints))
            (self._axisStartTime, self._axisEndTime) = self._currentTimeRange()

    def _plotChannels(self, channel_info_list, first_offset, start_time, end_time):
        self._plotStartDateTime = start_time[:4] + '-' + start_time[5:7] + '-' + start_time[8:]
        self._plotEndDateTime = end_time[:4] + '-' + end_time[5:7] + '-' + end_time[8:]
        total = 0
        i = first_offset 
        for info in channel_info_list:
            aColor = self._channelColors[i]

            linewidth_value = int(self._channelLinewidths[i])

            if self._channelDashVars[i].get():
                dashes_value = 1
            else:
                dashes_value = 0

            if self._channelDataSymbols[i] == 'None':
                symbol_value = ''
            else:
                symbol_value = self._channelDataSymbols[i]

            if self._channelSymbolFillVars[i].get():
                fill_value = 'defcolor'
            else:
                fill_value = ''

            info.plotData(self._root,
                          self._graph,
                          self._channelTimes[i],
                          self._channelValues[i],
                          self._channelIsInfos[i],
                          aColor,
                          linewidth_value,
                          dashes_value,
                          symbol_value,
                          fill_value,
                          start_time,
                          end_time)


            i = i + 1

    def configLoad (self, loadConfigFileName=()):
        """ This routine is called as a result of selecting the "Load..." drop down
            menu item underneath the "File" menu bar item.  The routine displays a
            "open file" dialog box, allowing the user to select an existing configuration
            file to load.  If the user selected the "OK" button, the selected XML format
            configuration file is read and the plot will be setup to reflect the
            configuration file contents, displaying data for the previous 2 weeks.
        """
        if len(loadConfigFileName) < 1:
            all_configs = "*." + self._configExt
            file_name = tkFileDialog.askopenfilename(title='Load Configuration File',
                                                     initialdir=self._configDir,
                                                     filetypes=[("config files", all_configs),("all files", "*")])
            if file_name:
                last_separator_index = string.rfind(file_name, '/')
                if last_separator_index == -1:
                    ErrorDlg (root, "Cannot parse specified file name %s" % file_name)
                    return 0

                if string.find(file_name[last_separator_index + 2:], '.') == -1:
                    self._loadConfigName = file_name + '.' + self._configExt
                else:
                    self._loadConfigName = file_name
            else:
                return

        else:
            if string.find(loadConfigFileName, '/') == -1:
                file_name = self._configDir + '/' + loadConfigFileName
            else:
                file_name = loadConfigFileName

            last_separator_index = string.rfind(file_name, '/')
            if last_separator_index == -1:
                ErrorDlg (root, "Cannot parse specified file name %s" % file_name)
                return 0

            if string.find(file_name[last_separator_index + 2:], '.') == -1:
                self._loadConfigName = file_name + '.' + self._configExt
            else:
                self._loadConfigName = file_name

        if self._loadConfigName:
            """ Open the specified configuration file to be loaded.
            """
            try:
                f = open(self._loadConfigName, "r")
#                print "opened ", self._loadConfigName
            except: 
                ErrorDlg (root, "Cannot open %s" % self._loadConfigName)
                return 0

            Pmw.showbusycursor()

            self.logConfigLoad();

#            x_min_found = 0
#            x_max_found = 0
#            y_log_found = 0
            y_min_found = 0
            y_max_found = 0

            color_list = []
            channel_list = []
            cur_curve_number = 1

            """ Read each line from the configuration file until the end-of-file is
                encountered (the readline function returns a null string).
            """ 
            while 1:
                self._aline = f.readline()
                if self._aline == '':
                    break

                """ If the current line contains the plot title (comments), set
                    the plot title to this string.
                """ 
                tag_contents = self.findTagContents("Comments")
                if tag_contents != '':
                    self._plotTitle = tag_contents
                    self._graph.configure(title=self._plotTitle)
                    continue 

#                tag_contents = self.findTagContents("XMin")
#                if tag_contents != '':
#                    x_min_found = 1
#                    (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(tag_contents) 
#                    x_min = time.mktime((Year, Month, Day, H, M, S, W, J, D))
#                    continue 

#                tag_contents = self.findTagContents("XMax")
#                if tag_contents != '':
#                    x_max_found = 1
#                    (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(tag_contents) 
#                    x_max = time.mktime((Year, Month, Day, H, M, S, W, J, D))
#                    continue

#                """ If the current line contains the Y Log scale flag, set a variable
#                    to store its contents for later use.
#                """
#                tag_contents = self.findTagContents("YLog")
#                if tag_contents != '':
#                    y_log_found = 1
#                    y_log = int(tag_contents)
#                    continue

                """ If the current line contains the Y minimum value, set a variable
                    to store its value for later use.
                """
                tag_contents = self.findTagContents("YMin")
                if tag_contents != '':
                    y_min_found = 1
                    y_min = float(tag_contents)
                    continue 

                """ If the current line contains the Y maximum value, set a variable
                    to store its value for later use.
                """
                tag_contents = self.findTagContents("YMax")
                if tag_contents != '':
                    y_max_found = 1
                    y_max = float(tag_contents)
                    continue

                """ If the current line contains RGB (red-green-blue) values, call a
                    routine for each color to parse the color value in the line and
                    convert it to a two digit hex string.  Then form the color string,
                    which is a concatenation of the '#' character and each two digit
                    hex string.
                """
                if string.find(self._aline, "<RGB") != -1:
                    red_rgb_value_str = self.findRGBValue("red")
                    green_rgb_value_str = self.findRGBValue("green")
                    blue_rgb_value_str = self.findRGBValue("blue")

                    if red_rgb_value_str != '' and green_rgb_value_str != '' and blue_rgb_value_str != '':
                        acolor = "#" + red_rgb_value_str + green_rgb_value_str + blue_rgb_value_str
                        color_list.append(acolor)

                """ If the current line contains the curve number, save it for later use.
                """
                tag_contents = self.findTagContents("Curve.Number")
                if tag_contents != '':
                    cur_curve_number = int(tag_contents)
                    continue

                """ If the current line contains a curve channel name, append the channel
                    name to a list for later use.
                """
                tag_contents = self.findTagContents("Curve.ChannelName")
                if tag_contents != '':
                    channel_list.append(tag_contents)
                    continue 

                """ If the current line contains a tag specifying the curve line width,
                    set the global line width variable for the current curve.
                """
                tag_contents = self.findTagContents("Curve.LineWidth")
                if tag_contents != '':
                    self._channelLinewidths[cur_curve_number - 1] = tag_contents
                    continue

                """ If the current line contains a tag specifying the whether the curve
                    should have dashes, set the global dashes variable for the current curve.
                """
                tag_contents = self.findTagContents("Curve.Dashes")
                if tag_contents != '':
                    dashes_value = int(tag_contents)
                    self._channelDashVars[cur_curve_number - 1].set(dashes_value)
                    continue

                """ If the current line contains a tag specifying the curve data symbol,
                    set the global data symbol variable for the current curve.
                """
                tag_contents = self.findTagContents("Curve.Symbol")
                if tag_contents != '':
                    self._channelDataSymbols[cur_curve_number - 1] = tag_contents 
                    continue

                """ If the current line contains a tag specifying the whether the curve
                    data symbol should be filled, set the global data symbol fill variable
                    for the current curve.
                """
                tag_contents = self.findTagContents("Curve.Fill")
                if tag_contents != '':
                    fill_value = int(tag_contents)
                    self._channelSymbolFillVars[cur_curve_number - 1].set(fill_value)
                    continue

                """ If the current line contains a tag indicating the end of the plot
                    information, use the previously saved configuration file information
                    to setup the plot.
                """
                if string.find(self._aline, "</PlotConfig>") != -1:

                    """ First, remove all of the old channels on the plot
                    """
#                    print "old self._channelNames = ", self._channelNames
                    if self._channelNames != []:
                        self.removeChannels(tuple(self._channelNames))

#                    print "channel_list = ", channel_list
#                    print "color_list = ", color_list

                    self._retrievedDataStartDateTime = ''
                    self._retrievedDataEndDateTime = ''

                    """ Add the new channels to the plot
                    """
                    self.addChannels(tuple(channel_list), tuple(color_list))

#                    if x_min_found and x_max_found:
#                        self._graph.xaxis_configure(min=x_min, max=x_max)
#                    elif x_min_found:
#                        self._graph.xaxis_configure(min=x_min)
#                    elif x_max_found:
#                        self._graph.xaxis_configure(max=x_max)

#                    if y_log_found and y_log:
#                        """ The configuration file specified log y axis.  If the y
#                            axis currently is linear (not log), remove the y scaling
#                            buttons before setting the y axis.  This is needed in
#                            order to prevent the user from changing the y axis scaling
#                            to something outside of the bounds permitted by log y
#                            scaling (which would cause a run-time error).  The y axis
#                            is finally set to log scaling with default minimum and
#                            maximum values.
#                        """
#                        self._yNoLogMin = '' 
#                        self._yNoLogMax = '' 
#                        y_log_set = self._yLogVar.get()
#                        if not y_log_set:
#                            self._Ybuttons.delete(3)
#                            self._Ybuttons.delete(2)
#                            self._Ybuttons.delete(1)
#                            self._Ybuttons.delete(0)
#                            self._yLogVar.set(1)

#                        self._graph.yaxis_configure(logscale = 1, min = "", max = "")
#                    else:
#                        """ The configuration file specified linear (not log) y axis.
#                            If the y axis is currently log, add y scaling buttons.
#                            Finally set the y axis to linear scaling with the minimum
#                            and maximum values that were specified in the configuration
#                            file.
#                        """
#                        self._yNoLogMin = y_min 
#                        self._yNoLogMax = y_max 
#                        y_log_set = self._yLogVar.get()
#                        if y_log_set:
#                            self._Ybuttons.add ('Up', command=self._up)
#                            self._Ybuttons.add ('Down', command=self._down)
#                            self._Ybuttons.add ('Y In', command=self._yIn)
#                            self._Ybuttons.add ('Y Out', command=self._yOut)
#                            self._yLogVar.set(0)

#                        self._graph.yaxis_configure(logscale = 0,
#                            min = y_min,
#                            max = y_max) 

                    self._graph.yaxis_configure(min = y_min, max = y_max)

                    continue

            f.close()

            Pmw.hidebusycursor()

        return 1

    def findTagContents (self, tag):
        """ This routine determines whether the current line being processed
            when loading a configuration file contains the specified tag.  If
            it does contain the specified tag, the contents of the tag is
            returned as a string.  Otherwise, a null string is returned.
        """ 
        tag_start = "<" + tag + ">"
        start_tag_index = string.find(self._aline, tag_start)
        if start_tag_index == -1:
            return ''

        tag_end = "</" + tag + ">"
        end_tag_index = string.find(self._aline, tag_end)
        if end_tag_index == -1:
            return ''

        start_tag_len = len(tag_start)
        start_tag_contents = start_tag_index + start_tag_len
        tag_contents = self._aline[start_tag_contents:end_tag_index]

        return tag_contents

    def findRGBValue (self, rgb_color):
        """ This routine parses a specified color from a RGB tag when
            loading a configuration file and converts the value from
            a string containing a decimal value to a string containing
            a two digit hex value.
        """

        """ First, find the position of the specified color in the current
            line and then find the position of the first double quote
            character that follows.
        """
        i = string.find(self._aline, rgb_color)
        i = i + len(rgb_color)
        found = 0
        while self._aline[i] != '\n' and not found:
            if self._aline[i] == '"':
                found = 1
            else:
                i = i + 1
        if not found:
            return ''

        """ Next find the position of the next double quote character.  The
            string of characters between the two double quote characters
            is the string containing the decimal value of the color.
        """ 
        j = i + 1 
        found = 0
        while self._aline[j] != '\n' and not found:
            if self._aline[j] == '"':
                found = 1
            else:
                j = j + 1

        if not found:
            return '' 

        rgb_int_str = self._aline[i + 1:j]

        """ Finally, convert the string containing the decimal value of the
            color to a two digit hex value (with a leading zero placed at
            the start, if necessary).
        """
        rgb_value = int(rgb_int_str)
        argb_value_str = "%2X" % rgb_value

        if argb_value_str[0] == ' ':
            rgb_value_str = "0" + argb_value_str[1]
        else:
            rgb_value_str = argb_value_str 

        return rgb_value_str

    def configSaveAs (self):
        """ This routine is called as a result of selecting the "Save As..." drop down
            menu item underneath the "File" menu bar item.  The routine displays a
            "Save As" dialog box, allowing the user to specify the filename of the
            file into which the current configuration is stored.
        """
        all_configs = "*." + self._configExt
        file_name = tkFileDialog.asksaveasfilename(title='Save Configuration File As',
                                                              initialdir=self._configDir,
                                                              filetypes=[("config files", all_configs),("all files", "*")])
#        print "request to save as ", file_name 

        if file_name:
            last_separator_index = string.rfind(file_name, '/')
            if last_separator_index == -1:
                ErrorDlg (root, "Cannot parse specified file name %s" % file_name)
                return 0

            if string.find(file_name[last_separator_index + 2:], '.') == -1:
                self._loadConfigName = file_name + '.' + self._configExt
            else:
                self._loadConfigName = file_name

#            print "self._loadConfigName = ", self._loadConfigName
            self.saveConfigFile()

    def configSave (self):
        """ This routine is called as a result of selecting the "Save" drop down
            menu item underneath the "File" menu bar item.  If a configuration
            was previously loaded or saved, the current configuration is stored
            into the last loaded or saved configuration file.
        """
        if not self._loadConfigName:
            ErrorDlg (root, "No previously specified configuration file name")
            return 0

#        print "request to save ", self._loadConfigName

        self.saveConfigFile()

    def saveConfigFile (self):
        """ This routine saves the current configuration into a file.
            The saved configuration consists of the following information:
              1.  Heading information
              2.  A flag indicating whether or not the Y axis is logarithmic
                  (if not, it is linear)
              3.  The Y axis minimum and maximum values (if the Y axis is linear)
              4.  The RGB values for each color used in the graph of a channel
              5.  For each channel graph, the name of the channel
        """
#        print "saving configuration file ", self._loadConfigName

        try:
            f = open(self._loadConfigName, "w")
#            print "opened ", self._loadConfigName
        except: 
            ErrorDlg (root, "Cannot open %s" % self._loadConfigName)
            return 0

        self.logConfigSave();

        f.write('<?xml version =  "1.0" encoding = "UTF-8"?>\n')
        f.write('<!DOCTYPE PlotConfig SYSTEM "Plot.dtd">\n')
        f.write('<PlotConfig>\n')
        f.write('   <Header>\n')
        f.write('      <Comments>' + self._plotTitle + '</Comments>\n')
        f.write('   </Header>\n')
        f.write('   <Axes>\n')

#        t0_stamp, t1_stamp = self._currentTimeRange()
#        t0 = t0_stamp[:19]
#        t1 = t1_stamp[:19]

#        f.write('      <XMin>' + t0 + '</XMin>\n')
#        f.write('      <XMax>' + t1 + '</XMax>\n')

#        y_log_set = self._yLogVar.get()
#        if y_log_set:
#            f.write('      <YLog>' + '1' + '</YLog>\n')
#        else:
#            f.write('      <YLog>' + '0' + '</YLog>\n')

#            y0, y1 = self._graph.yaxis_limits()
#            f.write('      <YMin>' + str(y0) + '</YMin>\n')
#            f.write('      <YMax>' + str(y1) + '</YMax>\n')

        y0, y1 = self._graph.yaxis_limits()
        f.write('      <YMin>' + str(y0) + '</YMin>\n')
        f.write('      <YMax>' + str(y1) + '</YMax>\n')

        f.write('   </Axes>\n')

        """ For each color used in a channel graph, convert the hex representation
            of the RGB values to decimal before writting each value.
        """
        f.write('   <Color>\n')
        for acolor in self._channelColors:
            f.write('      <Color.Channel>\n')

            rgb_red = string.atoi(acolor[1:3], 16)
            rgb_green = string.atoi(acolor[3:5], 16)
            rgb_blue = string.atoi(acolor[5:7], 16)

            f.write('         <RGB red ="' + str(rgb_red) + '" green ="' + str(rgb_green) +
                    '" blue ="' + str(rgb_blue) + '"/>\n')

            f.write('      </Color.Channel>\n')

        f.write('   </Color>\n')

        i = 0 
        for aname in self._channelNames:
            f.write('   <Curve>\n')

            f.write('      <Curve.Number>' + str(i + 1) + '</Curve.Number>\n')
            f.write('      <Curve.ChannelName>' + aname + '</Curve.ChannelName>\n')
            f.write('      <Curve.LineWidth>' + self._channelLinewidths[i] + '</Curve.LineWidth>\n')
            f.write('      <Curve.Dashes>' + str(self._channelDashVars[i].get()) + '</Curve.Dashes>\n')
            f.write('      <Curve.Symbol>' + self._channelDataSymbols[i] + '</Curve.Symbol>\n')
            f.write('      <Curve.Fill>' + str(self._channelSymbolFillVars[i].get()) + '</Curve.Fill>\n')

            f.write('   </Curve>\n')

            i = i + 1

        f.write('</PlotConfig>\n')

        f.close()

    def parseEnvironStr(self, environ_str):
        """ This routine parses the values of an environment variable, returning
            a list of values.
        """
        environ_list = []
        last_offset = len(environ_str)

        if last_offset < 1:
            return environ_list

        cur_offset = 0
        colon_offset = 0
        while colon_offset != -1:
            colon_offset = string.find(environ_str, ':', cur_offset)
            if colon_offset == -1:
                environ_list.append(environ_str[cur_offset:last_offset])
            else:
                environ_list.append(environ_str[cur_offset:colon_offset])
                cur_offset = colon_offset + 1

        return environ_list

    def printToPrinter(self):
        """ This is the callback function called when the user requests to
            bring up a dialog box allowing the plot image to be printed to
            a selected printer. 
        """
        self.toPrinter.activate()

    def toPrinterExecute(self, result):
        """ This routine is called when the user selects the "OK" or "Cancel"
            button in the dialog box used to allow the user to generate
            Postscript data representing the current displayed plot and to
            output this data to a selected printer.
        """
        if result == 'OK':
#            sels = self.toPrinter.getcurselection()
#            if len(sels) == 0:
#                print 'You clicked on', result, '(no selection)'
#                selected_printer = self._last_printer_list_element 
#            else:
#                print 'You clicked on', result, sels[0]

#                selected_printer = sels[0]

#            print "selected_printer = ", selected_printer

            aprinter = self.toPrinter.get()
            if aprinter == '':
                selected_printer = self._last_printer_list_element
            else:
                selected_printer = aprinter

#            print "selected_printer = ", selected_printer 

            """ Form a shell command to print a file to the selected printer.
                Then generate Postscript output of the currently displayed
                plot, invoke the shell script to print the output, and finally
                remove the output file.
            """ 
            printer_command_str = "lpr -P" + selected_printer + " aplot.ps"

            self.writeEditedPostscriptFile("/tmp/aplot.ps")
            os.system(printer_command_str)
            os.unlink("/tmp/aplot.ps")

        self.toPrinter.deactivate(result)

    def printToFile(self):
        """ This is the callback function called when the user requests to
            bring up a dialog box allowing the plot image to be saved to 
            a selected file. 
        """
        all_postscripts = "*." + self._postscriptExt
        file_name = tkFileDialog.asksaveasfilename(title='Print To File',
                                                   initialdir=self._postscriptDir,
                                                   filetypes=[("postscript files", all_postscripts),("all files", "*")])

        if file_name:
            last_separator_index = string.rfind(file_name, '/')
            if last_separator_index == -1:
                ErrorDlg (root, "Cannot parse specified file name %s" % file_name)
                return 0

            if string.find(file_name[last_separator_index + 2:], '.') == -1:
                postscript_file = file_name + '.ps'
            else:
                postscript_file = file_name

            self.writeEditedPostscriptFile(postscript_file) 


    def writeEditedPostscriptFile(self, postscript_file):
 
        f = open(postscript_file, "w")

        ps_str = self._graph.postscript_output(landscape = 1)

        match_list = []
        match_list = self._datePattern.findall(ps_str)
        num_dates = len(match_list)
#        print "num_dates = ", num_dates
        if num_dates < 1:
#            print "No dates found"
            return

        num_time_labels = len(_timeLabels)
#        print "num_time_labels = ", num_time_labels

        cur_time_label_index = num_time_labels - num_dates

#        print "current label = ", _timeLabels[cur_time_label_index]

        now = time.time()
        now_str = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))

        print_time_str = "Print time: " + now_str

        print_time_postscript_1 = "120 20 0 92 "
        print_time_postscript_2 = " BeginText\n10 /Courier-Bold SetFont\n0 0 0 SetFgColor\n"
        print_time_postscript_3 = "(" + print_time_str + ") 120 0 8 DrawAdjText\nEndText\n"
 
        start_search_index = 0
        for i in range(num_dates):
            match_obj = self._datePattern.search(ps_str, start_search_index)
            if match_obj == None:
#                print "No match"
                return

            match_index = match_obj.start(0)
            end_text_str = "EndText\n"
            end_index = string.find(ps_str, end_text_str, match_index)
            if end_index == -1:
#                print "could not find EndText end-of-line after date"
                return
            end_text_str_len = len(end_text_str)
            end_index = end_index + end_text_str_len - 1

            end_text_index = string.rfind(ps_str, "EndText", 0, match_index)
            if end_text_index == -1:
#                print "could not find EndText"
                return

            start_index = string.find(ps_str, "\n", end_text_index)
            if start_index == -1:
#                print "could not find end-of-line after EndText"
                return

#            print "start_index = ", start_index
#            print "end_index = ", end_index
#            print "date postscript info = ", ps_str[start_index:end_index]

            m1 = self._dateYBegin.search(ps_str, start_index)
            if m1 == None:
#                print "No match"
                return

            y_start_index = m1.start(0)
#            print "y start = ", ps_str[y_start_index:y_start_index + 10]

            m2 = self._whiteSpace.search(ps_str, y_start_index)
            if m2 == None:
#                print "No match"
                return

            y_end_index = m2.start(0)
#            print "y value string = ", ps_str[y_start_index:y_end_index]

            y_value = int(ps_str[y_start_index:y_end_index]) + 10
#            print "new y value = ", y_value

            m3 = self._beginText.search(ps_str, y_start_index)
            if m3 == None:
#                print "No match"
                return

            begin_text_index = m3.start(0)

            f.write(ps_str[start_search_index:end_index])

            f.write(ps_str[start_index:y_start_index])

            f.write(str(y_value))

            f.write(" " + ps_str[begin_text_index:match_index])

            f.write(_timeLabels[cur_time_label_index])

            f.write(ps_str[match_index + 10:end_index + 1])

            if i == 0:
                f.write(print_time_postscript_1)

                y_value = y_value + 10

                f.write(str(y_value))

                f.write(print_time_postscript_2)

                f.write(print_time_postscript_3)

            cur_time_label_index = cur_time_label_index + 1
            start_search_index = end_index + 1 

        f.write(ps_str[start_search_index:])
        f.close()


    def openArchive (self, archiveName=None):
        """ This is the callback function called when the user requests to
            bring up a dialog box to open a specified archive file.  
        """
        if not archiveName:
            archiveName = tkFileDialog.askopenfilename(title='Open Archive',
                                                       initialdir=self._archiveDir)

        if archiveName:
            if not self._archive.open (archiveName):
                ErrorDlg (root, "Cannot open specified Channel Archive %s" % archiveName)
                return 0

            self._curOpenArchive = archiveName

            if self._channelNames != []:
                self.removeChannels(tuple(self._channelNames))

            self._retrievedDataStartDateTime = ''
            self._retrievedDataEndDateTime = ''

            last_separator_index = string.rfind(archiveName, '/')
            if last_separator_index == -1:
                ErrorDlg (root, "Cannot parse specified Channel Archive %s" % archiveName)
                return 0

            self._curOpenArchiveDir = archiveName[:last_separator_index + 1]

            system_index = string.find(self._curOpenArchiveDir, 'nlcta')
            if system_index == -1:
                system_index = string.find(self._curOpenArchiveDir, 'pepii')

            if system_index != -1:
                old_system_index = string.find(self._postscriptDir, 'nlcta')
                if old_system_index == -1:
                    old_system_index = string.find(self._postscriptDir, 'pepii')
                if old_system_index != -1:
                    prefix = self._postscriptDir[:old_system_index]
                    suffix = self._postscriptDir[old_system_index + 5:]
                    self._postscriptDir = prefix + self._curOpenArchiveDir[system_index:system_index + 5] + suffix

                old_system_index = string.find(self._matlabDir, 'nlcta')
                if old_system_index == -1:
                    old_system_index = string.find(self._matlabDir, 'pepii')
                if old_system_index != -1:
                    prefix = self._matlabDir[:old_system_index]
                    suffix = self._matlabDir[old_system_index + 5:]
                    self._matlabDir = prefix + self._curOpenArchiveDir[system_index:system_index + 5] + suffix

                old_system_index = string.find(self._logDir, 'nlcta')
                if old_system_index == -1:
                    old_system_index = string.find(self._logDir, 'pepii')
                if old_system_index != -1:
                    prefix = self._logDir[:old_system_index]
                    suffix = self._logDir[old_system_index + 5:]
                    self._logDir = prefix + self._curOpenArchiveDir[system_index:system_index + 5] + suffix

                old_system_index = string.find(self._configDir, 'nlcta')
                if old_system_index == -1:
                    old_system_index = string.find(self._configDir, 'pepii')
                if old_system_index != -1:
                    prefix = self._configDir[:old_system_index]
                    suffix = self._configDir[old_system_index + 5:]
                    self._configDir = prefix + self._curOpenArchiveDir[system_index:system_index + 5] + suffix

        return 1

    def matlabSaveAs (self):
        all_matlabs = "*." + self._matlabExt
        file_name = tkFileDialog.asksaveasfilename(title='Save Matlab File As',
                                                   initialdir=self._matlabDir,
                                                   filetypes=[("matlab files", all_matlabs),("all files", "*")])
#        print "request to save matlab as ", file_name

        if file_name:
            last_separator_index = string.rfind(file_name, '/')
            if last_separator_index == -1:
                ErrorDlg (root, "Cannot parse specified file name %s" % file_name)
                return 0

            if string.find(file_name[last_separator_index + 2:], '.') == -1:
                self._matlabSaveName = file_name + '.mat'
            else:
                self._matlabSaveName = file_name

            if len(self._channelNames) > 0 and self._curOpenArchive != '': 
                try:
                    f = open("/tmp/matlab_channels.txt", "w")
#                    print "opened matlab_channels.txt"
                except: 
                    ErrorDlg (root, "Cannot open matlab_channels.txt")
                    return 0

                for name in self._channelNames:
                    f.write(name + '\n')

                f.close()

                start = self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + '/' + self._curStartDate[:4] + ' ' + self._curStartTime
                end = self._curEndDate[5:7] + '/' + self._curEndDate[8:10] + '/' + self._curEndDate[:4] + ' ' + self._curEndTime

                (Year, Month, Day, H, M, S, W, J, D) = self._mdYStrptime(end) 
                end_seconds = time.mktime((Year, Month, Day, H, M, S, W, J, D))
                end_seconds = end_seconds + 1.0
                end_str = time.strftime('%m/%d/%Y %H:%M:%S', time.localtime(end_seconds))

                # This one uses the SLAC version of the ArchiveManager
                # command_string = './ArchiveManager -start "' + start + '" -end "' + end_str + '" -out_matlab matlab_channels.txt ' + self._curOpenArchive
                #os.system(command_string)
                #mv_string = 'mv archive_matlab.mat ' + self._matlabSaveName
                #os.system(mv_string)
                #os.unlink("/tmp/matlab_channels.txt")

                # This one uses the ArchiveExport tool
                command_string = 'ArchiveExport -start "' + start + '" -end "' + end_str + '" -Matlab -output "' + self._matlabSaveName + '" "' + self._curOpenArchive + '"'

                print "command_string = ", command_string
                os.system(command_string)


    def getColor (self):
        """ This routine is used to get the next available color that is not
            being used for a channel graph in the current displayed plot.
            If all of the available colors are currently being used, colors
            are reused. 
        """
        global _next_color

        i = 0
        found = 0
        for aColorUsed in self._colorUsed:
            if self._colorUsed[i] == 0:
                found = 1
                color = self._colors[i]
                self._colorUsed[i] = 1
                break

            i = i + 1

        if found == 0:
            color = self._colors[_next_color]
            if _next_color < len(self._colors) - 1:   _next_color = _next_color+1
            else:               _next_color = 0

        return color

    def __freeColor (self, theColor):
        """ This routine is called to indicate that a specified color is no longer
            being used for channel graphs in the current displayed plot.
        """
        itemIndex = self._colors.index(theColor)
        self._colorUsed[itemIndex] = 0

    def addChannels (self, names=(), colors=()):
        """ This routine adds one or more channels to the currently displayed plot.
            If no channel names were specified by the caller, this routine displays
            a dialog box to allow the user to specify the added channel(s).  The
            caller may specify the colors to be used for the channels to be added.
            If they are not specified, the next available colors will be used.
        """
        # No names given -> pop up dialog
        if len(names) < 1:
            self.getAddChannelNames()
            self.addChannelsDialog.activate(geometry = 'centerscreenalways')
            for name in self.addChannelNames:
                if name in self._channelNames:
                    self.addChannelNames.remove(name)
                    ErrorDlg(self._root, "Channel %s is already being plotted" % name)

                elif not self._archive.findChannelByName(name, self._channel):
                    self.addChannelNames.remove(name)
                    ErrorDlg(self._root, "Channel %s is not archived" % name)

            names = tuple(self.addChannelNames)
#            print "names = ", names
        if len(names) < 1:
            return # Still no names? Well, I tried!

        """ For each channel to be added, add its channel information, name, and
            color to be used to graph it to separate lists.
        """
        new_channel_infos = []
        first_offset = len(self._channelInfos) 
        first = len(self._channelInfos) == 0
        i = 0
        for name in names:
#            print "addChannels name = " + name 
            found = filter (lambda info, n=name: info.name==n, self._channelInfos)
            if len(found) == 0:
                self.logChannelName(name)
                new_channel_infos.append (ChannelInfo (name))
                self._channelInfos.append (ChannelInfo (name))
                self._channelNames.append (name)
                self._channelPoints.append(0)
                self._channelTimes.append([])
                self._channelValues.append([])
                self._channelIsInfos.append([])
                if len(colors) < 1:
                    aColor = self.getColor()
                else:
                    aColor = colors[i]
                    item_index = self._colors.index(aColor)
                    if item_index != -1:
                        self._colorUsed[item_index] = 1
                    i = i + 1
                    if i >= len(colors):
                        i = 0
                self._channelColors.append(aColor)

        if self._retrievedDataStartDateTime == '':
            (range_start, end) = self._findDataTimeRange2(self._channelNames)
#            print "end = ", end

            if end == '':
                self.removeChannels(tuple(self._channelNames))
                ErrorDlg (root, "No data for specified channels in currently opened archive")
                return

            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(end)
            D = -1
            x_max = time.mktime((Year, Month, Day, H, M, S, W, J, D))

            x_min = x_max - self._secsPerDay

            self._graph.xaxis_configure(min=x_min, max=x_max)

            start = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(x_min)) 

            start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(x_min))
#            print "start_str = ", start_str

            self._curStartDate = start_str[:10]
            self._curStartTime = start_str[11:]

            end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(x_max))
#            print "end_str   = ", end_str

            self._curEndDate = end_str[:10]
            self._curEndTime = end_str[11:]

        else:
            start = self._retrievedDataStartDateTime
            end = self._retrievedDataEndDateTime

        if first:
            self._smallestValue = float(sys.maxint)
            self._largestValue = - float(sys.maxint)

        Pmw.showbusycursor()

#        print "start = ", start
#        print "end   = ", end
 
        data_available = self._loadAndPlotChannels(new_channel_infos, first_offset, start, end)

#        print "data_available = ", data_available

        Pmw.hidebusycursor()

        if first and data_available:
#            print "new min = ", self._smallestValue, " new max = ", self._largestValue 
            self._graph.yaxis_configure(min = self._smallestValue, max = self._largestValue)

        if self._retrievedDataStartDateTime == '':
            self._retrievedDataStartDateTime = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + ' ' + self._curStartTime
            self._retrievedDataEndDateTime = self._curEndDate[:4] + '/' + self._curEndDate[5:7] + '/' + self._curEndDate[8:10] + ' ' + self._curEndTime

    def getAddChannelNames (self):

        self.addChannelNames = []

        self.addChannelsDialog = Pmw.Dialog(self._addChannelsWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Add Channels',
            command = self.addChannelsExecute)
        self.addChannelsDialog.withdraw()

        self.addChannelsField = Pmw.EntryField(self.addChannelsDialog.interior(),
                labelpos = 'w',
                label_text = 'Channel')
        self.addChannelsField.pack(side = 'top', fill = 'both', pady=10)

        add_button = Tkinter.Button(self.addChannelsDialog.interior(), text = 'Valid Channels Add...',
                command = self.listSelectedFilteredAdd)
        add_button.pack(side = 'top', fill = 'both', padx=20, pady=20)

    def addChannelsExecute(self, result):
#        print 'You clicked on', result
        if result == 'OK':
            achannel = self.addChannelsField.get()
            if achannel != '':
                self.addChannelNames.append(achannel)

        self.addChannelsDialog.deactivate(result)

    def listSelectedFilteredAdd (self):
        result = 'Cancel'
        self.addChannelsDialog.deactivate(result)
        dlg = ListSelectedFilteredChannels.ChannelDialog (self._root, self._archive, "-", self._channelNames,
                                                          self._curOpenArchiveDir)
        names = dlg.activate()

        for name in names:
            self.addChannelNames.append(name)

    def removeChannels (self, names=()):
        "Menu command"
        # No names given -> pop up dialog
        if len(names) < 1:
#            dlg = ListSelectedChannels.ChannelDialog (self._root, self._archive, "+", self._channelNames)
#            names = dlg.activate()
            self.getRemoveChannelNames()
            self.removeChannelsDialog.activate(geometry = 'centerscreenalways')
            for name in self.removeChannelNames:
                if not name in self._channelNames:
                    self.removeChannelNames.remove(name)
                    ErrorDlg(self._root, "Channel %s is not currently being plotted" % name)

            names = tuple(self.removeChannelNames)
#            print "names = ", names
        if len(names) < 1:
            return # Still no names? Well, I tried!

#        print "len of channelInfos = "
#        print len(self._channelInfos)
#        print "len of channelNames = "
#        print len(self._channelNames)  
        for name in names:
#            print "removeChannels name = " + name 
            found = filter (lambda info, n=name: info.name==n, self._channelInfos)
            if len(found) != 0:
#                 print "in list"
#                 print self._channelNames

                 item_index = self._channelNames.index(name)
                 del self._channelPoints[item_index]
                 del self._channelTimes[item_index]
                 del self._channelValues[item_index]
                 del self._channelIsInfos[item_index]
                 self._channelNames.remove (name)
                 theColor = self._channelColors[item_index] 
                 self._channelColors.remove (theColor)
                 self.__freeColor(theColor)

#                 print "calling deletePlot"
                 self._channelInfos[item_index].deletePlot(self._graph)

                 self._channelInfos = []
                 for aname in self._channelNames:
                     self._channelInfos.append (ChannelInfo (aname))

#                 print "len of channelInfos = "
#                 print len(self._channelInfos)
#                 print "len of channelNames = "
#                 print len(self._channelNames)

        self._totalPoints = 0
        for i in range(len(self._channelPoints)):
            self._totalPoints = self._totalPoints + self._channelPoints[i]

        self._messagebar.message ('state', "Points: %d" % (self._totalPoints))

    def getRemoveChannelNames (self):

        self.removeChannelNames = []

        self.removeChannelsDialog = Pmw.Dialog(self._removeChannelsWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Remove Channels',
            command = self.removeChannelsExecute)
        self.removeChannelsDialog.withdraw()

        self.removeChannelsField = Pmw.EntryField(self.removeChannelsDialog.interior(),
                labelpos = 'w',
                label_text = 'Channel')
        self.removeChannelsField.pack(side = 'top', fill = 'both', pady=10)

        remove_button = Tkinter.Button(self.removeChannelsDialog.interior(), text = 'Valid Channels Remove...',
                command = self.listSelectedFilteredRemove)
        remove_button.pack(side = 'top', fill = 'both', padx=20, pady=20)

    def removeChannelsExecute(self, result):
#        print 'You clicked on', result
        if result == 'OK':
            achannel = self.removeChannelsField.get()
            if achannel != '':
                self.removeChannelNames.append(achannel)

        self.removeChannelsDialog.deactivate(result)

    def listSelectedFilteredRemove (self):
        result = 'Cancel'
        self.removeChannelsDialog.deactivate(result)

        dlg = ListSelectedChannels.ChannelDialog (self._root, self._archive, "+", self._channelNames)
        names = dlg.activate()

        for name in names:
            self.removeChannelNames.append(name)

    def setPlotTitle (self):
        "Menu command"
        self.plotTitleDialog = Pmw.Dialog(self._titleWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Plot Title',
            command = self.plotTitleExecute)
        self.plotTitleDialog.withdraw()

        self._titleEntry = Pmw.EntryField(self.plotTitleDialog.interior(),
                labelpos = 'w',
                label_text = 'Plot Title:',
                validate = None)
        self._titleEntry.pack(side = 'top', expand = 1, fill = 'both')

        self.plotTitleDialog.activate(geometry = 'centerscreenalways')

    def plotTitleExecute (self, result):
        if result == 'OK':
            self._plotTitle = self._titleEntry.get()
#            print "Entered plot title = ", self._plotTitle
 
            self._graph.configure(title=self._plotTitle)

        self.plotTitleDialog.deactivate(result)

    def setCurveStyles (self):
        self.curveStylesDialog = Pmw.Dialog(self._curveStylesWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Curve Styles',
            command = self.curveStylesExecute)
        self.curveStylesDialog.withdraw()

        self._row_frames = []
        self._row_labels = []
        self._row_linewidths = []
        self._row_dashs = []
        self._row_data_symbols = []
        self._row_symbol_fills = []

        blank = ' '

        i = 0
        for name in self._channelNames:
            self._row_frames.append(Tkinter.Frame(self.curveStylesDialog.interior(), borderwidth=1, relief=RIDGE))
            self._row_frames[i].pack(side = 'top', expand = 1, fill = 'both')

            self._row_labels.append(Tkinter.Label(self._row_frames[i], text = name, width = 30, anchor = W))
            self._row_labels[i].pack(side=LEFT, expand=YES, fill=BOTH)

            self._row_linewidths.append(Pmw.ComboBox(self._row_frames[i],
                                                     label_text = 'Line Width:',
                                                     labelpos = 'nw',
                                                     scrolledlist_items = tuple(self._linewidths),
                                                     dropdown = 1))
            self._row_linewidths[i].component('entryfield').configure(validate = self.validateLinewidth)

            self._row_linewidths[i].pack(side=LEFT, expand=YES, fill=BOTH)

            self._row_linewidths[i].selectitem(self._channelLinewidths[i])

            self._tempChannelDashVars[i].set(self._channelDashVars[i].get())

            self._row_dashs.append(Tkinter.Checkbutton(self._row_frames[i],
                                   variable = self._tempChannelDashVars[i],
                                   text = 'Dashed', command = self.toggleDashed))
            self._row_dashs[i].pack(side = 'left', expand=YES, fill=BOTH)

            self._row_data_symbols.append(Pmw.ComboBox(self._row_frames[i],
                                                       label_text = 'Data Symbol:',
                                                       labelpos = 'nw',
                                                       scrolledlist_items = tuple(self._data_symbols),
                                                       dropdown = 1))
            self._row_data_symbols[i].component('entryfield').configure(validate = self.validateDataSymbols)

            self._row_data_symbols[i].pack(side=LEFT, expand=YES, fill=BOTH)

            self._row_data_symbols[i].selectitem(self._channelDataSymbols[i])

            self._tempChannelSymbolFillVars[i].set(self._channelSymbolFillVars[i].get())

            self._row_symbol_fills.append(Tkinter.Checkbutton(self._row_frames[i],
                                          variable = self._tempChannelSymbolFillVars[i],
                                          text = 'Symbol Fill', command = self.toggleSymbolFilled))
            self._row_symbol_fills[i].pack(side = 'left', expand=YES, fill=BOTH)

            i = i + 1

        Pmw.alignlabels(tuple(self._row_labels))
        Pmw.alignlabels(tuple(self._row_linewidths))
        Pmw.alignlabels(tuple(self._row_dashs))

        self.curveStylesDialog.activate(geometry = 'centerscreenalways')

    def curveStylesExecute (self, result):
        if result == 'OK':
            data_symbol_specified = 0
            i = 0 
            for name in self._channelNames:
                self._channelLinewidths[i] = self._row_linewidths[i].get()
                self._channelDashVars[i].set(self._tempChannelDashVars[i].get())
                self._channelDataSymbols[i] = self._row_data_symbols[i].get()
                if self._channelDataSymbols[i] != 'None':
                    data_symbol_specified = 1
                self._channelSymbolFillVars[i].set(self._tempChannelSymbolFillVars[i].get())
                i = i + 1

            if data_symbol_specified:
                self.curveStylesSymbolWarningDialog = Pmw.MessageDialog(self._curveStylesWidget,
                    title = 'Data Symbol Warning',
                    message_text = 'Data symbols on curves may cause the Archive Browser\n' +
                                   'to crash when the number of data points is very large.',
                    iconpos = 'w',
                    icon_bitmap = 'warning',
                    command = self.curveStylesWarningExecute,
                    buttons = ('OK', 'Cancel'),
                    defaultbutton = 'OK')
                self.curveStylesSymbolWarningDialog.iconname('Data Symbol Warning Dialog')
                self.curveStylesSymbolWarningDialog.withdraw()
                self.curveStylesSymbolWarningDialog.activate(geometry = 'centerscreenalways')
            else:
                self.applyCurveStyles()
                self.curveStylesDialog.deactivate(result)

        else:
            self.curveStylesDialog.deactivate(result)

    def applyCurveStyles(self):
        for curvename in self._graph.element_show():
            found = 0
            i = 0
            for name in self._channelNames:
                channelname = name + " "
                if string.find(curvename, channelname) != -1:
                    found = 1
                    break
                else:
                    i = i + 1

            if found:
                linewidth_value = int(self._channelLinewidths[i])

                if self._channelDashVars[i].get():
                    dashes_value = 1
                else:
                    dashes_value = 0

                if self._channelDataSymbols[i] == 'None':
                    symbol_value = ''
                else:
                    symbol_value = self._channelDataSymbols[i]

                if self._channelSymbolFillVars[i].get():
                    fill_value = 'defcolor'
                else:
                    fill_value = ''

                self._graph.element_configure(curvename, linewidth=linewidth_value,
                                              dashes=dashes_value, symbol=symbol_value,
                                              fill=fill_value)
    def curveStylesWarningExecute(self, result):
        self.curveStylesSymbolWarningDialog.deactivate(result)
        if result == 'OK':
            self.applyCurveStyles()
            self.curveStylesDialog.deactivate(result)

    def validateLinewidth (self, text):
        found = 0
        i = 0
        while i < len(self._linewidths) and not found:
            if text == self._linewidths[i]:
                found = 1
            else:
                i = i + 1

        if found:
            return 1
        else:
            return -1

    def validateDataSymbols (self, text):
        found = 0
        i = 0
        while i < len(self._data_symbols) and not found:
            if text == self._data_symbols[i]:
                found = 1
            else:
                i = i + 1

        if found:
            return 1
        else:
            return -1

    def toggleDashed (self):
        return

    def toggleSymbolFilled (self):
        return

#    def toggleYLog (self):
#        y_log_set = self._yLogVar.get()
#        if y_log_set:
#            self._Ybuttons.delete(3)
#            self._Ybuttons.delete(2)
#            self._Ybuttons.delete(1)
#            self._Ybuttons.delete(0)
#            self._yNoLogMin, self._yNoLogMax = self._graph.yaxis_limits()
#            self._graph.yaxis_configure(logscale = 1, min = "", max = "") 
         
#        else:
#            self._Ybuttons.add ('Up', command=self._up)
#            self._Ybuttons.add ('Down', command=self._down)
#            self._Ybuttons.add ('Y In', command=self._yIn)
#            self._Ybuttons.add ('Y Out', command=self._yOut)
#            self._graph.yaxis_configure(logscale = 0,
#                                        min = self._yNoLogMin,
#                                        max = self._yNoLogMax) 

    def browserHelp (self):
        try:
            browser = os.environ['BROWSER']
        except:
            browser = 'shelp'

        try:
            arbrowser = os.environ['ARBROWSER']
        except:
            arbrowser = './archiveBrowser'

        command_string = browser + ' ' + arbrowser + '/archive_browser_users_guide.htm'
#        print "command_string = ", command_string
        os.system(command_string)

    def setTimeSpan (self):
        "Menu command"
        self.timeSpanDialog = Pmw.Dialog(self._timeWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Time Span',
            command = self.timeSpanExecute)
        self.timeSpanDialog.withdraw()

        self.top_row = Tkinter.Frame(self.timeSpanDialog.interior())
        self.top_row.pack(side = 'top', expand = 1, fill = 'both')

        self._startDate = Pmw.Counter(self.top_row,
                labelpos = 'w',
                label_text = 'Start Date  yyyy-mm-dd',
                entryfield_value = self._curStartDate,
                entryfield_validate = {'validator' : 'date', 'format' : 'ymd',
                        'separator' : '-' },
                datatype = {'counter' : 'date', 'format' : 'ymd', 'yyyy' : 1,
                        'separator' : '-' })
        self._startDate.pack(side = 'left', expand = 1, fill = 'both')

        self._startTime = Pmw.TimeCounter(self.top_row,
                labelpos = 'w',
                label_text = '     hh:mm:ss',
                value = self._curStartTime,
                min = '00:00:00',
                max = '23:59:59')
        self._startTime.pack(side = 'left', expand = 1, fill = 'both')


        self.bottom_row = Tkinter.Frame(self.timeSpanDialog.interior())
        self.bottom_row.pack(side = 'top', expand = 1, fill = 'both')

        self._endDate = Pmw.Counter(self.bottom_row,
                labelpos = 'w',
                label_text = 'End Date    yyyy-mm-dd',
                entryfield_value = self._curEndDate,
                entryfield_validate = {'validator' : 'date', 'format' : 'ymd',
                        'separator' : '-' },
                datatype = {'counter' : 'date', 'format' : 'ymd', 'yyyy' : 1,
                        'separator' : '-' })
        self._endDate.pack(side = 'left', expand = 1, fill = 'both')

        self._endTime = Pmw.TimeCounter(self.bottom_row,
                labelpos = 'w',
                label_text = '     hh:mm:ss',
                value = self._curEndTime,
                min = '00:00:00',
                max = '23:59:59')
        self._endTime.pack(side = 'left', expand = 1, fill = 'both')

        self.timeSpanDialog.activate(geometry = 'centerscreenalways')

    def timeSpanExecute(self, result):
#        print 'You clicked on', result
        if result == 'OK':
            astart_date = self._startDate.get()
            astart_time = self._startTime.getstring()
            aend_date = self._endDate.get()
            aend_time = self._endTime.getstring()
            
            astart_str = astart_date + ' ' + astart_time 
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(astart_str) 
            start_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))

            aend_str = aend_date + ' ' + aend_time 
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(aend_str) 
            end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))

            if end_time < start_time:
                ErrorDlg(self._root, "End time is earlier than start time")
                return

            Pmw.showbusycursor()

            self._curStartDate = astart_date
            self._curStartTime = astart_time
            self._curEndDate = aend_date 
            self._curEndTime = aend_time

            self.logTimeSpan()

            start = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10]
            start = start + ' ' + self._curStartTime
            end = self._curEndDate[:4] + '/' + self._curEndDate[5:7] + '/' + self._curEndDate[8:10] + ' ' + self._curEndTime

#            print 'new start = ', start
#            print 'new end   = ', end

            if self._retrievedDataStartDateTime != '':
                loaded_new_data = 0
                data_start_str = self._retrievedDataStartDateTime 
                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_start_str) 
                data_start_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
                if start_time < data_start_time:
#                    print "new start time < data start time"

                    is_before_data = 1
                    start = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + ' ' + self._curStartTime
                    old_data_start = self._retrievedDataStartDateTime
                    self._loadChannels(self._channelInfos, 0, start, old_data_start, is_before_data)

                    self._retrievedDataStartDateTime = start
                    loaded_new_data = 1

                data_end_str = self._retrievedDataEndDateTime
                (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_end_str) 
                data_end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
                if end_time > data_end_time:
#                    print "new end time > data end time"
                    is_before_data = 0 
                    now = time.time()
                    if end_time > now:
                        end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))
                    else:
                        end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(end_time))

                    self._loadChannels(self._channelInfos, 0, data_end_str, end, is_before_data)
                    self._retrievedDataEndDateTime = end
                    loaded_new_data = 1

                if loaded_new_data:
                    self._plotChannels(self._channelInfos, 0, self._retrievedDataStartDateTime, self._retrievedDataEndDateTime)

            self._graph.xaxis_configure(min=start_time, max=end_time)

            self.timeSpanDialog.deactivate(result)

            Pmw.hidebusycursor()

        if result == 'Cancel':
            self.timeSpanDialog.deactivate(result)

    def setTimeLast2Hours (self):
        days = 2.0 / 24.0
        self.setTimeLastPeriod(days)

    def setTimeLast4Hours (self):
        days = 4.0 / 24.0
        self.setTimeLastPeriod(days)

    def setTimeLast8Hours (self):
        days = 8.0 / 24.0
        self.setTimeLastPeriod(days)

    def setTimeLast12Hours (self):
        days = 0.5
        self.setTimeLastPeriod(days)

    def setTimeLast24Hours (self):
        days = 1.0 
        self.setTimeLastPeriod(days)

    def setTimeLast48Hours (self):
        days = 2.0
        self.setTimeLastPeriod(days)

    def setTimeLast3Days (self):
        days = 3.0 
        self.setTimeLastPeriod(days)

    def setTimeLast4Days (self):
        days = 4.0
        self.setTimeLastPeriod(days)

    def setTimeLastWeek (self):
        days = 7.0 
        self.setTimeLastPeriod(days)

    def setTimeLast2Weeks (self):
        days = 14.0
        self.setTimeLastPeriod(days)

    def setTimeLastPeriod (self, days):
        if len(self._channelNames) <= 0:
            return

        (range_start, end) = self._findDataTimeRange2(self._channelNames)
#        print "range_start = ", range_start
#        print "end =         ", end

        if end == '':
            ErrorDlg (root, "No data for specified channels in currently opened archive")
            return

        Pmw.showbusycursor()

        (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(end) 
        end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))

        start_time = end_time - (self._secsPerDay * days)

        self._graph.xaxis_configure(min=start_time, max=end_time)

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(start_time))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(end_time))
#        print "end_str   = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        self.logTimeSpan()

#        print 'new start = ', start_str
#        print 'new end   = ', end

        if self._retrievedDataStartDateTime != '':
            loaded_new_data = 0
            data_start_str = self._retrievedDataStartDateTime 
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_start_str) 
            data_start_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if start_time < data_start_time:
#                print "new start time < data start time"

                is_before_data = 1
                start = self._curStartDate[:4] + '/' + self._curStartDate[5:7] + '/' + self._curStartDate[8:10] + ' ' + self._curStartTime
                old_data_start = self._retrievedDataStartDateTime
                self._loadChannels(self._channelInfos, 0, start, old_data_start, is_before_data)

                self._retrievedDataStartDateTime = start
                loaded_new_data = 1

            data_end_str = self._retrievedDataEndDateTime
            (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(data_end_str) 
            data_end_time = time.mktime((Year, Month, Day, H, M, S, W, J, D))
            if end_time > data_end_time:
#                print "new end time > data end time"
                is_before_data = 0 
                now = time.time()
                if end_time > now:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))
                else:
                    end = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(end_time))

                self._loadChannels(self._channelInfos, 0, data_end_str, end, is_before_data)
                self._retrievedDataEndDateTime = end
                loaded_new_data = 1

            if loaded_new_data:
                self._plotChannels(self._channelInfos, 0, self._retrievedDataStartDateTime, self._retrievedDataEndDateTime)

        Pmw.hidebusycursor()

    def setYInterval (self):
        "Menu command"
        self.yIntervalDialog = Pmw.Dialog(self._yIntervalWidget,
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            title = 'Y Span',
            command = self.yIntervalExecute)
        self.yIntervalDialog.withdraw()

        (y0, y1) = self._graph.yaxis_limits()
        self._curYMin = "%g" % y0 
        self._curYMax = "%g" % y1 

        self.yMaxField = Pmw.EntryField(self.yIntervalDialog.interior(),
                labelpos = 'w',
                value = self._curYMax,
                label_text = 'Y Maximum',
                validate = {'validator' : 'real'})
        self.yMaxField.pack(side = 'top', expand = 1, fill = 'both')

        self.yMinField = Pmw.EntryField(self.yIntervalDialog.interior(),
                labelpos = 'w',
                value = self._curYMin,
                label_text = 'Y Minimum',
                validate = {'validator' : 'real'})
        self.yMinField.pack(side = 'top', expand = 1, fill = 'both')


        entries = (self.yMinField, self.yMaxField)
        Pmw.alignlabels(entries)

        self.yIntervalDialog.activate(geometry = 'centerscreenalways')

    def yIntervalExecute(self, result):
#        print 'You clicked on', result
        if result == 'OK':
            self._graph.yaxis_configure(min=self.yMinField.get(), max=self.yMaxField.get())

        self.yIntervalDialog.deactivate(result)

    def logChannelName(self, channelName):
        self.logEntry('channel_name = ' + channelName)

    def logUserName(self, userName):
        self.logEntry('user_name = ' + userName)

    def logTimeSpan(self):
        global _prevLogTimeSpanTime

        start = self._curStartDate[:4] + '/' + self._curStartDate[8:10] + '/' + self._curStartDate[5:7] + ' ' + self._curStartTime
        end = self._curEndDate[:4] + '/' + self._curEndDate[8:10] + '/' + self._curEndDate[5:7] + ' ' + self._curEndTime

        self.logEntry('start = ' + start + ' end = ' + end)

        now = time.time()
        _prevLogTimeSpanTime = now 

    def logTimeSpanCheck(self):
        global _prevLogTimeSpanTime
 
        now = time.time()
        call_interval = now - _prevLogTimeSpanTime
        if call_interval > self._log_time_span_interval:
            self.logTimeSpan()

    def logConfigLoad(self):
        self.logEntry('loaded configuration file = ' + self._loadConfigName)

    def logConfigSave(self):
        self.logEntry('saved configuration file = ' + self._loadConfigName)

    def logEntry(self, logString):
        if self._loggingDisabled:
            return

        now = time.time()
        cur_time = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))

        if self._logDir != '.':
            cur_month = int(cur_time[5:7])
            if cur_month == 1:
                month_str = 'January'
            elif cur_month == 2:
                month_str = 'February'
            elif cur_month == 3:
                month_str = 'March'
            elif cur_month == 4:
                month_str = 'April'
            elif cur_month == 5:
                month_str = 'May'
            elif cur_month == 6:
                month_str = 'June'
            elif cur_month == 7:
                month_str = 'July'
            elif cur_month == 8:
                month_str = 'August'
            elif cur_month == 9:
                month_str = 'September'
            elif cur_month == 10:
                month_str = 'October'
            elif cur_month == 11:
                month_str = 'November'
            elif cur_month == 12:
                month_str = 'December'
            else:
                month_str = 'January' # should never happen

            log_file = self._logDir + '/' + month_str + '/' + 'archive.log'
        else:
            log_file = self._logDir + '/' + 'archive.log'

        try:
            f = open(log_file, "a")
        except: 
            ErrorDlg (root, "Cannot open %s" % log_file)
            return 0

        f.write(cur_time + ' ' + logString + '\n')

        f.close()

    def quitSession(self):
        self.logTimeSpan()
        self.logEntry('*** End Session')
        self._root.quit()

    def __init__ (self, root, archiveName, loadConfigFileName, userName, channelName, loggingDisabled):
        # Instance variables
        self._root=root           # Root window
        self._next_color = 0      # for getColor
        self._rubberbanding = 0
        self._loggingDisabled = loggingDisabled
        self._channelInfos = []
        self._channelNames = []
        self._channelColors = []
        self._channelPoints = []
        self._channelTimes = []
        self._channelValues = []
        self._channelIsInfos = []
        self._totalPoints = 0
        self._secsPerDay = 86400.0 

        self._matlabExt = 'mat'
        self._postscriptExt = 'ps'

#        self._postscriptDir = "/afs/slac.stanford.edu/u/cd/rdh/epics/extensions/src/ChannelArchiver/casi/python/O.solaris"
#        self._archiveDir = "/afs/slac.stanford.edu/u/cd/rdh/epics/extensions/src/ChannelArchiver/Engine/Test/freq_directory" 
#        self._matlabDir = "/afs/slac.stanford.edu/u/cd/rdh/epics/extensions/src/ChannelArchiver/casi/python/O.solaris"
#        self._logDir = "/afs/slac.stanford.edu/u/cd/rdh/epics/extensions/src/ChannelArchiver/casi/python/O.solaris"
#        self._configDir = "/afs/slac.stanford.edu/u/cd/rdh/epics/extensions/src/ChannelArchiver/casi/python/O.solaris"

        try:
            self._postscriptDir = os.environ['ARBPSFILES']
        except:
            self._postscriptDir = '.' 

        try:
            self._archiveDir = os.environ['ARDATAFILES']
        except:
            self._archiveDir = '.' 

        try:
            self._matlabDir = os.environ['ARBMATLABFILES']
        except:
            self._matlabDir = '.'

        try:
            self._logDir = os.environ['ARBLOGFILES']
        except:
            self._logDir = '.'

        try:
            self._configDir = os.environ['ARBCONFIGFILES']
        except:
            self._configDir = '.' 

        try:
            self._configExt = os.environ['ARBCONFIGEXT']
        except:
            self._configExt = 'cfg' 

        self._channelLinewidths = []
        self._channelDashVars = []
        self._tempChannelDashVars = []
        self._channelDataSymbols = []
        self._channelSymbolFillVars = []
        self._tempChannelSymbolFillVars = []

        self._linewidths = ['1',
                            '2',
                            '3',
                            '4',
                            '5']

        self._data_symbols = ['None',
                              'square',
                              'circle',
                              'diamond',
                              'plus',
                              'cross',
                              'splus',
                              'scross',
                              'triangle']

        self._button_down_action_interval = 1
        self._log_time_span_interval = 30.0
        self._max_channels = 50
        for i in range(self._max_channels):
            self._channelLinewidths.append('1')
            self._channelDashVars.append(Tkinter.IntVar())
            self._channelDashVars[i].set(0)
            self._tempChannelDashVars.append(Tkinter.IntVar())
            self._tempChannelDashVars[i].set(0)
            self._channelDataSymbols.append('None')
            self._channelSymbolFillVars.append(Tkinter.IntVar())
            self._channelSymbolFillVars[i].set(0)
            self._tempChannelSymbolFillVars.append(Tkinter.IntVar())
            self._tempChannelSymbolFillVars[i].set(0)

#        self._colorUsed = [0, 0, 0, 0, 0, 0, 0, 0]
#        self._colors = ["#0000FF", "#FF0000", "#000000", "#00FFFF",
#                        "#00FF00", "#505050", "#FFFF00", "#FF00FF"]

#       Nine colors were chosen.  Any more and some colors seem to be hard
#       to distiguish from others.  The colors chosen were (in order):
#       blue, red, green, black, light blue, yellow, magenta, purple, tan.
        self._colorUsed = [0, 0, 0, 0, 0, 0, 0, 0, 0]
        self._colors = ["#0019FF", "#FF0000", "#33FF00", "#000000",
                        "#00CCFF", "#E5FF00", "#FF00B2", "#9800FF", "#FF6F00"]

        self._nonLeapYearMonthDays = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

        # Need to use long ints here because on the Macintosh the maximum size
        # of an integer is smaller than the value returned by time.time().
#        now = (long(time.time()) / 300) * 300

        now = time.time()
        yesterday = now - self._secsPerDay

        start_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(yesterday))
#        print "start_str = ", start_str

        self._curStartDate = start_str[:10]
        self._curStartTime = start_str[11:]

        self._retrievedDataStartDateTime = '' 

        self._plotStartDateTime = ''

        end_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(now))
#        print "end_str = ", end_str

        self._curEndDate = end_str[:10]
        self._curEndTime = end_str[11:]

        self._retrievedDataEndDateTime = '' 

        self._plotEndDateTime = ''

        self._curYMin = ''
        self._curYMax = '' 

        self._loadConfigName = ''
        self._plotTitle = 'Graph Title'

        self._yNoLogMin = '' 
        self._yNoLogMax = ''

        self._datePattern = re.compile(r"([0-9][0-9][0-9][0-9]/[0-9][0-9]/[0-9][0-9])")
        self._dateYBegin = re.compile(r"[0-9]+\sBeginText")
        self._whiteSpace = re.compile(r"\s")
        self._beginText = re.compile(r"BeginText")

        self._smallestValue = float(sys.maxint)
        self._largestValue = - float(sys.maxint) 

        self._curOpenArchive = ''
        self._curOpenArchiveDir = ''

#       Testing to produce list of colors
#        colorList = []
#        colorList = Pmw.Color.spectrum(8)
#        print "colorList = ", colorList

        self.logEntry('*** Start Session')
        if userName != '':
            self.logUserName(userName)

        # Init GUI
        menu = Pmw.MenuBar (root, hull_relief=RAISED, hull_borderwidth=1)
        menu.addmenu ('File', '')
        menu.addmenuitem ('File', 'command', '',
                          label='Open Archive...', command=self.openArchive)
        menu.addmenuitem ('File', 'command', '',
                          label='Load...', command=self.configLoad)
        menu.addmenuitem ('File', 'command', '',
                          label='Save As...', command=self.configSaveAs)
        menu.addmenuitem ('File', 'command', '',
                          label='Save', command=self.configSave)
        self._toPrinterWidget = menu.addmenuitem ('File', 'command', '',
                          label='Print To Printer...', command=self.printToPrinter)

        try:
            self._printer_environ_str = os.environ['EPICS_PR_LIST']
        except:
            self._printer_environ_str = '' 

        self._printer_list = self.parseEnvironStr(self._printer_environ_str)

        list_len = len(self._printer_list)
        if list_len > 0:
            self._last_printer_list_element = self._printer_list[list_len - 1]
        else:
            self._last_printer_list_element = '' 

#        self.toPrinter = Pmw.SelectionDialog(self._toPrinterWidget,
#            title = 'Printer Selection',
#            buttons = ('OK', 'Cancel'),
#            defaultbutton = 'OK',
#            scrolledlist_labelpos = 'nw',
#            label_text = 'Printer:',
#            scrolledlist_items = tuple(self._printer_list),
#            command = self.toPrinterExecute)
#        self.toPrinter.withdraw()

        self.toPrinter = Pmw.ComboBoxDialog(self._toPrinterWidget,
            title = 'Printer Selection',
            buttons = ('OK', 'Cancel'),
            defaultbutton = 'OK',
            combobox_labelpos = 'nw',
            label_text = 'Printer:',
            scrolledlist_items = tuple(self._printer_list),
            command = self.toPrinterExecute)
        self.toPrinter.withdraw()

        menu.addmenuitem ('File', 'command', '',
                          label='Print To File...', command=self.printToFile)
        menu.addmenuitem ('File', 'command', '',
                          label='Save Matlab As...', command=self.matlabSaveAs)
        menu.addmenuitem ('File', 'command', '',
                          label='Exit', command=self.quitSession)

        menu.addmenu ('Channel', '')
        self._addChannelsWidget = menu.addmenuitem ('Channel', 'command', '',
                          label='Add Channels...', command=self.addChannels)
        self._removeChannelsWidget = menu.addmenuitem ('Channel', 'command', '',
                          label='Remove Channels...', command=self.removeChannels)
        menu.addmenu ('Scales', '')
        self._timeWidget = menu.addmenuitem ('Scales', 'command', '',
                          label='Time Span...', command=self.setTimeSpan)

        menu.addcascademenu('Scales', 'Common Time Spans',
                'Set some other preferences', traverseSpec = 'z', tearoff = 1)

        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 2 hours', command=self.setTimeLast2Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 4 hours', command=self.setTimeLast4Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 8 hours', command=self.setTimeLast8Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 12 hours', command=self.setTimeLast12Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 24 hours', command=self.setTimeLast24Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 48 hours', command=self.setTimeLast48Hours)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 3 days', command=self.setTimeLast3Days)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 4 days', command=self.setTimeLast4Days)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last week', command=self.setTimeLastWeek)
        menu.addmenuitem('Common Time Spans', 'command', '',
                          label = 'Last 2 weeks', command=self.setTimeLast2Weeks)


        self._yIntervalWidget = menu.addmenuitem ('Scales', 'command', '',
                          label='Y Span...', command=self.setYInterval)

#        self._yLogVar = Tkinter.IntVar()
#        self._yLogVar.set(0)
#        self._yLogWidget = menu.addmenuitem ('Scales', 'checkbutton', '',
#                          label='Log Y Axis', command = self.toggleYLog,
#                          variable = self._yLogVar)

        menu.addmenu ('Options', '')
        self._titleWidget = menu.addmenuitem ('Options', 'command', '',
                          label='Plot Title...', command=self.setPlotTitle)
        self._curveStylesWidget = menu.addmenuitem ('Options', 'command', '',
                          label='Curve Styles...', command=self.setCurveStyles)

        menu.addmenu ('Help', '')
        menu.addmenuitem ('Help', 'command', '',
                          label='Browser Help', command=self.browserHelp)

        self._graph = Pmw.Blt.Graph (root)
        self._graph.configure(title=self._plotTitle)

	self._graph.xaxis_configure (command=self.timeLabel)

        start = self._curStartDate + ' ' + self._curStartTime
        (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(start) 
        x_min = time.mktime((Year, Month, Day, H, M, S, W, J, D))

        end = self._curEndDate + ' ' + self._curEndTime
        (Year, Month, Day, H, M, S, W, J, D) = self._YmdStrptime(end) 
        x_max = time.mktime((Year, Month, Day, H, M, S, W, J, D))

        self._graph.xaxis_configure(min=x_min, max=x_max)

        self._Xbuttons = Pmw.ButtonBox (root,
                                        labelpos = 'w',
                                        label_text = 'Time Axis:')

        self._panLeftButton = self._Xbuttons.add ('Pan Left')
        self._panRightButton = self._Xbuttons.add ('Pan Right')
        self._timeInButton = self._Xbuttons.add ('Zoom In')
        self._timeOutButton = self._Xbuttons.add ('Zoom Out')

        self._Ybuttons = Pmw.ButtonBox (root,
                                        labelpos = 'w',
                                        label_text = 'Y Axis:     ')

        self._Ybuttons.add ('Up', command=self._up)
        self._Ybuttons.add ('Down', command=self._down)
        self._Ybuttons.add ('Y In', command=self._yIn)
        self._Ybuttons.add ('Y Out', command=self._yOut)

        self._commandButtons = Pmw.ButtonBox (root)
        self._commandButtons.add ('Auto Scale', command=self._autoScale)
        self._commandButtons.add ('Full Time ', command=self._xAutoScale)
        self._commandButtons.add ('  Full Y  ', command=self._yAutoScale)

        self._messagebar = Pmw.MessageBar (root, 
                                           entry_width = 40,
                                           entry_relief='groove',
                                           labelpos = 'w',
                                           label_text = 'Info:')

        # Packing
        menu.pack             (side=TOP, fill=X)

        self._messagebar.pack (side=BOTTOM, fill=X)
        self._commandButtons.pack (side=BOTTOM, fill=X)
        self._Ybuttons.pack    (side=BOTTOM, fill=X)
        self._Xbuttons.pack    (side=BOTTOM, fill=X)
        self._graph.pack       (side=TOP, expand=YES, fill=BOTH)
        self._graph.configure(width=500)

        # Bindings
        self._graph.crosshairs_configure (dashes="1", hide=0,
                                         linewidth=1,
                                         color="lightblue")
        self._graph.bind("<ButtonPress-1>",   self._mouseDown)
        self._graph.bind("<ButtonRelease-1>", self._mouseUp)
        self._graph.bind("<Motion>", self._mouseMove)

        self._panLeftButton.bind("<ButtonPress-1>", self._panLeftPressed)
        self._panLeftButton.bind("<ButtonRelease-1>", self._panLeftReleased)
        self._panRightButton.bind("<ButtonPress-1>", self._panRightPressed)
        self._panRightButton.bind("<ButtonRelease-1>", self._panRightReleased)
        self._timeInButton.bind("<ButtonPress-1>", self._timeInPressed)
        self._timeInButton.bind("<ButtonRelease-1>", self._timeInReleased)
        self._timeOutButton.bind("<ButtonPress-1>", self._timeOutPressed)
        self._timeOutButton.bind("<ButtonRelease-1>", self._timeOutReleased)
   
        # Init archive
        self._archive = casi.archive ()
        self._channel = casi.channel ()
        self._value   = casi.value ()

#        if not self.openArchive ("/afs/slac.stanford.edu/u/cd/rdh/chanarch/test/freq_directory"):
        if not self.openArchive (archiveName):
            sys.exit (1)
            
        # Test Data
#        self.addChannels (('li31:asts:gp_01.val', 'li31:bpms:31:devt_x.val'))
#        self.addChannels (('fred', 'freddy'))

        if loadConfigFileName == '': 
            channels = []

#            channels.append('fred')
#            channels.append('freddy')
#            self.addChannels(tuple(channels))
        else:
            self._messagebar.message ('state', "Busy loading configuration file data.  Please be patient.")

            self.configLoad(loadConfigFileName)

            self._messagebar.message ('state', "Completed loading configuration file data.")

        if channelName != '':
#            print "channelName = ", channelName 

            channels = []
            channels.append(channelName)
            self.addChannels(tuple(channels))

        (self._axisStartTime, self._axisEndTime) = self._currentTimeRange()

        # run
        root.mainloop()

def usage ():
    print "USAGE:  casipython archiveBrowser.py [Options] [archive]"
    print " "
    print "Options:"
    print "        -channel <channel>             Specify channel name"
    print "        -file <configuration file>     Specify configuration file"
    print "        -user <user name>              Specify user name (for logging)"
    print "        -nolog                         Disables logging"
    print "        -help                          Request usage help" 

if __name__ == "__main__":
    argc = len(sys.argv)

    specified_archive_name = 0
    specified_config_file = 0
    specified_user_name = 0
    specified_channel_name = 0
    requested_help = 0
    logging_disabled = 0 

    i = 1
    while i < argc:
        if string.find(sys.argv[i], '-f') != -1:
            i = i + 1
            if i < argc:
                loadConfigFileName = sys.argv[i]
                specified_config_file = 1

        elif string.find(sys.argv[i], '-u') != -1:
            i = i + 1
            if i < argc:
                userName = sys.argv[i]
                specified_user_name = 1

        elif string.find(sys.argv[i], '-c') != -1:
            i = i + 1
            if i < argc:
                channelName = sys.argv[i]
                specified_channel_name = 1

        elif string.find(sys.argv[i], '-n') != -1:
            logging_disabled = 1

        elif string.find(sys.argv[i], '-h') != -1:
            requested_help = 1

        else:
            archiveName = sys.argv[i]
            specified_archive_name = 1

        i = i + 1

    if requested_help:
        usage()
        sys.exit()

    if not specified_archive_name:
        try:
            dataFilesDir = os.environ['ARDATAFILES']
        except:
            dataFilesDir = '.' 

        archiveName = dataFilesDir + "/multi_archive.txt"
      
    if not specified_config_file:
        loadConfigFileName = ''   

    if not specified_user_name:
        userName = ''

    if not specified_channel_name:
        channelName = ''

    root=Tk()
    Pmw.initialise()
    if not archiveName: sys.exit()
    Plot (root, archiveName, loadConfigFileName, userName, channelName, logging_disabled)
