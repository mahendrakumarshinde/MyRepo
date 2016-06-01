import datetime
import time


time_tuple = datetime.datetime.now().timetuple()
int_data = time.mktime(time_tuple)

print(str(time_tuple))
print(str(int_data))

timestamp_string = time.gmtime(int_data)
timestamp2 = time.strftime("%Y-%m-%d %H:%M:%S", timestamp_string)
print(str(timestamp2))