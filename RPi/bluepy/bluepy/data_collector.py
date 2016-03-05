import RPi.GPIO as g
import btle
import time
import datetime
import MySQLdb
import sys

conn = MySQLdb.connect(host="gator4189.hostgator.com",user="ycchen", passwd="g6n,qMi4aocO", db="ycchen_productivity_database")
x = conn.cursor()
#x.execute("SELECT VERSION()")
#data = x.fetchone()
sql = "SELECT * FROM IU_device_data WHERE State = '%d'" %(2)
#table = "IU_device_data"
#try:
x.execute(sql)
result=x.fetchone()
print(result)
#except:
 #   print("error, unable to fetch data")