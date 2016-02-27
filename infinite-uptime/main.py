#!/usr/bin/env python
#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import cgi
import webapp2
from google.appengine.ext.webapp.util import run_wsgi_app

import MySQLdb
import os
import jinja2

#Configure the Jinja2 environment.
JINJA_ENVIRONMENT = jinja2.Environment(
	loader=jinja2.FileSystemLoader(os.path.dirname(__file__)),
	autoescape=True,
	extensions=['jinja2.ext.autoescape'])

#Define Cloud SQL instance information.
_INSTANCE_NAME = 'infinite-uptime-1232:server0'

class MainHandler(webapp2.RequestHandler):
    def get(self):
    	#Display existing tables
    	if (os.getenv('SERVER_SOFTWARE') and
    		os.getenv('SERVER_SOFTWARE').startswith('Google App Engine/')):
    		db = MySQLdb.connect(unix_socket='/cloudsql/' + _INSTANCE_NAME, db='InfiniteUptime', user='root', charset='utf8')
    	else:
    		db = MySQLdb.connect(host='173.194.252.192', port=3306, db='InfiniteUptime', user='infinite-uptime', passwd='infiniteuptime', charset='utf8')

    	cursor = db.cursor()
    	cursor.execute('SELECT Timestamp_DB, Timestamp_Pi, MAC_ADDRESS, State, Battery_Level, Feature_Value_0, Feature_Value_1, Feature_Value_2, Feature_Value_3, Feature_Value_4, Feature_Value_5 FROM IU_device_data')

    	IU_headers = ['Timestamp_DB','Timestamp_Pi','MAC_ADDRESS','State','Battery_Level','Feature_Value_0','Feature_Value_1','Feature_Value_2','Feature_Value_3','Feature_Value_4','Feature_Value_5']
    	IU_entries = []
    	for row in cursor.fetchall():
    		IU_entries.append(dict([('Timestamp_DB',row[0]),
    								('Timestamp_Pi',row[1]),
    								('MAC_ADDRESS',row[2]),
    								('State',row[3]),
    								('Battery_Level',row[4]),
                                    ('Feature_Value_0',row[5]),
    								('Feature_Value_1',row[6]),
    								('Feature_Value_2',row[7]),
    								('Feature_Value_3',row[8]),
    								('Feature_Value_4',row[9]),
    								('Feature_Value_5',row[10])]))
    	
    	cursor.execute('SELECT Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, MAC_ADDRESS FROM Feature_Dictionary')
    	Feature_headers = ['MAC_ADDRESS', 'Feature_ID','Feature_Name','Threshold_1','Threshold_2','Threshold_3']
    	Feature_entries = []
    	for row in cursor.fetchall():
    		Feature_entries.append(dict([('Feature_ID',row[0]),
    									 ('Feature_Name',row[1]),
    									 ('Threshold_1',row[2]),
    									 ('Threshold_2',row[3]),
    									 ('Threshold_3',row[4]),
                                         ('MAC_ADDRESS',row[5])]))
    	variables = {'IU_headers': IU_headers, 'IU_entries': IU_entries, 'Feature_headers': Feature_headers, 'Feature_entries': Feature_entries}
    	template = JINJA_ENVIRONMENT.get_template('main.html')
    	self.response.write(template.render(variables))
    	db.close()

app = webapp2.WSGIApplication([
    ('/', MainHandler)
], debug=True)
