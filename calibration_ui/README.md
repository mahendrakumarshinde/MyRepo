# Infinite Uptime Calibration UI #

A calibration UI, using Tkinter.

## How it works ##
After connecting the IDE / IDE+ to one of the laptop's USB port, the user can use the calibration_ui script to run "calibration experiments".

Each calibration experiment is a test to see if the device outputs results as expected. The calibration experiment then has a list of expected results, and a list of allowable tolerance for these measures. For each mesure, we can then determine if the test is passed or failed.

After each calibration experiment has been run, the user can print a PDF report.


## Executable creation with cx_freeze ##
To make the UI more user friendly, we compile it in an executable file.

We chose python library cx_freeze to do so because it's easy to use. To buid the executable file (read below section first though), just run:
$python3 setup.py build

**To read before build the executable**
cx_freeze is easy to use, but it has some drawbacks:
- Executable for each OS (Windows, Mac OS, Linux) has to be compilated using that OS.
- tkinter and cx_freeze don't interact well:
  - For Windows, paths to tkinter library for your python have to be manually specified. Open the setup.py file and uncomment + update the paths
  - After building the exectuble ($python setup.py build), you have to manually add some files and/or folders to the build folder.
    - For linux, copy these 2 folders: tcl8.? and tk8.? (replace '?' by the minor version you're using), from /lib folder (or /tcl folder) of your python folder, to the executable folder.
    - For windows, copy tcl8?t.dll and tk8?t.dll (replace '?' by the minor version you're using) to the build folder
- Also copy the IU_logo.png file to the build folder.
