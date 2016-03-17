import MySQLdb
import sys

def sql_connect(db_name):
    return MySQLdb.connect(host='173.194.252.192', db=db_name, user='infinite-uptime', passwd='infiniteuptime', charset='utf8')
try:
    master_conn=sql_connect('mysql')
    master_x=master_conn.cursor()
except:
    print("SQL connection failed. Check wireless connection. ")
    exit(1)

def prompt():
    sys.stdout.write('> ')

def show_databases():
    print("\n")
    print("Current databases: ")
    print("==============")
    master_x.execute("SHOW DATABASES")
    for db in master_x.fetchall():
        print(db[0])
    print("==============\n")

def show_tables(db_name=None):
    if db_name is None:
        db_name = raw_input("Enter a database to examine: ")
    conn=sql_connect(db_name)
    x=conn.cursor()
    x.execute("SHOW TABLES")
    print("\nTables:\n==============")
    for table in x.fetchall():
        print(table[0])
    print("==============\n")


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

def create_database():
    show_databases()
    db_name = raw_input("Enter the name of the database to create: ")
    master_x.execute("CREATE DATABASE %s" % db_name)
    conn=sql_connect(db_name)
    x=conn.cursor()
    x.execute("CREATE TABLE Feature_Dictionary(MAC_ADDRESS VARCHAR(255), Feature_ID INT(11), Feature_Name VARCHAR(255), Threshold_1 FLOAT, Threshold_2 FLOAT, Threshold_3 FLOAT, Status INT(11))")
    x.execute("CREATE TABLE IU_device_data(Id INT(11), Timestamp_DB VARCHAR(45), Timestamp_Pi VARCHAR(45), MAC_ADDRESS VARCHAR(45), State INT(11), Battery_Level FLOAT, Feature_Value_0 FLOAT, Feature_Value_1 FLOAT, Feature_Value_2 FLOAT, Feature_Value_3 FLOAT, Feature_Value_4 FLOAT, Feature_Value_5 FLOAT)")
    x.execute("CREATE TABLE Env_variables(reset_value INT(11))")
    x.execute("INSERT INTO Env_variables(reset_value) VALUES ('0')")
    conn.commit()
    print("Tables created: ")
    show_tables(db_name)

    user_resp = raw_input("Change this RPi's mapping to the new database %s? y/n: " % db_name)
    if user_resp == 'y' or user_resp == 'Y':
        update_mapping(db_name)
    else:
        print("No change to mapping. ")

def update_mapping(db_name=None):
    if db_name is None:
        show_databases()
        db_name = raw_input("Enter a database to switch this Pi to: ")
    conn=sql_connect("Hub_Dictionary")
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
    print("Mapping changed to: " + db_name)

def remove_database():
    show_databases()
    db_name=raw_input("Enter a database to remove: ")
    resp=raw_input("Are you sure you would like to remove '%s'? y/n: " % db_name)
    if (resp == 'Y' or resp == 'y'):
        master_x.execute("DROP DATABASE %s" % db_name)
        print("Database removed. ")
    else:
        print("No database removed. ")

def add_user():
    show_databases()
    db_name=raw_input("Enter the database that the user is being added to: ")
    user=raw_input("Username to add: ")
    pwd=raw_input("Password: ")
    conn=sql_connect("InfiniteUptime")
    x=conn.cursor()
    x.execute("INSERT INTO UserInformation (user_name, password, dbname) VALUES ('%s', '%s', '%s')" % (user, pwd, db_name))
    conn.commit()
    print("New user added.")

def inspect_table():
    show_databases()
    db_name=raw_input("Enter a database to look in: ")
    show_tables(db_name)
    table_name=raw_input("Enter a table to inspect: ")
    conn=sql_connect(db_name)
    x=conn.cursor()

    x.execute("DESC %s" % table_name)

    print("\nTable: %s" % table_name)

    formatted_table(x)

    x.execute("SELECT * FROM %s" % table_name)

    print("\nValues: ")

    formatted_table(x)

def formatted_table(x):
    results = x.fetchall()
    widths = []
    columns = []
    tavnit = '|'
    separator = '+'

    for cd in x.description:
        widths.append(max(cd[2], len(cd[0])))
        columns.append(cd[0])

    for w in widths:
        tavnit += " %-"+"%ss |" % (w,)
        separator += '-'*w + '--+'

    print(separator)
    print(tavnit % tuple(columns))
    print(separator)
    for row in results:
        print(tavnit % row)
    print(separator)

def clear_data():
    show_databases()
    db_name=raw_input("Enter a database to clear data from: ")
    resp=raw_input("Clearing IU device data from %s. Are you sure? y/n: " % db_name)
    if resp == 'y' or resp == 'Y':
        conn=sql_connect(db_name)
        x=conn.cursor()
        x.execute("TRUNCATE TABLE IU_device_data")
        print("Data cleared. ")
    else:
        print("No data cleared. ")


help_string = "\nType a number to execute a SQL command:\n" \
            + "1. Create database\n" \
            + "2. Update this RPi's mapping\n" \
            + "3. Add user credential for database\n" \
            + "4. Delete database\n" \
            + "5. Show databases\n" \
            + "6. Show tables\n" \
            + "7. Inspect table\n" \
            + "8. Clear IU device data\n"

function_mapping={
    1: create_database,
    2: update_mapping,
    3: add_user,
    4: remove_database,
    5: show_databases,
    6: show_tables,
    7: inspect_table,
    8: clear_data
}

if __name__ == '__main__':
    print(help_string)
    prompt()
    try:
        inp = int(raw_input())
        fn = function_mapping[inp]
    except:
        print("Error: command not properly specified. Enter a number 1-6.\n")
        exit(1)
    fn()
    print("\n")
