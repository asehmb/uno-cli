
#include "client.h" // client functions
#include "server.h" // server functions
#include "uno.h"    // game logic header
#include <fcntl.h>  // for non-blocking input
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h> // for terminal size
#include <termios.h>
#include <time.h>
#include <unistd.h>

static int get_real_player_count(int argc, char *argv[]) {
  int real_players = 4;
  if (argc > 2) {
    real_players = atoi(argv[2]);
    if (real_players < 1 || real_players > 4) {
      fprintf(stderr, "Invalid real player count: %d (expected 1-4)\n",
              real_players);
      return -1;
    }
    return real_players;
  }

  printf("How many real players? (1-4): ");
  fflush(stdout);
  if (scanf("%d", &real_players) != 1 || real_players < 1 || real_players > 4) {
    fprintf(stderr, "Invalid input. Expected a number from 1 to 4.\n");
    return -1;
  }

  return real_players;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "--debug") == 0) {
    }
    if (strcmp(argv[1], "--server") == 0) {
      int clients[4] = {0, 0, 0, 0};
      int real_players = get_real_player_count(argc, argv);
      if (real_players < 0) {
        return 1;
      }
      int server_fd = start_game_server(5050, clients, real_players);
      if (server_fd < 0) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
      }
      run_server(clients, server_fd, real_players);
      close_game_server(clients, 0, server_fd);
      return 0;
    }
    if (strcmp(argv[1], "--client") == 0) {
      ClientGameDetails *details = connect_to_server("127.0.0.1", 5050);
      run_client(*details);
      return 0;
    }
  }

  cleanup();

  return 0;
}
