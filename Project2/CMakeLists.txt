cmake_minimum_required(VERSION 3.26)
project(Project2 C)

set(CMAKE_C_STANDARD 11)

add_executable(Project2 Bank.c
        BankServer.c)
