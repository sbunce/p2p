@ECHO OFF
cls
cd src && scons && cd .. && copy /Y src\p2p_gui.exe . && copy /Y src\p2p_nogui.exe .
