#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 30 15:02:52 2017

@author: pat

Run with:
$python setup.py build
"""

import sys
import os

from cx_Freeze import setup, Executable

script_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(script_path)

# For windows, need to manually add path to tkinter libraries
#os.environ['TCL_LIBRARY'] = r'C:\Users\IU PC2016\AppData\Local\Programs\Python\Python36-32\tcl\tcl8.6'
#os.environ['TK_LIBRARY'] = r'C:\Users\IU PC2016\AppData\Local\Programs\Python\Python36-32\tcl\tk8.6'

addtional_mods = ['numpy.core._methods', 'numpy.lib.format']
setup(
    name = "IU Calibration Software",
    version = "0.1",
    description = "IDE+ calibration helper",
    options = {'build_exe': {'includes': addtional_mods}},
    executables = [Executable("triaxial_calibration_ui.py")],
)
