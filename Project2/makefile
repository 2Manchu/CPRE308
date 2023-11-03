all: appserver appserver-coarse

appserver: 	BankServer.o Bank.o
		gcc -o appserver BankServer.o Bank.o -lpthread -lrt
		
appserver-coarse: BankServer-Coarse.o Bank.o
		gcc -o appserver-coarse BankServer-Coarse.o Bank.o -lpthread -lrt
	
appserver-coarse.o: BankServer-Coarse.c Bank.h
		gcc -c BankServer-Coarse.c

appserver.o: 	BankServer.c Bank.h
		gcc -c BankServer.c
							
Bank.o: 	Bank.c Bank.h
		gcc -c Bank.c
							
.PHONY: all appserver clean
				all appserver-coarse clean

