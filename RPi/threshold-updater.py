import MySQLdb
#import pdb

#pdb.set_trace()
MAC_ADDRESSES=["68:9E:19:07:F2:47"]

thresholds = [(0, "Signal Energy", 30, 600, 1200), (1, "Mean Crossing Rate", 100, 500, 1000), (2, "Spectral Centroid", 6, 7, 8), (3, "Spectral Flatness", 40, 40, 45), (4, "Accel Spectral Spread", 200, 205, 210), (5, "Audio Spectral Spread", -60, -45, -30)]

conn = MySQLdb.connect(host="gator4189.hostgator.com", user="ycchen", passwd="g6n,qMi4aocO", db="ycchen_productivity_database")
x=conn.cursor()
#x.execute("""DELETE FROM `Feature_dictionary` WHERE 1""")

for address in MAC_ADDRESSES:
    for thresh in thresholds:
        command = """INSERT INTO Feature_dictionary (`MAC_ADDRESS`, `Feature_ID`, `Feature_Name`, `Threshold_1_actual`, `Threshold_2_actual`, `Threshold_3_actual`) VALUES (%s, %s, %s, %s, %s, %s)"""
        x.execute(command, (address, thresh[0], thresh[1], thresh[2], thresh[3], thresh[4]))
