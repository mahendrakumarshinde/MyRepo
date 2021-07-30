## Packaging utilities into binaries
This is the src directory for the python serial utilities.  
For any changes in a script, it has to be packaged into a binary, which has to be placed in flashing_tools directory.  
Follow the following steps to package a script into a binary format :  

1. Create a python3 environment.
2. Install pyserial module with pip.
3. Install pyinstaller module with pip.
4. Create a binary distribution with pyinstaller. (Command is "pyinstaller `utility.py`")
5. Copy the `utility` directory from "dist" directory to flashing_tools directory.

