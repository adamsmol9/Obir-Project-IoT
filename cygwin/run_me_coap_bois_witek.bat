@rem DLL preparing
@rem For EXE compiled under cyginw-win10
@rem 
PATH=./dll-win10;%PATH%%

@rem Exe simulation
@echo "Run simulation"
EBSimUnoEth.exe -ip 192.168.1.36 C:\Users\adams\AppData\Local\Temp\arduino_build_110632\uno.ino.hex
pause
