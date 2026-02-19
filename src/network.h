
#ifndef UNO_NETWORKING_H
#define UNO_NETWORKING_H

#include <stdlib.h>
#include "uno.h"

#define MAX_HAND_SIZE 50
#define MAX_PLAYERS 4
#define MAX_PACKET_SIZE 1024

typedef enum {
    MSG_WELCOME,
    MSG_WAITING_FOR_PLAYERS,
    MSG_STATE,
    MSG_HAND,
    MSG_ACTION,
    MSG_GAME_OVER,
    MSG_ERROR
} MsgType;

enum ActionType {
    ACTION_PLAY_CARD = 0,
    ACTION_DRAW_CARD = 1,
    ACTION_SKIPPED = 2
};

enum ErrorCode {
    ERROR_INVALID_ACTION = 0,
    ERROR_NOT_YOUR_TURN = 1,
    ERROR_INVALID_CARD_INDEX = 2,
    ERROR_INVALID_COLOR_CHOICE = 3
};

struct Action {
    uint8_t type; // ActionType
    uint8_t player_id; // ID of the player performing the action
    uint8_t card_index; // index of the card played, -1 if not applicable (e.g. for drawing a card)
    uint8_t chosen_color; // for wild cards
};

// info sent from server to every client every time a new turn starts
struct GameState {
    uint8_t current_player_id;
    uint8_t player_hand_sizes[MAX_PLAYERS]; // number of cards in each player's hand
    uint8_t direction; // 0 for clockwise, 1 for counterclockwise
    CardDetails top_card;
    // last turn's action, e.g. "Player 2 played a red 5" or "Player 3 drew a card"
    struct Action last_action;
};

struct PlayerHand {
    uint8_t player_id;
    uint8_t num_cards;
    CardDetails cards[MAX_HAND_SIZE];
};

enum PacketReadError {
    READ_OK = 0,
    READ_ERROR_RECV_LEN = -3,
    READ_ERROR_INVALID_PAYLOAD_SIZE = -4,
    READ_ERROR_MALLOC = -5,
    READ_ERROR_RECV_PAYLOAD = -6,
    READ_ERROR_DESERIALIZE = -1
};

struct Packet {
    uint8_t type; // MsgType
    union {
        uint8_t player_id; // for MSG_WELCOME
        //client sends FFFFFFFF to indicate they are ready, server responds with bitmask of which players are connected
        uint8_t num_players; // bit 0 = player 1 etc. for MSG_WAITING_FOR_PLAYERS 
        struct GameState game_state; // for MSG_STATE
        struct PlayerHand player_hand; // for MSG_HAND
        struct Action action; // for MSG_ACTION
        uint8_t winner_id; // for MSG_GAME_OVER
        uint8_t error_code; // for MSG_ERROR
    } data;
};

int setup_server(uint16_t port);

void close_server(int server_fd);



int read_packet(int client_fd, struct Packet** packet);

int accept_client(int server_fd);


int send_packet(int client_fd, struct Packet* packet);

int send_player_hand(int client_fd, uint8_t player_id);

#endif
