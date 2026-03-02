
#ifndef UNO_SERVER_H
#define UNO_SERVER_H

#include <stdint.h>

void close_game_server(int clients[4], int reason, int server_fd);

void run_server(int clients[4], int server_fd, int real_players);

int start_game_server(uint16_t port, int clients[4], int real_players);


#endif // UNO_SERVER_H
