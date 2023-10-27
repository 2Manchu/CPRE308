BankServer: 	BankServer.o Bank.o
							gcc -o BankServer BankServer.o Bank.o -lpthread
	
BankServer.o: BankServer.c Bank.h
							gcc -c BankServer.c
							
Bank.o: 			Bank.c Bank.h
							gcc -c Bank.c
							
.PHONY: all BankServer clean

