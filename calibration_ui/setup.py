#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 30 15:02:52 2017

@author: pat

Run with:
$python setup.py build
"""

from cx_Freeze import setup, Executable

addtional_mods = ['numpy.core._methods', 'numpy.lib.format']
setup(
    name = "IU Calibration Software",
    version = "0.1",
    description = "IDE+ calibration helper",
    options = {'build_exe': {'includes': addtional_mods}},
    executables = [Executable("calibration_ui.py")],
)
