appserver-coarse: 	BankServer.o Bank.o
							gcc -o appserver-coarse BankServer.o Bank.o -lpthread -lrt
	
BankServer.o: BankServer.c Bank.h
							gcc -c BankServer.c
							
Bank.o: 			Bank.c Bank.h
							gcc -c Bank.c
							
.PHONY: all appserver-coarse clean

