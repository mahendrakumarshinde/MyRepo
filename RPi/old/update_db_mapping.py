import MySQLdb
import sys

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

if not len(sys.argv) == 2:
    raise Exception("Only 1 argument needed.")

db_name = sys.argv[1]
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
