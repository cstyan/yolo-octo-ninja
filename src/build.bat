del obj\*.obj obj\*.res client-gui.exe  client-gui.exe.manifest
rc  /foobj/resource.res resource.rc
cl  /c /O1 /GS- /Oi-  client-* /Foobj/
link /SUBSYSTEM:WINDOWS obj/client-*.obj obj/resource.res WS2_32.lib user32.lib gdi32.lib comdlg32.lib 
