
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h> // for terminal size
#include <unistd.h>
#include "uno.h"  // game logic header
#include <time.h>
#include <fcntl.h> // for non-blocking input
#include "server.h" // server functions
#include "client.h" // client functions

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--debug") == 0) {
        }
        if (strcmp(argv[1], "--server") == 0) {
            int clients[4];
            int server_fd = start_game_server(5050, clients);
            if (server_fd < 0) {
                fprintf(stderr, "Failed to start server\n");
                return 1;
            }
            run_server(clients, server_fd);
            return 0;
        }
        if (strcmp(argv[1], "--client") == 0) {
            ClientGameDetails* details = connect_to_server("127.0.0.1", 5050);
            run_client(*details);
            return 0;

        }
    }


   
    cleanup();
    
    return 0;
}
