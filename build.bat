@ECHO OFF
cls
cd src && python scons/scons.py && cd .. && copy /Y src\p2p_gui.exe . && copy /Y src\p2p_nogui.exe .
