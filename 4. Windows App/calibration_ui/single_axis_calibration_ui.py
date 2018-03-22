#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 30 17:26:51 2017
@author: pat
"""

import time
import serial
import numpy as np
import datetime as dt
from collections import defaultdict, namedtuple

import tkinter as tk
from tkinter import font, messagebox
from tkinter.filedialog import asksaveasfilename
import fpdf


# =============================================================================
# Data collection and calibration
# =============================================================================

def ensure_bytes(str_or_bytes):
    """
    Return bytes from either str or bytes
    """
    if isinstance(str_or_bytes, bytes):
        return str_or_bytes
    else:
        return str_or_bytes.encode()


class CalibrationData:
    """Calibration data."""

    data_meaning = ['mac_address',              # 0
                    'bat_id', 'bat_val',        # 1, 2
                    'feat0_id', 'feat0_val',    # 3, 4 => velocity X
                    'feat1_id', 'feat1_val',
                    'feat2_id', 'feat2_val',
                    'feat3_id', 'feat3_val',    # 9, 10 => Temperature
                    'feat4_id', 'feat4_val',    # 11, 12 => Freq X
                    'feat5_id', 'feat5_val',
                    'feat6_id', 'feat6_val',
                    'feat7_id', 'feat7_val',    # 17, 18 => RMS X
                    'feat8_id', 'feat8_val',
                    'feat9_id', 'feat9_val',    # 21, 22
                    'timestamp']                # 23

    data_mapping = {'mac_address': 0,
                    'velocityX':   4,
                    'velocityY':   6,
                    'velocityZ':   8,
                    'temperature': 10,
                    'freqX':       12,
                    'freqY':       14,
                    'freqZ':       16,
                    'rmsX':        18,
                    'rmsY':        20,
                    'rmsZ':        22,
                    'timestamp':   23}

    def __init__(self, byte_data, delimiter=','):
        self.data = byte_data.decode().split(delimiter)
        if len(self.data) != len(self.data_meaning):
            raise ValueError('Invalid data {}'.format(byte_data.decode()))

    def get(self, data_name):
        return self.data[self.data_mapping[data_name]]


class CalibrationExperiment:
    """
    Generic calibration experiment
    """
    fields = []
    verbose_names = []
    units = []
    unit_conversion_factors = []
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
        self.reset()

    def reset(self):
        self.avg_values = dict.fromkeys(self.fields, 0)
        self.errors = dict.fromkeys(self.fields, 100)
        self.passed = dict.fromkeys(self.fields, False)
        self.done = False

    def calibrate(self, data_generator):
        """
        Get values from data generator updates the calibration results
        """
        values = defaultdict(list)
        for i, data_byte in enumerate(data_generator):
            if i == 0:
                continue
            data = CalibrationData(data_byte)
            for field, factor in zip(self.fields,
                                     self.unit_conversion_factors):
                values[field].append(factor * float(data.get(field)))
        for field, ref_val, tol in zip(self.fields,
                                       self.ref_values,
                                       self.tolerances):
            avg = np.mean(values[field])
            self.avg_values[field] = avg
            err = (avg - ref_val) / float(ref_val) * 100
            self.errors[field] = err
            self.passed[field] = abs(err) < tol
        self.done = True

    def get_results(self):
        """
        Calibrations results consists of:
        - reference values
        - average measured values
        - errors in %
        - tolerances in %
        - calibration test passed or failed
        """
        if any([self.avg_values is None, self.errors is None,
                self.passed is None]):
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
        Genretor yielding pretty formatted calibration message, but empty.
        These empty messages are intended to be use as placeholders for
        calibration results, when the calibration have not been run yet.
        """
        for ref_val, unit, name, tol in zip(self.units,
                                            self.ref_values,
                                            self.verbose_names,
                                            self.tolerances):
            yield self._pretty_template.format(
                    name, ref_val, unit, '__', '__', tol, '__')

    def get_pretty_results(self):
        """
        Genretor yielding pretty formatted calibration results for each field
        """
        for field, ref_val, unit, name, tol in zip(self.fields,
                                                   self.units,
                                                   self.ref_values,
                                                   self.verbose_names,
                                                   self.tolerances):
            success = 'OK' if self.passed[field] else 'NOT OK'
            yield self._pretty_template.format(
                    name, ref_val, unit, round(self.avg_values[field], 2),
                    round(self.errors[field], 2), tol, success)


class VibrationCalibration(CalibrationExperiment):
    fields = ['rmsZ', 'freqZ', 'velocityZ']
    verbose_names = ['Accel. RMS', 'Frequency', 'Velocity RMS']
    units = ['mm/s2', 'Hz', 'mm/s']
    unit_conversion_factors = [1000, 1, 1]


class TemperatureCalibration(CalibrationExperiment):
    fields = ['temp']
    verbose_names = ['Temperature']
    units = ['deg C']
    unit_conversion_factors = [1]


class AudioCalibration(CalibrationExperiment):
    fields = ['audioDB', 'freq']
    verbose_names = ['Audio Volume', 'Frequency']
    units = ['DB', 'Hz']
    unit_conversion_factors = [1, 1]


class SerialDataCollector:
    """Serial Data Collector."""

    default_baud_rate = 115200
    default_parity = False

    def __init__(self, port, start_collection_command, end_collection_command,
                 start_confirm_msg, end_confirm_msg, **serial_settings):
        self.start_collection_command = ensure_bytes(start_collection_command)
        self.end_collection_command = ensure_bytes(end_collection_command)
        self.start_confirm_msg = ensure_bytes(start_confirm_msg)
        self.end_confirm_msg = ensure_bytes(end_confirm_msg)
        self.port = port
        self.baud_rate = serial_settings.get('baud_rate',
                                             self.default_baud_rate)
        self.parity = serial_settings.get('parity', self.default_parity)

    def collect_data(self, termination_byte=b';', timeout=0):
        """
        Generator that collect data from device.
        It sends a signal to the device to enter calibration mode, and exit it
        at the end of collection.
        Data collection duration is timeout seconds.
        """
        if len(termination_byte) != 1:
            raise TypeError('termination_byte arg needs to be a single byte')
        if not isinstance(termination_byte, bytes):
            termination_byte = termination_byte.encode()
        ser = serial.Serial(self.port, self.baud_rate, timeout=timeout)
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
                        start_time = time.time()  # Reset start_time
                        if not data == self.start_confirm_msg:
                            break  # Return early if no confirmation
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
    Calibration Interface.
    12 columns and as many rows as necessary.
    """
    default_title = 'Infinite Uptime Calibration Software'
    window_width = 1200
    window_height = 800
    frame_width = 144
    grid_col_count = 12
    column_width = int(frame_width / grid_col_count)

    start_calibration_cmd = b'IUCAL_START\n'
    end_calibration_cmd = b'IUCAL_END\n'
    data_termination_byte = b';'
    start_calibration_confirm = b'IUOK_START'
    end_calibration_confirm = b'IUOK_END'

    calibration_experiments = [
        VibrationCalibration([1000, 15.92, 10], [5, 5, 5]),
        VibrationCalibration([2000, 15.92, 20], [5, 5, 5]),
        VibrationCalibration([1000, 40, 4], [5, 5, 5]),
        VibrationCalibration([2000, 40, 8], [5, 5, 5]),
#        VibrationCalibration([5000, 40, 20], [5, 5, 5]),
        VibrationCalibration([2000, 80, 4], [5, 5, 5]),
        VibrationCalibration([5000, 80, 10], [5, 5, 5]),
        VibrationCalibration([10000, 80, 20], [5, 5, 5]),
        ]

    # Additionnal Calibration Information
    accel_range = 4
    accel_precision = 12
    least_count = (2 * accel_range) / pow(2, accel_precision)
    uncertainty = 95
    manufacturer = 'Infinite Uptime'

    def __init__(self, master, title='', **kwargs):
        # Create the frame along with the scrollbar
        self.root = master
        self.center_window(self.root, self.window_width, self.window_height)
        self.title = title or self.default_title
        self.root.wm_title(self.title)
        kwargs.setdefault('relief', tk.GROOVE)
        kwargs.setdefault('bd', 1)
        self.outer_frame = tk.Frame(master, **kwargs)
        self.canvas = tk.Canvas(self.outer_frame)
        super().__init__(self.canvas)
        self.set_scrollbar()
        # Graphical variables
        self._input_validated = False
        self.labels = dict()
        self.text_vars = dict()
        self.text_fields = dict()
        self.buttons = dict()
        self.alerts = dict()
        # Calibration variables
        self._data_collecter = None
        self.current_temperature = 0
        # Create fonts
        default_font = font.nametofont("TkDefaultFont")
        default_font.configure(size=11)
        bold_font = default_font.copy()
        bold_font.configure(weight='bold')
        title_font = font.Font(family='Arial', size=20, weight='bold')
        title_font.configure(underline=True)
        self.fonts = dict(
            title=title_font,
            section_title=font.Font(family='Arial', size=16, weight='bold'),
            button=font.Font(family='Arial', size=12, weight='bold'),
            alert=font.Font(family='Arial', size=12, weight='bold'),
            default=default_font,
            bold=bold_font)
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

    def run_calibration(self, calibration_experiment, timeout=6):
        """
        Run a calibration experiment
        """
        data_gen = self.data_collecter.collect_data(
                                termination_byte=self.data_termination_byte,
                                timeout=timeout)
        calibration_experiment.calibrate(data_gen)

    def get_all_calibration_from_type(self, calibration_type):
        """
        Return all the listed calibration experiment of given type
        """
        calibration_experiments = []
        for exp in self.calibration_experiments:
            if isinstance(exp, calibration_type):
                calibration_experiments.append(exp)
        return calibration_experiments

    # ===== Graphic interface methods =====

    def center_window(self, frame, width, heigth):
        frame.update_idletasks()
        w = frame.winfo_screenwidth()
        h = frame.winfo_screenheight()
        x = int(w/2 - width / 2)
        y = int(h/2 - heigth / 2)
        frame.geometry("{}x{}+{}+{}".format(width, heigth, x, y))

    def _scroll_function(self, event):
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))

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
        self.canvas.bind(
                '<4>',
                lambda event: self.canvas.yview('scroll', -1, 'units'))
        self.canvas.bind(
                '<5>',
                lambda event: self.canvas.yview('scroll', 1, 'units'))
        self.bind('<4>',
                  lambda event: self.canvas.yview('scroll', -1, 'units'))
        self.bind('<5>',
                  lambda event: self.canvas.yview('scroll', 1, 'units'))

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

    def add_title(self, title, row=None):
        self.add_empty_row()
        label = tk.Label(self, text=title, font=self.fonts['title'])
        label.grid(row=row, column=0, columnspan=self.grid_col_count,
                   sticky=tk.W+tk.E)
        label.config(fg='#4DABC1')
        self.add_empty_row()

    def add_section_title(self, title, row=None):
        self.add_empty_row()
        label = tk.Label(self, text=title, font=self.fonts['section_title'])
        label.grid(row=row, column=0, columnspan=self.grid_col_count,
                   sticky=tk.W+tk.E)
        self.add_empty_row()

    def add_empty_row(self, row=None):
        label = tk.Label(self)
        label.grid(row=row, column=0, columnspan=self.grid_col_count)

    def add_text_field(self, name, text='', row=None, column=1, columnspan=2,
                       sticky=tk.E+tk.W, default='', disabled=False):
        text = text or name
        self.labels[name] = tk.Label(self, text=text)
        self.text_vars[name] = tk.StringVar()
        self.text_fields[name] = tk.Entry(
                self, textvariable=self.text_vars[name],
                font=self.fonts['default'], justify='center')
        if default:
            self.text_fields[name].insert(tk.END, default)
        if disabled:
            self.text_fields[name].config(state='disabled')
        self.labels[name].grid(row=row, column=column, columnspan=columnspan)
        current_row = self.labels[name].grid_info()['row']
        self.text_fields[name].grid(row=current_row,
                                    column=column + columnspan,
                                    columnspan=columnspan,
                                    sticky=sticky)

    def add_button(self, name, text='', command=None, disabled=False,
                   **kwargs):
        kwargs.setdefault('column', 1)
        self.buttons[name] = tk.Button(self, text=text, command=command,
                                       font=self.fonts['button'])
        self.buttons[name].grid(**kwargs)
        if disabled:
            self.buttons[name].config(state='disabled')

    def add_calibration_section(self, section_number, calibration_experiment):
        section_name = str(calibration_experiment)
        self.add_section_title("{}. {}".format(section_number,
                                               repr(calibration_experiment)))

        def callback():
            self.run_calibration(calibration_experiment)
            for i, msg in enumerate(
                    calibration_experiment.get_pretty_results()):
                label = self.labels[section_name + '_' + str(i)]
                label['text'] = msg
                field = calibration_experiment.fields[i]
                success = calibration_experiment.passed[field]
                label['bg'] = 'green' if success else 'red'

        self.add_button(section_name, text='Calibrate', command=callback,
                        disabled=True)
        label = tk.Label(self, text='Results:', font=self.fonts['bold'])
        label.grid(column=1, columnspan=2)
        current_row = int(label.grid_info()['row']) + 1
        for i, msg in enumerate(
                calibration_experiment.get_pretty_message_template()):
            label = tk.Label(self, text=msg)
            label.grid(row=current_row, column=1 + i * 3,
                       columnspan=3, sticky=tk.W+tk.E)
            self.labels[section_name + '_' + str(i)] = label

    def reset_calibrations(self):
        result = messagebox.askquestion("Confirmation", "Reset calibration?",
                                        icon='warning')
        if not result == 'yes':
            return
        for exp in self.calibration_experiments:
            exp.reset()
            exp_name = str(exp)
            for i, msg in enumerate(exp.get_pretty_message_template()):
                label = self.labels[exp_name + '_' + str(i)]
                label.config(text=msg)
                label['bg'] = '#d9d9d9'

    def create_pop_up_alert(self, name, text, width=450, height=110):
        alert = tk.Toplevel(self)
        self.center_window(alert, width, height)
        self.alerts[name] = alert
        empty_line = tk.Label(alert, text='')
        empty_line.pack()
        label = tk.Label(alert, text=text, font=self.fonts['alert'])
        label.pack()
        empty_line = tk.Label(alert, text='')
        empty_line.pack()
        button = tk.Button(alert, text='OK', command=alert.withdraw)
        button.pack()
        alert.withdraw()

    def check_inputs_not_empty(self):
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
        if not self.get_user_input('certificate_number'):
            self.alerts['no_certificate_number'].deiconify()
            return False
        if not self.get_user_input('accel_range'):
            self.alerts['no_accel_range'].deiconify()
            return False
        return True

    def check_serial_port(self):
        """
        Return True if the device is available at given port, else False.
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

    def check_device_response(self):
        """
        Return True if the mac_address correspond to the device, else False.
        Get the device temperature at the same time
        """
        data_gen = self.data_collecter.collect_data(
                                termination_byte=self.data_termination_byte,
                                timeout=1.5)
        for data in data_gen:
            # wait for data_gen to empty so that we close properly \
            # connection to serial
            pass
        try:
            data = CalibrationData(data)
        except KeyError as e: #(ValueError, UnboundLocalError) as e:
            invalid_data_msg = 'Invalid data'
            if isinstance(e, ValueError) and \
                    e.args[0][:len(invalid_data_msg)] != invalid_data_msg:
                raise e
            self.alerts['invalid_data'].deiconify()
            return
        mac_address = data.get('mac_address')
        # TODO get actual temperature once its calibrated
        self.current_temperature = 28  # data.get('temperature')
        if mac_address != self.get_user_input('mac_address'):
            self.alerts['wrong_mac_address'].deiconify()
            return False
        return True

    def validate_inputs(self):
        if self._input_validated:
            # Reset inputs
            self.reset_calibrations()
            self.set_entry_states('normal')
            self.set_calibration_button_states('disabled')
            self.buttons['input_validation'].config(text='Validate')
            self._input_validated = False
        else:
            if not self.check_inputs_not_empty():
                return
            if not self.check_serial_port():
                return
            if not self.check_device_response():
                return
            self.set_entry_states('disabled')
            self.set_calibration_button_states('normal')
            self.buttons['input_validation'].config(text='Change inputs')
            self._input_validated = True

    def set_entry_states(self, state):
        """
        state should be either 'normal' or 'disabled'.
        """
        for name, entry in self.text_fields.items():
            if not name == 'accel_range':  # Do not make accel_range modifiable
                entry.config(state=state)

    def set_calibration_button_states(self, state):
        """
        state should be either 'normal' or 'disabled'.
        """
        for exp in self.calibration_experiments:
            self.buttons[str(exp)].config(state=state)

    def quit_application(self):
        result = messagebox.askquestion("Confirmation",
                                        "Exit application?",
                                        icon='warning')
        if result == 'yes':
            self.root.quit()

    def create_graphic_interface(self):
        """"""
        self.format_grid_width()
        self.add_title('Infinite Uptime IDE+ Calibration Software')

        # Setup Section
        self.add_section_title('Calibration setup')
        self.add_text_field('port', text='COM Port (eg. COM7)')
        current_row = int(self.text_fields['port'].grid_info()['row'])
        self.add_text_field('mac_address', text='MAC Address')
        self.add_text_field('accel_range', text='Accelerometer Range (+/-G)',
                            default='4', disabled=True)
        self.add_text_field('operator_name', text='Calibrated By',
                            row=current_row, column=6)
        self.add_text_field('certificate_number', text='Certificate #',
                            row=current_row + 1, column=6)
        self.add_text_field('report_number', text='Report #',
                            row=current_row + 2, column=6)

        # Validate / Reset Input
        self.add_empty_row()
        self.add_button('input_validation', text='Validate',
                        command=self.validate_inputs, column=2,
                        sticky=tk.W+tk.E)
        self.add_empty_row()

        # Calibration Sections
        self.add_empty_row()
        for i, experiment in enumerate(self.calibration_experiments):
            self.add_calibration_section(i + 1, experiment)
            self.add_empty_row()

        # Print and quit
        self.add_empty_row()
        self.add_button('print', text='Print report',
                        command=self.print_report, column=2,
                        sticky=tk.W+tk.E+tk.S+tk.N)
        current_row = int(self.buttons['print'].grid_info()['row'])
        self.add_button('reset', text='Reset\ncalibrations',
                        command=self.reset_calibrations, row=current_row,
                        column=4, sticky=tk.W+tk.E+tk.S+tk.N)
        self.add_button('quit', text='Quit', command=self.quit_application,
                        column=9, sticky=tk.W+tk.E)
        self.add_empty_row()

        # Alerts generation
        self.create_pop_up_alert('no_port', 'Please input a port')
        self.create_pop_up_alert(
                'wrong_port',
                'IDE / IDE+ is not available at specified port')
        self.create_pop_up_alert('no_mac_address',
                                 'Please input a MAC Address')
        self.create_pop_up_alert(
                'wrong_mac_address',
                "Specified MAC Address doesn't match the device's MAC Address")
        self.create_pop_up_alert('no_operator_name',
                                 "Please input the operator name")
        self.create_pop_up_alert('no_report_number',
                                 "Please input the report number")
        self.create_pop_up_alert('no_accel_range',
                                 'Please input the accelerometer range')
        self.create_pop_up_alert('no_certificate_number',
                                 'Please input the certificate number')
        self.create_pop_up_alert(
                'calibration_incomplete',
                'Please complete all calibration before printing')
        self.create_pop_up_alert('invalid_data',
                                 'Invalid data received from device.\n' +
                                 'Please check firmware version.')

    # ===== Printing methods =====

    def get_header_info(self):
        today = dt.date.today()
        info = [
            ('Report # :', self.get_user_input('report_number')),
            ('Manufacturer :', self.manufacturer),
            ('MAC Address :', self.get_user_input('mac_address')),
            ('Range :', '+/- {}g'.format(self.accel_range)),
            ('Least Count :', '{}g'.format(round(self.least_count, 4))),
            ('Calibration Date :', str(today)),
            ('Next Calibration Date :', str(today + dt.timedelta(365))),
            ('Ambient Calibration Condition :',
             '{}°C'.format(self.current_temperature))]
        return info

    def get_footer_info(self):
        info = [
            ('Calibrated By :', self.get_user_input('operator_name')),
            ('Certificate # :', self.get_user_input('certificate_number')),
            ('Uncertainty :', '{}% Confidence'.format(self.uncertainty))]
        return info

    def print_report(self):
        """ """
#        if not all(exp.done for exp in self.calibration_experiments):
#            self.alerts['calibration_incomplete'].deiconify()
#            return
        pdf = CalibrationReportPDF(title='IU Hardware Calibration Report')
        pdf.add_infos(self.get_header_info())
        # Vibration Experiments
        experiments = self.get_all_calibration_from_type(VibrationCalibration)
        pdf.add_calibration_section(experiments, 'freqZ', 'rmsZ',
                                    y=max(pdf.current_y, 60))
        pdf.add_calibration_section(experiments, 'rmsZ', 'freqZ',
                                    y=max(pdf.current_y, 60))
        pdf.add_calibration_section(experiments, 'velocityZ', 'freqZ',
                                    y=max(pdf.current_y, 60))
        # TODO Add other experiment types here if needed
        pdf.add_infos(self.get_footer_info())
        filepath = asksaveasfilename()
        if filepath[-4:] != '.pdf':
            if filepath[-4:] == '.PDF':
                filepath = filepath[-4:] + '.pdf'
            else:
                filepath += '.pdf'
        pdf.output(filepath)


# =============================================================================
# PDF printing
# =============================================================================

# ===== Colors =====
RGBColor = namedtuple('RGBColor', 'Red Green Blue')

WHITE = RGBColor(252, 252, 252)
BLACK = RGBColor(0, 0, 0)
VIKING = RGBColor(77, 171, 193)  # IU Logo 'blue' color
DIM_GRAY = RGBColor(102, 102, 102)
HAVELOCK_BLUE = RGBColor(31, 138, 234)  # URL color


class CalibrationReportPDF(fpdf.FPDF):

    logo_filename = 'IU_logo.png'

    def __init__(self, *args, title='', author='Infinite Uptime',
                 creator='Infinite Uptime', **kwargs):
        kwargs.setdefault('format', 'letter')
        super().__init__(*args, **kwargs)
        self.metadata = dict(title=title,
                             author=author,
                             creator=creator)
        self.set_title(title)
        self.set_author(author)
        self.set_creator(creator)
        self.add_page()
        self.current_x = self.l_margin
        self.current_y = self.t_margin

    def header(self):
        self.image(self.logo_filename, w=72)
        self.set_font('Arial', size=15)
        txt = 'HARDWARE '
        w = self.get_string_width(txt)
        self.cell(w, 9, txt, 0, 0, 'C', 0)
        self.set_font('Arial', style='B', size=15)
        txt = 'CALIBRATION'
        w = self.get_string_width(txt)
        self.cell(w, 9, txt, 0, 0, 'C', 0)
        self.set_font('Arial', size=15)
        self.ln(15)

    def footer(self):
        self.set_font('Arial', size=10)
        txt1 = 'Refer to '
        txt2 = 'infinite-uptime.com/products/industrial-data-enabler'
        self.set_y(-20)
        self.cell(0, 4, '©Infinite Uptime, INC 2016', 0, 1)
        self.cell(self.get_string_width(txt1), 4, txt1, ln=0)
        self.set_text_color(*HAVELOCK_BLUE)
        self.set_font('Arial', style='U', size=10)
        self.cell(0, 4, txt2, link='http://infinite-uptime.com/products/' +
                  'industrial-data-enabler')
        # Position at 1.5 cm from bottom
        self.set_y(-10)
        # Arial italic 8
        self.set_font('Arial', 'I', 8)
        # Text color in gray
        self.set_text_color(128)
        # Page number
        self.cell(0, 10, 'Page ' + str(self.page_no()), 0, 0, 'C')

    def add_infos(self, info, limit=130, w=60, h=6, x=0, y=0):
        """
        infos is a list of tuple [(category, value)]
        """
        x = x or self.current_x
        y = y or self.current_y
        for i, (cat, val) in enumerate(info):
            self.set_xy(x + limit - w, y + h * i)
            self.set_font('Arial', size=12)
            self.cell(w, h=h, txt=cat, align='R')
            self.set_font('Arial', style='B', size=12)
            self.cell(w, h=h, txt=val, ln=1)
        self.ln()
        self.current_x = self.l_margin
        self.current_y = y + h * len(info)

    def add_calibration_section(self, calibration_experiments, main_field,
                                secondary_field, x=0, y=0):
        """All calibration experiments must be of the same type."""
        if len(calibration_experiments) == 0:
            return
        exp_type = type(calibration_experiments[0])
        for exp in calibration_experiments[1:]:
            if not isinstance(exp, exp_type):
                msg = "All calibration experiments must be of the same type"
                return TypeError(msg)
        x = x or self.current_x
        y = y or self.current_y
        self.set_xy(x, y)
        self.set_font('Arial', size=10)
        exp0 = calibration_experiments[0]
        idx1 = exp0.fields.index(main_field)
        idx2 = exp0.fields.index(secondary_field)
        unit = exp0.units[idx1]
        title = '{} Calibration - Units: {}'
        title = title.format(exp0.verbose_names[idx1], unit)
        headers = ['No.',
                   '{} ({})'.format(exp0.verbose_names[idx2],
                                    exp0.units[idx2]),
                   'Actual ({})'.format(unit),
                   'Measured ({})'.format(unit),
                   'Error',
                   'Tolerance',
                   'Result']
        rows = []
        for i, exp in enumerate(calibration_experiments):
            rows.append([i + 1,
                         str(exp.ref_values[idx2]),
                         str(exp.ref_values[idx1]),
                         str(round(exp.avg_values[main_field], 2)),
                         str(round(exp.errors[main_field], 2)) + '%',
                         '+/- {}'.format(round(exp.tolerances[idx1], 2)) + '%',
                         'OK' if exp.passed[main_field] else 'NOT OK'])
        self.add_table(headers, rows, title=title, x=x, y=y)

    def add_table(self, headers, rows, col_widths=None, h=8, title='',
                  x=0, y=0, fit_width=True):
        """ """
        x = x or self.current_x
        y = y or self.current_y
        # Check if enough room to add the table
        bot_limit = self.h - self.b_margin
        table_heigth = (len(rows) + 2) * h
        if y + table_heigth > bot_limit:
            self.add_page()  # go to next page
            self.current_y = self.y
            y = self.y

        x = x or self.current_x
        y = y or self.current_y
        self.set_xy(x, y)
        if col_widths is None:
            col_widths = [self.get_string_width(head) + 4 for head in headers]
        if fit_width:
            full_width = self.w - (self.l_margin + self.r_margin)
            used_width = sum(col_widths)
            factor = full_width / used_width
            col_widths = [w * factor for w in col_widths]

        self.set_line_width(0.2)
        self.set_draw_color(*BLACK)
        self.set_text_color(*WHITE)
        # Title
        if title:
            self.set_fill_color(*BLACK)
            self.cell(sum(col_widths), h=h, txt=title, border=1,
                      ln=1, align='L', fill=1)
            y += h
        # Header
        self.set_fill_color(*DIM_GRAY)
        for w, head in zip(col_widths, headers):
            self.cell(w, h=h, txt=head, border=1, ln=0, align='C', fill=1)
        self.ln()
        y += h
        # Rows
        self.set_draw_color(*BLACK)
        self.set_fill_color(*WHITE)
        self.set_text_color(*BLACK)
        for row in rows:
            for w, cell_content in zip(col_widths, row):
                self.cell(w, h=h, txt=str(cell_content), border=1, ln=0,
                          align='C', fill=1)
            self.ln()
        # Closing line
        self.cell(sum(col_widths), 0, '', 'T')
        self.ln()
        y += h
        self.current_x = self.l_margin
        self.current_y = y + h * len(rows)


# =============================================================================
# Run
# =============================================================================

root = tk.Tk()
app = CalibrationInterface(root)
app.mainloop()
root.destroy()
