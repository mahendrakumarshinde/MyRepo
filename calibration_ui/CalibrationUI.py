#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 30 17:26:51 2017

@author: pat
"""

import time
import serial
import numpy as np
from collections import defaultdict

import tkinter as tk
import fpdf

#from tkinter import ttk
#import tkinter.messagebox
#import tkinter.colorchooser
#import tkinter.filedialog
#import tkinter.simpledialog
#import tkinter.commondialog
#import tkinter.font
#import tkinter.scrolledtext
#import tkinter.tix

"""
button.x['state'] = 'normal'
button.config(state="normal")
button.config(state='disabled')
"""

#==============================================================================
#
#==============================================================================

PORT = "/dev/ttyACM0"

def ensure_bytes(str_or_bytes):
    """
    Return bytes from either str or bytes
    """
    if isinstance(str_or_bytes, bytes):
        return str_or_bytes
    else:
        return str_or_bytes.encode()


class CalibrationData:
    
    data_meaning = ['mac_address',              # 0
                    'bat_id', 'bat_val',        # 1, 2
                    'feat0_id', 'feat0_val',    # 3, 4
                    'feat1_id', 'feat1_val',
                    'feat2_id', 'feat2_val',
                    'feat3_id', 'feat3_val',
                    'feat4_id', 'feat4_val',
                    'feat5_id', 'feat5_val',
                    'feat6_id', 'feat6_val',
                    'feat7_id', 'feat7_val',
                    'feat8_id', 'feat8_val',    # 19, 20
                    'timestamp']                # 21
    
    data_mapping = dict(mac_address = 0,
                        velocityX   = 4,
                        velocityY   = 6,
                        velocityZ   = 8,
                        freqX       = 14,
                        freqY       = 16,
                        freqZ       = 18,
                        rmsX        = 14,
                        rmsY        = 16,
                        rmsZ        = 18,
                        timestamp   = 21)
    
    def __init__(self, byte_data, delimiter=','):
        self.data = byte_data.decode().split(delimiter)
        if len(self.data) != len(self.data_meaning):
            raise ValueError('invalid data {}'.format(byte_data.decode()))
    
    def get(self, data_name):
        return self.data[self.data_mapping[data_name]]


class CalibrationExperiment:
    """
    Generic calibration experiment
    """
    
    fields = []
    verbose_names = []
    units = []
    _pretty_template = \
        """
            ----------------------------------------------------
              Expected {0}: {2} {1}
              Measured {0}: {3} {1}
              Error: {4} %
              Tolerance: +/-{5} %
              Result: {6}
            ----------------------------------------------------
         """
    
    def __init__(self, ref_values, tolerances):
        self.ref_values = ref_values
        self.tolerances = tolerances
        self.avg_values = None
        self.errors = None
        self.passed = None
        
    def calibrate(self, data_generator):
        """
        Get values from data generator updates the calibration results
        """
        values = defaultdict(list)
        for data_byte in data_generator:
            data = CalibrationData(data_byte)
            for field in self.fields:
                values[field].append(float(data.get(field)))
        self.avg_values = dict()
        self.errors = dict()
        self.passed = dict()
        for field, ref_val, tol in zip(self.fields,
                                       self.ref_values,
                                       self.tolerances):
            avg = np.mean(values[field])
            self.avg_values[field] = avg
            err = (avg - ref_val) / float(ref_val) * 100
            self.errors[field] = err
            self.passed[field] = abs(err) < tol
        
    def get_results(self):
        """
        Calibrations results consists of:
        - reference values
        - average measured values
        - errors in %
        - tolerances in %
        - calibration test passed or failed
        """
        if self.avg_values is None or self.errors is None or self.passed is None:
            return None
        return (self.ref_values, self.avg_values, self.errors, 
                self.tolerances, self.passed)
        
    def __str__(self):
        msg = str(self.__class__)[:-1]
        for val in self.ref_values:
            msg += ' ' + str(val)
        msg += '>'
        return msg
    
    def __repr__(self):
        subtitle = "Calibration conditions: "
        for ref_val, unit, name in zip(self.ref_values, self.units, 
                                       self.verbose_names):
            subtitle += name + ' = ' + str(ref_val) + unit + ' '
        return subtitle

    def get_pretty_message_template(self):
        """
        Genretor yielding pretty formatted calibration message, but empty
        
        These empty messages are intended to be use as placeholders for 
        calibration results, when the calibration have not been run yet.
        """
        for unit, name in zip(self.units, self.verbose_names):
            yield self._pretty_template.format(name, unit,
                                              '__', '__', '__', '__', '__')
        
    def get_pretty_results(self):
        """
        Genretor yielding pretty formatted calibration results for each field
        """
        for field, ref_val, unit, name in zip(self.fields,
                                              self.units, 
                                              self.ref_values,
                                              self.verbose_names):
            success = 'OK' if self.passed[field] else 'NOT OK'
            yield self._pretty_template.format(name,
                                               unit,
                                               ref_val,
                                               self.avg_values[field],
                                               self.errors[field], 
                                               self.tolerances[field],
                                               success)


class VibrationCalibration(CalibrationExperiment):
    fields = ['rmsZ', 'freqZ', 'velocityZ']
    verbose_names = ['Acceleration RMS', 'Frequency', 'Velocity RMS']
    units = ['mm/s2', 'Hz', 'mm/s']


class TemperatureCalibration(CalibrationExperiment):
    fields = ['temp']
    verbose_names = ['Temperature']
    units = ['deg C']


class AudioCalibration(CalibrationExperiment):
    fields = ['audioDB', 'freq']
    verbose_names = ['Audio Volume', 'Frequency']
    units = ['DB', 'Hz']


class SerialDataCollector:
    
    default_baud_rate = 115200
    default_parity = False
    
    def __init__(self, port, start_collection_command, end_collection_command,
                 start_confirm_msg, end_confirm_msg, **serial_settings):
        self.start_collection_command = ensure_bytes(start_collection_command)
        self.end_collection_command = ensure_bytes(end_collection_command)
        self.start_confirm_msg = ensure_bytes(start_confirm_msg)
        self.end_confirm_msg = ensure_bytes(end_confirm_msg)
        self.port = port
        self.baud_rate = serial_settings.get( 'baud_rate',
                                             self.default_baud_rate)
        self.parity = serial_settings.get('parity', self.default_parity)

    def collect_data(self, termination_byte=b';', timeout=0):
        if len(termination_byte) != 1:
            raise TypeError('termination_byte arg needs to be a single byte')
        if not isinstance(termination_byte, bytes):
            termination_byte = termination_byte.encode()
        ser = serial.Serial(self.port, self.baud_rate)
        ser.write(self.start_collection_command)
        time.sleep(.1)
        start_time = time.time()
        data = b''
        try:
            first_loop = True
            while True:
                if timeout and time.time() - start_time > timeout:
                    break
                new_byte = ser.read()
                if new_byte == b';':
                    if first_loop:
                        first_loop = False
                        data = data[:len(self.start_confirm_msg)]
                        start_time = time.time() # reset start_time
                        if not data == self.start_confirm_msg:
                            break #Return early if no confirmation
                    else:
                        yield data
                    data = b''
                else:
                    data += new_byte
        except KeyboardInterrupt:
            pass
        ser.write(self.end_collection_command)
        time.sleep(.1)
        ser.close()


class CalibrationInterface(tk.Frame):
    """
    
    
    12 columns and as many rows as necessary
    """
    default_title = 'Infinite Uptime Calibration Software'
    window_width = 1200
    window_height = 800
    frame_width = 144
    grid_col_count = 12
    column_width = int(frame_width / grid_col_count)
    
    start_calibration_cmd = b'IUCAL_START'
    end_calibration_cmd = b'IUCAL_END'
    data_termination_byte = b';'
    start_calibration_confirm = b'IUOK_START'
    end_calibration_confirm = b'IUOK_END'
    
    calibration_experiments = [
        VibrationCalibration([1000, 15.92, 10], [5, 5, 5]),
        VibrationCalibration([2000, 15.92, 20], [5, 5, 5]),
        VibrationCalibration([1000, 40, 4], [5, 5, 5]),
        VibrationCalibration([2000, 40, 8], [5, 5, 5]),
        VibrationCalibration([5000, 40, 20], [5, 5, 5]),
        VibrationCalibration([1000, 80, 2], [5, 5, 5]),
        VibrationCalibration([2000, 80, 4], [5, 5, 5]),
        VibrationCalibration([5000, 80, 10], [5, 5, 5]),
        ]
    
    def __init__(self, master, title='', **kwargs):
        # Create the frame along with the scrollbar
        self.root = master
        self.root.wm_geometry("{}x{}".format(self.window_width,
                                             self.window_height))
        self.title = title or self.default_title
        self.root.wm_title(self.title)
        kwargs.setdefault('relief', tk.GROOVE)
        kwargs.setdefault('bd', 1)
        self.outer_frame = tk.Frame(master, **kwargs)
        self.canvas = tk.Canvas(self.outer_frame)
        super().__init__(self.canvas)
        self.set_scrollbar()
        # Graphical variables
        self.labels = dict()
        self.text_vars = dict()
        self.text_fields = dict()
        self.buttons = dict()
        self.alerts = dict()
        self.calibration_section_names = []
        # Calibration variables
        self._data_collecter = None
        # show
        self.create_graphic_interface()
    
    # ===== Calibration methods =====
    
    @property
    def data_collecter(self):
        if not self.check_serial_port():
            return None
        port = self.get_user_input('port')
        if self._data_collecter is None or self._data_collecter.port != port:
            self._data_collecter = SerialDataCollector(
                                    port,
                                    self.start_calibration_cmd,
                                    self.end_calibration_cmd,
                                    self.start_calibration_confirm,
                                    self.end_calibration_confirm)
        return self._data_collecter
        
    def run_calibration(self, calibration_experiment, timeout=5.5):
        """
        Run a calibration experiment
        """
        data_gen = self.data_collecter.collect_data(
                                termination_byte=self.data_termination_byte,
                                timeout=timeout)
        calibration_experiment.calibrate(data_gen)
    
    # ===== Graphic interface methods =====
        
    def _scroll_function(self, event):
        self.canvas.configure(scrollregion = self.canvas.bbox("all"))
        
    def set_scrollbar(self):
        self.scrollbar = tk.Scrollbar(self.outer_frame, 
                                      orient="vertical",
                                      command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        self.outer_frame.pack(fill=tk.BOTH, expand=True)
        self.scrollbar.pack(side="right", fill="y")
        self.canvas.pack(side="left", fill=tk.BOTH, expand=True)
        self.canvas.create_window((0, 0),
                                  window=self,
                                  anchor='nw')
        self.bind("<Configure>", self._scroll_function)

    def get_user_input(self, field_name):
        text_var = self.text_vars.get(field_name)
        if text_var is None:
            return
        return text_var.get()
    
    def format_grid_width(self, row=None):
        label = tk.Label(self, width=self.column_width)
        label.grid(row=row, column=0)
        row = label.grid_info()['row']
        for i in range(1, self.grid_col_count):
            label = tk.Label(self, width=self.column_width)
            label.grid(row=row, column=i)
            
    def add_section_title(self, title, row=None):
        label = tk.Label(self, text=title)
        label.grid(row=row, column=0, columnspan=12, sticky=tk.W)
        
    def add_empty_row(self, row=None):
        label = tk.Label(self)
        label.grid(row=row, column=0, columnspan=12)
    
    def add_text_field(self, name, text='', row=None, column=0, columnspan=2,
                       sticky=tk.E+tk.W):
        text = text or name
        self.labels[name] = tk.Label(self, text=text)
        self.text_vars[name] = tk.StringVar()
        self.text_fields[name] = tk.Entry(self,
                                          textvariable=self.text_vars[name])
        self.labels[name].grid(row=row, column=column, columnspan=columnspan)
        current_row = self.labels[name].grid_info()['row']
        self.text_fields[name].grid(row=current_row,
                                    column=column + columnspan, 
                                    columnspan=columnspan,
                                    sticky=sticky)
        
    def add_button(self, name, text='', command=None, **kwargs):
        self.buttons[name] = tk.Button(self, text=text, command=command)
        self.buttons[name].grid(**kwargs)
        
    def add_calibration_section(self, calibration_experiment):
        section_name = str(calibration_experiment)
        self.calibration_section_names.append(section_name)
        self.add_section_title(repr(calibration_experiment))
        def callback():
            if not self.check_all_inputs():
                return
            self.run_calibration(calibration_experiment)
            for i, msg in enumerate(calibration_experiment.get_pretty_results()):
                label = self.labels[section_name + '_' + str(i)]
                label['text'] = msg
                field = calibration_experiment.fields[i]
                success = calibration_experiment.passed[field]
                label['bg'] = 'green' if success else 'red'
        self.add_button(section_name, text='Calibrate', command=callback)
        label = tk.Label(self, text='Results:')
        label.grid(column=0, columnspan=2)
        current_row = int(label.grid_info()['row']) + 1
        for i, msg in enumerate(calibration_experiment.get_pretty_message_template()):
            label = tk.Label(self, text=msg)
            label.grid(row=current_row, column=i * 3,
                       columnspan=3, sticky=tk.W+tk.E)
            self.labels[section_name + '_' + str(i)] = label
    
    def create_pop_up_alert(self, name, text, width=300, height=100):
        alert = tk.Toplevel(self)
        alert.wm_geometry("{}x{}".format(width, height))
        self.alerts[name] = alert
        empty_line = tk.Label(alert, text='')
        empty_line.pack()
        label = tk.Label(alert, text=text)
        label.pack()
        empty_line = tk.Label(alert, text='')
        empty_line.pack()
        button = tk.Button(alert, text='OK', command=alert.withdraw)
        button.pack()
        alert.withdraw()
        
    def check_all_inputs(self):
        """
        Return True if all the inputs fields have been filled, else False
        """
        if not self.get_user_input('port'):
            self.alerts['no_port'].deiconify()
            return False
        if not self.get_user_input('mac_address'):
            self.alerts['no_mac_address'].deiconify()
            return False
        if not self.get_user_input('operator_name'):
            self.alerts['no_operator_name'].deiconify()
            return False
        if not self.get_user_input('report_number'):
            self.alerts['no_report_number'].deiconify()
            return False
        return True
    
    def check_serial_port(self):
        """
        Return True if the device is available at given port, else False
        
        
        """
        try:
            ser = serial.Serial(self.get_user_input('port'))
            ser.close()
        except serial.SerialException as e:
            if e.args[0] == 2:
                self.alerts['wrong_port'].deiconify()
                return False
            else:
                raise e
        return True
                
    def check_mac_address(self):
        """
        Return True if the mac_address correspond to the device, else False
        """
        data_gen = self.data_collecter.collect_data(
                                termination_byte=self.data_termination_byte,
                                timeout=2)
        data = CalibrationData(next(data_gen))
        data_gen.close()
        mac_address = data.get('mac_address')
        if mac_address != self.get_user_input('mac_address'):
            self.alerts['wrong_mac_address'].deiconify()
            return False
        return True
                
    
    def create_graphic_interface(self):
        """
        """
        
        self.format_grid_width()

        # Setup Section
        self.add_section_title('Calibration setup')
        self.add_text_field('port', text='COM Port (eg. COM7)')
        self.add_text_field('mac_address', text='MAC Address')
        self.add_text_field('operator_name', text='Calibrated By')
        self.add_text_field('report_number', text='Report #')
        
        # Calibration Sections
        self.add_empty_row()
        for experiment in self.calibration_experiments:
            self.add_calibration_section(experiment)
            self.add_empty_row()
        
        # Print and quit
        self.add_empty_row()
        self.add_button('print', text='Print report', 
                        command=self.print_report, column=0)
        self.add_button('quit', text='Quit', 
                        command=self.root.quit, column=12, sticky=tk.W)
        
        #Alerts generation
        self.create_pop_up_alert('no_port',
            'Please input a port')
        self.create_pop_up_alert('wrong_port',
            'IDE / IDE+ is not available at specified port')
        self.create_pop_up_alert('no_mac_address',
            'Please input a MAC Address')
        self.create_pop_up_alert('wrong_mac_address',
            "Specified MAC Address doesn't match the device's MAC Address")
        self.create_pop_up_alert('no_operator_name',
            "Please input the operator name")
        self.create_pop_up_alert('no_report_number',
            "Please input the report number")
        
    def print_report(self):
        pass



if __name__ == '__main__':
    root = tk.Tk()
    app = CalibrationInterface(root)
    
    app.mainloop()
    root.destroy()

#test_collector = SerialDataCollector(PORT,
#                                     CalibrationInterface.start_calibration_cmd,
#                                     CalibrationInterface.end_calibration_cmd,
#                                     CalibrationInterface.start_calibration_confirm,
#                                     CalibrationInterface.end_calibration_confirm)