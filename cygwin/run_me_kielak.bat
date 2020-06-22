@rem DLL preparing
@rem For EXE compiled under cyginw-win10
@rem 
PATH=./dll-win10;%PATH%%

@rem Exe simulation
@echo "Run simulation"
EBSimUnoEth.exe -ip 192.168.0.192 .//main_coap.ino.hex