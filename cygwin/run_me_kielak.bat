@rem DLL preparing
@rem For EXE compiled under cyginw-win10
@rem 
PATH=./dll-win10;%PATH%%

@rem Exe simulation
@echo "Run simulation"
EBSimUnoEth.exe -ip 192.168.0.192 C:\Users\Michal\AppData\Local\Temp\arduino_build_871628\main_coap.ino.hex