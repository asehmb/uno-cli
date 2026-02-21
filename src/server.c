
#include "server.h"
#include "uno.h"
#include "network.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "logger.h"

#define MAX_PLAYERS 4
extern struct GameDetails game_details; // Declare the global game_details from uno.c

struct GameState get_game_state_for_client() {
    struct GameState state;
    state.current_player_id = game_details.current_player;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        state.player_hand_sizes[i] = game_details.hands[i].card_count;
    }
    memcpy(&state.top_card, get_top_discard(), sizeof(CardDetails));
    
    // Placeholder for direction and last_action.
    // last_action also needs to be managed when actions are processed.
    state.direction = (get_direction() > 0) ? 0 : 1; // 0 for clockwise, 1 for counter-clockwise
    memset(&state.last_action, 0, sizeof(struct Action)); // Clear last action for now
    
    return state;
}

// This function needs to be declared in server.h and implemented here
int send_player_hand_to_client(int client_fd, uint8_t player_id) {
    struct Packet packet;
    packet.type = MSG_HAND;
    
    packet.data.player_hand.player_id = player_id;
    packet.data.player_hand.num_cards = game_details.hands[player_id].card_count;
    // Copy card details from game_details.hands[player_id] to packet.data.player_hand.cards
    // Ensure MAX_HAND_SIZE is respected
    for (int i = 0; i < game_details.hands[player_id].card_count && i < MAX_HAND_SIZE; i++) {
        packet.data.player_hand.cards[i] = game_details.hands[player_id].cards[i];
    }
    
    return send_packet(client_fd, &packet);
}

int start_game_server(uint16_t port, int clients[4]) {
    init_game();
    int socket = setup_server(port);
    if (socket < 0) {
        return socket;
    }
    clients[0] = clients[1] = clients[2] = clients[3] = -1;

    // now that server is open, wait for players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // accept player
        // update clinets to tell everyone a player joined
        // tell clients there id
        int new_client_sock = accept_client(socket);
        clients[i] = new_client_sock;
        struct Packet packet = { 0 };
        packet.type = MSG_WELCOME;
        packet.data.player_id = i;
        send_packet(new_client_sock, &packet);

    }


    // turn off blocking as we wait for clients to ready up in no specific order
    /* for (int i = 0; i < MAX_PLAYERS; i++) { */
    /*     fcntl(clients[i], F_SETFL, O_NONBLOCK); */
    /* } */
    /* uint8_t waiting_for_ready = 0; */
    /**/
    /* while (waiting_for_ready != 0x0F) { */
    /**/
    /*     // listen for ready packets from clients */
    /*     for (int i = 0; i < MAX_PLAYERS; i++) { */
    /*         struct Packet* packet; */
    /*         int result = read_packet(clients[i], &packet); */
    /*         if (result == READ_OK) { */
    /*             if (packet->type == MSG_WAITING_FOR_PLAYERS) { */
    /*                 waiting_for_ready = 0x0F; */
    /*                 waiting_for_ready |= (1 << i); */
    /*                 struct Packet return_data = { */
    /*                     MSG_WAITING_FOR_PLAYERS, */
    /*                     { waiting_for_ready } */
    /*                 }; */
    /*                 send_packet(clients[i], &return_data); */
    /*             } */
    /*             free(packet); */
    /*         } */
    /*     } */
    /**/
    /*     usleep(100000); // sleep for 100ms to avoid busy waiting */
    /* } */
    /**/
    
    // turn blocking back on for game play
    for (int i = 0; i < MAX_PLAYERS; i++) {
        fcntl(clients[i], F_SETFL, fcntl(clients[i], F_GETFL) & ~O_NONBLOCK);

    }

    return socket;
}

void run_server(int clients[4], int server_fd) {
    // get current player from gameDetails
    // make their socket the active one
    // wait for a action from them
    // handle the action
    // move onto next player
    //
    //
    // after game end
    // tell everyone winner
    // ask for replay from everyone
    // if yes:
    //    clear out game state
    //
    int running = 1;
    int current_player = 0;
    int current_player_sock = clients[0];

    while (running) {
        // game loop
        // Broadcast game state and hands to all players at the start of each turn
        for (int i = 0; i < MAX_PLAYERS; i++) {
            struct GameState current_state = get_game_state_for_client();
            struct Packet state_packet = {
                MSG_STATE,
                .data.game_state = current_state
            };
            send_packet(clients[i], &state_packet);
            send_player_hand_to_client(clients[i], i);
            LOG_INFO("Sent game state and hand to player %d", i);
        }

        // Get the current player
        current_player = get_current_player();
        current_player_sock = clients[current_player];

        // Set current player's socket to blocking to wait for their action
        get_packet:;

        struct Packet* packet;
        int result = read_packet(current_player_sock, &packet);

        if (result == READ_OK) {
            LOG_INFO("Received packet from player %d: type %d", current_player, packet->type);
            if (packet->type == MSG_ACTION) {
                struct Action action = packet->data.action;
                int result;

                switch (action.type) {
                    case ACTION_PLAY_CARD:
                        LOG_INFO("\tPlayer %d attempts to play card at index %d", current_player, action.card_index);
                        result = play_card(current_player, action.card_index);
                        if (result == -1) {
                            // Invalid play, ask for action again
                            LOG_WARN("\tInvalid play by player %d: card index %d", current_player, action.card_index);
                            struct Packet error_packet = {
                                MSG_ERROR,
                                .data.error_code = ERROR_INVALID_ACTION
                            };
                            result = send_packet(current_player_sock, &error_packet);
                            if (result < 0) {
                                // Handle send error (e.g., client disconnected)
                                LOG_ERROR("Failed to send error packet to player %d, closing connection", current_player);
                                clients[current_player] = -1; // Mark client as disconnected
                                close_game_server(clients, 1, server_fd); // Close server due to error
                            }
                            goto get_packet;
                        }
                        if (result == 4 || result == 5) { // wild card
                            change_color(action.chosen_color);
                        }
                        next_player();
                        break;
                    case ACTION_DRAW_CARD:
                        pickup_card(current_player);
                        // TODO: Implement logic for playing after drawing or skipping after drawing.
                        next_player(); // Advance turn after drawing
                        break;
                    case ACTION_SKIPPED:
                        // Player explicitly skipped their turn.
                        next_player(); // Advance turn
                        break;
                }

                // Check for win condition
                if (game_details.hands[current_player].card_count == 0) {
                    LOG_INFO("Player %d has won the game!", current_player);
                    running = 0; // End game loop
                    // Send game over message to all clients
                    for (int j = 0; j < MAX_PLAYERS; j++) {
                        struct Packet game_over_packet = {
                            MSG_GAME_OVER,
                            .data.winner_id = current_player
                        };
                        send_packet(clients[j], &game_over_packet);
                    }
                }
            }
            free(packet);
        }
        usleep(100000); // sleep for 100ms to let everyone else process the turn and avoid busy waiting
    }


    // After game ends, ask for replay (TODO)
}

void close_game_server(int clients[4], int reason, int server_fd) {
    printf("Closing server: %s\n", (reason == 0) ? "Normal shutdown" : "Error occurred");
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (clients[i] != -1) {
            close(clients[i]);
        }
    }
    close_server(server_fd);
}
