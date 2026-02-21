# a terminal app to play uno programmed entirely in C

Uses local sockets and a client server model
delivers packets using TCP

to build:

run `make` in your terminal
*note, only works on UNIX systems as it uses UNIX apis*

run `./uno --server` to host a game
run `./uno --client [GAME CODE]` to connect to a game (falls back to creating a  
server and client instance if no code is provided)
