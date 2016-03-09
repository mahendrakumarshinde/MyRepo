import MySQLdb
import sys

if not len(sys.argv) == 2:
    raise Exception('Only 1 argument required.')
db_name = sys.argv[1]

conn=MySQLdb.connect(host='173.194.252.192', db='mysql', user='infinite-uptime', passwd='infiniteuptime', charset='utf8')
x=conn.cursor()
x.execute("CREATE DATABASE %s" % db_name)

conn=MySQLdb.connect(host='173.194.252.192', db=db_name, user='infinite-uptime', passwd='infiniteuptime', charset='utf8')
x=conn.cursor()
#x.execute("SHOW TABLES")
#print x.fetchall()
x.execute("CREATE TABLE Feature_Dictionary(MAC_ADDRESS VARCHAR(255), Feature_ID INT(11), Feature_Name VARCHAR(255), Threshold_1 FLOAT, Threshold_2 FLOAT, Threshold_3 FLOAT, Status INT(11))")
x.execute("CREATE TABLE IU_device_data(Id INT(11), Timestamp_DB VARCHAR(45), Timestamp_Pi VARCHAR(45), MAC_ADDRESS VARCHAR(45), State INT(11), Battery_Level FLOAT, Feature_Value_0 FLOAT, Feature_Value_1 FLOAT, Feature_Value_2 FLOAT, Feature_Value_3 FLOAT, Feature_Value_4 FLOAT, Feature_Value_5 FLOAT)")
x.execute("CREATE TABLE Env_variables(reset_value INT(11))")
x.execute("INSERT INTO Env_variables(reset_value) VALUES ('0')")
conn.commit()

def getserial():
    cpuserial= "0000000000000000"
    try:
        f = open('/proc/cpuinfo', 'r')
        for line in f:
            if line[0:6]=='Serial':
                cpuserial = line[10:26]
        f.close()
    except:
        cpuserial="ERROR00000000000"
        raise
    return cpuserial

user_resp = raw_input("Change this RPi's mapping to the new database %s? y/n: " % db_name)

if user_resp == 'y' or user_resp == 'Y':
    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db="Hub_Dictionary")
    x=conn.cursor()
    x.execute("SHOW DATABASES")
    db_names = x.fetchall()
    db_names = zip(*db_names)[0]
    print db_names
    print db_name
    print getserial()
    if db_name not in db_names:
        raise Exception("You must first create that database.")
    x.execute("DELETE FROM Hub_Mapping WHERE Hub_ID='%s'" % getserial())
    x.execute("INSERT INTO Hub_Mapping (Hub_ID, Database_Name) VALUES (%s, %s)", (getserial(), db_name))
    conn.commit()