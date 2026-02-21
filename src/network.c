
#include "network.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

int set_socket_timeout(int fd, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    
    // Set the send timeout
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        return -1;
    }
    return 0;
}

// returns offset of next byte to write in dest, or -1 on error
int write_bytes(void* src, void* dest, int size) {
    if (src == NULL || dest == NULL) {
        return -1;
    }
    memcpy(dest, src, size);
    return size;
}

int read_bytes(void* src, void* dest, int size) {
    if (src == NULL || dest == NULL) {
        return -1;
    }
    memcpy(dest, src, size);
    return size;
}

int serialize_card_details(CardDetails* src, char* dest) {
    int offset = 0;
    offset += write_bytes(&src->color_code, dest + offset, sizeof(src->color_code));
    offset += write_bytes(&src->discarded, dest + offset, sizeof(src->discarded));
    offset += write_bytes(src->color_str, dest + offset, sizeof(src->color_str));
    offset += write_bytes(src->value_str, dest + offset, sizeof(src->value_str));
    offset += write_bytes(src->original_text, dest + offset, sizeof(src->original_text));
    return offset;
}

int deserialize_card_details(char* src, CardDetails* dest) {
    int offset = 0;
    offset += read_bytes(src + offset, &dest->color_code, sizeof(dest->color_code));
    offset += read_bytes(src + offset, &dest->discarded, sizeof(dest->discarded));
    offset += read_bytes(src + offset, dest->color_str, sizeof(dest->color_str));
    offset += read_bytes(src + offset, dest->value_str, sizeof(dest->value_str));
    offset += read_bytes(src + offset, dest->original_text, sizeof(dest->original_text));
    return offset;
}

int serialize_action(struct Action* src, char* dest) {
    int offset = 0;
    offset += write_bytes(&src->type, dest + offset, sizeof(src->type));
    offset += write_bytes(&src->player_id, dest + offset, sizeof(src->player_id));
    offset += write_bytes(&src->card_index, dest + offset, sizeof(src->card_index));
    offset += write_bytes(&src->chosen_color, dest + offset, sizeof(src->chosen_color));
    return offset;
}

int deserialize_action(char* src, struct Action* dest) {
    int offset = 0;
    offset += read_bytes(src + offset, &dest->type, sizeof(dest->type));
    offset += read_bytes(src + offset, &dest->player_id, sizeof(dest->player_id));
    offset += read_bytes(src + offset, &dest->card_index, sizeof(dest->card_index));
    offset += read_bytes(src + offset, &dest->chosen_color, sizeof(dest->chosen_color));
    return offset;
}

int serialize_packet(struct Packet* packet, void* buffer) {
    memset(buffer, 0, MAX_PACKET_SIZE);
    int offset = 0;
    offset += write_bytes(&packet->type, buffer + offset, sizeof(packet->type));

    switch (packet->type) {
        case MSG_WELCOME:
            offset += write_bytes(&packet->data.player_id, buffer + offset, sizeof(packet->data.player_id));
            break;
        case MSG_WAITING_FOR_PLAYERS:
            offset += write_bytes(&packet->data.num_players, buffer + offset, sizeof(packet->data.num_players));
            break;
        case MSG_STATE:
            offset += write_bytes(&packet->data.game_state.current_player_id, buffer + offset, sizeof(packet->data.game_state.current_player_id));
            offset += write_bytes(packet->data.game_state.player_hand_sizes, buffer + offset, sizeof(packet->data.game_state.player_hand_sizes));
            offset += write_bytes(&packet->data.game_state.direction, buffer + offset, sizeof(packet->data.game_state.direction));
            offset += serialize_card_details(&packet->data.game_state.top_card, buffer + offset);
            offset += serialize_action(&packet->data.game_state.last_action, buffer + offset);
            break;
        case MSG_HAND:
            offset += write_bytes(&packet->data.player_hand.player_id, buffer + offset, sizeof(packet->data.player_hand.player_id));
            offset += write_bytes(&packet->data.player_hand.num_cards, buffer + offset, sizeof(packet->data.player_hand.num_cards));
            for (int i = 0; i < packet->data.player_hand.num_cards; i++) {
                offset += serialize_card_details(&packet->data.player_hand.cards[i], buffer + offset);
            }
            break;
        case MSG_ACTION:
            offset += serialize_action(&packet->data.action, buffer + offset);
            break;
        case MSG_GAME_OVER:
            offset += write_bytes(&packet->data.winner_id, buffer + offset, sizeof(packet->data.winner_id));
            break;
        case MSG_ERROR:
            offset += write_bytes(&packet->data.error_code, buffer + offset, sizeof(packet->data.error_code));
            break;
    }

    return offset;
}

struct Packet* deserialize_packet(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 1) {
        return NULL;
    }
    struct Packet* packet = calloc(1, sizeof(struct Packet));
    int offset = 0;
    offset += read_bytes(buffer + offset, &packet->type, sizeof(packet->type));

    switch (packet->type) {
        case MSG_WELCOME:
            offset += read_bytes(buffer + offset, &packet->data.player_id, sizeof(packet->data.player_id));
            break;
        case MSG_WAITING_FOR_PLAYERS:
            offset += read_bytes(buffer + offset, &packet->data.num_players, sizeof(packet->data.num_players));
            break;
        case MSG_STATE:
            offset += read_bytes(buffer + offset, &packet->data.game_state.current_player_id, sizeof(packet->data.game_state.current_player_id));
            offset += read_bytes(buffer + offset, packet->data.game_state.player_hand_sizes, sizeof(packet->data.game_state.player_hand_sizes));
            offset += read_bytes(buffer + offset, &packet->data.game_state.direction, sizeof(packet->data.game_state.direction));
            offset += deserialize_card_details(buffer + offset, &packet->data.game_state.top_card);
            offset += deserialize_action(buffer + offset, &packet->data.game_state.last_action);
            break;
        case MSG_HAND:
            offset += read_bytes(buffer + offset, &packet->data.player_hand.player_id, sizeof(packet->data.player_hand.player_id));
            offset += read_bytes(buffer + offset, &packet->data.player_hand.num_cards, sizeof(packet->data.player_hand.num_cards));
            for (int i = 0; i < packet->data.player_hand.num_cards; i++) {
                offset += deserialize_card_details(buffer + offset, &packet->data.player_hand.cards[i]);
            }
            break;
        case MSG_ACTION:
            offset += deserialize_action(buffer + offset, &packet->data.action);
            break;
        case MSG_GAME_OVER:
            offset += read_bytes(buffer + offset, &packet->data.winner_id, sizeof(packet->data.winner_id));
            break;
        case MSG_ERROR:
            offset += read_bytes(buffer + offset, &packet->data.error_code, sizeof(packet->data.error_code));
            break;
    }

    return packet;
}

int setup_server(uint16_t port){
    int server_fd; // For bind
    struct sockaddr_in server_addr; // For bind

    int opt = 1; // For setsockopt
    //
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("bind");
        return -1;
    }

    if (listen(server_fd, 4)) {
        perror("listen");
        return -1;
    }


    return server_fd;

}

int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return -1;
    }
    return client_fd;
}

int send_all(int fd, const void* buffer, size_t length) {
    size_t total = 0;
    const char* buf = buffer;

    while (total < length) {
        ssize_t n = send(fd, buf + total, length - total, 0);
        if (n <= 0)
            return -1;
        total += n;
    }
    return 0;
}

int recv_all(int fd, void* buffer, size_t length) {
    size_t total = 0;
    char* buf = buffer;

    while (total < length) {
        ssize_t n = recv(fd, buf + total, length - total, 0);
        if (n <= 0)
            return -1;
        total += n;
    }
    return 0;
}

int send_packet(int client_fd, struct Packet* packet) {
    uint8_t payload[MAX_PACKET_SIZE];
    size_t payload_size = serialize_packet(packet, payload);

    uint32_t len = htonl(payload_size);

    if (send_all(client_fd, &len, sizeof(len)) < 0)
        return -1;

    if (send_all(client_fd, payload, payload_size) < 0)
        return -1;

    return 0;
}

int read_packet(int client_fd, struct Packet** packet) {
    uint32_t net_len;
    
    // Step 1: Read length
    if (recv_all(client_fd, &net_len, sizeof(net_len)) < 0)
        return READ_ERROR_RECV_LEN;

    uint32_t payload_size = ntohl(net_len);

    if (payload_size > MAX_PACKET_SIZE)   // sanity check
        return READ_ERROR_INVALID_PAYLOAD_SIZE;

    char* buffer = malloc(payload_size);
    if (!buffer)
        return READ_ERROR_MALLOC;

    // Step 2: Read full payload
    if (recv_all(client_fd, buffer, payload_size) < 0) {
        free(buffer);
        return READ_ERROR_RECV_PAYLOAD;
    }

    // Step 3: Deserialize
    *packet = deserialize_packet(buffer, payload_size);

    free(buffer);
    return *packet ? READ_OK : READ_ERROR_DESERIALIZE;
}

int send_player_hand(int client_fd, uint8_t player_id){

    Hand* hand = get_player_hand(player_id);

    if (hand == NULL) {
        fprintf(stderr, "Error: Player %d hand not found\n", player_id);
        return -1;
    }

    int count = hand->card_count;

    struct Packet packet = {0};

    packet.type = MSG_HAND;
    packet.data.player_hand.player_id = player_id;
    packet.data.player_hand.num_cards = count;

    memcpy(packet.data.player_hand.cards, hand->cards, count * sizeof(CardDetails));

    return send_packet(client_fd, &packet);

}

void close_server(int server_fd) {
    close(server_fd);
}
