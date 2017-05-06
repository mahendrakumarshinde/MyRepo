# Infinite Uptime Calibration UI #

## Executable creation ##
An executable file is created using cx_freeze. However, this doesn't interact well with tkinter, a Python graphical library.
After building the exectuble ($python setup.py build), you have to manually copy these 2 folders: tcl8.5 and tk8.5, from /lib folder (or /tcl folder) of your python folder, to the executable folder.
eg:
cp -R /home/pat/anaconda3/lib/tcl8.5 /home/pat/workspace/python/testing/build/exe.linux-x86_64-3.6
cp -R /home/pat/anaconda3/lib/tk8.5 /home/pat/workspace/python/testing/build/exe.linux-x86_64-3.6

Also copy the IU_logo.png file to the same folder.
