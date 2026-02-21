
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h> // for non-blocking input
#include <sys/ioctl.h> // for terminal size
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "network.h"
#include "client.h"
#include "logger.h"
#include <stdarg.h> // Required for va_list

// Debug viewport at the top of the screen
void debug_print(const char* fmt, ...) {
    // 1. Save cursor position (optional but good practice)
    // printf("\0337"); 
    
    // 2. Move to Row 2, Column 1 (Safe zone)
    move_cursor(1, 2);
    
    // 3. Clear the entire line first (to remove old debug msg)
    printf("\033[2K"); 
    
    // 4. Print the "DEBUG: " prefix in Red to make it visible
    printf("\033[1;31m[DEBUG] \033[0m");
    
    // 5. Print the actual message
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    // 6. Reset cursor acts as a flush usually
    fflush(stdout);
}
#define CARD_WIDTH 11
#define CARD_HEIGHT 7
#define OPPS_MAX_CARDS 5 // less cards that are visibile in opponents hands but they can have more

struct termios orig_termios;

void move_cursor(int x, int y);
void set_color(int fg);
void draw_card(int x, int y, const char* text, const char* value, int color);



// Function to draw a single card at specific coordinates
void draw_single_card_at_coords(int x, int y, const CardDetails* details) {
    draw_card(x, y, details->original_text, details->value_str, details->color_code);
    printf("\033[0m"); // Reset color after drawing
}

// Function to clear a card area
void clear_card_area(int x, int y) {
    for (int row = 0; row < CARD_HEIGHT; ++row) {
        move_cursor(x, y + row);
        for (int col = 0; col < CARD_WIDTH; ++col) {
            printf(" ");
        }
    }
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode); // turn raw mode off at exit

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void get_terminal_size(int* rows, int* cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

void move_cursor(int x, int y) { printf("\033[%d;%dH", y, x); }

void set_color(int fg) {
    // color codes: 0-255
    printf("\x1b[38;5;%dm", fg);
}

int get_input() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return -1; // Error or no input
}

void draw_centered_text(int x, int y, const char* text) {
    int total_inner_width = 9; 
    int text_len = strlen(text);
    
    if (text_len > total_inner_width) text_len = total_inner_width;

    int padding = (total_inner_width - text_len) / 2;
    int extra_space = (total_inner_width - text_len) % 2; // For odd-lengths

    move_cursor(x, y);
    printf("│"); // Left border
    
    for(int i = 0; i < padding; i++) printf(" ");
    
    printf("%.*s", total_inner_width, text);
    
    for(int i = 0; i < (padding + extra_space); i++) printf(" ");
    
    printf("│"); // Right border
}

void draw_card(int x, int y, const char* text, const char* value, int color) {
    int type = strlen(value); // 1 = normal card,  1 <  special card

    set_color(color);
    move_cursor(x, y);
    printf("┌─────────┐");

    move_cursor(x, y+1);
    switch(type) {
        case 1:
            printf("│ %s       │", value);
            break;
        case 2:
            printf("│ %s      │", value);
            break;
        case 3:
            printf("│%s     │", value);
            break;
        case 4:
            printf("│%s     │", value);
            break;
        case 5:
            printf("│%s    │", value);
            break;
        case 7:
            printf("│%s  │", value);
            break;
        default:
            printf("│         │");
            // unexpected length
            break;
    }


    move_cursor(x, y + 2);
    printf("│         │");
    

    draw_centered_text(x, y + 3, text); // draw text centered

    move_cursor(x, y + 4);
    printf("│         │");

    move_cursor(x, y + 5);
    switch(type) {
        case 0:
             printf("│         │");
             break;
        case 1:
            printf("│       %s │", value);
            break;
        case 2:
            printf("│      %s │", value);
            break;
        case 3:
            printf("│     %s│", value);
            break;
        case 4:
            printf("│     %s│", value);
            break;
        case 5:
            printf("│    %s│", value);
            break;
        case 7:
            printf("│  %s│", value);
            break;
        default:
            printf("│       %s │", value);
            // unexpected length
            break;
    }
    
    move_cursor(x, y + 6);
    printf("└─────────┘");
}

void redraw_hand(const CardDetails* cards, int card_count, int selected_index, int x, int y, int prev_selected_index) {
        if (selected_index != prev_selected_index) {
            int start_x = (x/2)-((card_count*CARD_WIDTH)/2);
            int base_y = y;

            // Redraw previously selected card (unhighlighted)
            // It was at base_y - 1, now needs to be drawn at base_y
            clear_card_area(start_x + prev_selected_index * (CARD_WIDTH), base_y - 1);
            draw_single_card_at_coords(start_x + prev_selected_index * (CARD_WIDTH), base_y, &cards[prev_selected_index]);

            // Redraw newly selected card (highlighted)
            // It was at base_y, now needs to be drawn at base_y - 1
            clear_card_area(start_x + selected_index * (CARD_WIDTH), base_y);
            draw_single_card_at_coords(start_x + selected_index * (CARD_WIDTH), base_y - 1, &cards[selected_index]);
        }

}

void draw_deck(int x, int y) {
    set_color(15); // White color for deck
    move_cursor(x, y);
    printf("┌─────────┐");
    for (int i = 1; i <= 5; i++) {
        move_cursor(x, y + i);
        printf("│░░░░░░░░░│");
    }
    move_cursor(x, y + 6);
    printf("└─────────┘");

    // Draw count
    move_cursor(x + 3, y + 3);
    printf("UNO");
}

void draw_color_menu(int x, int y) {
    move_cursor(x, y);
    printf("Choose a color:");
    move_cursor(x, y + 1);
    set_color(196); // red
    printf("1. Red");
    move_cursor(x, y + 2);
    set_color(40); // green
    printf("2. Green");
    move_cursor(x, y + 3);
    set_color(20); // blue
    printf("3. Blue");
    move_cursor(x, y + 4);
    set_color(220); // yellow
    printf("4. Yellow");
    set_color(15); // reset color
}

void draw_hand(const CardDetails* cards, int card_count, int selected_index, int x, int y) {
    int start_x = x;
    int base_y = y; // Fixed y position for hand

    for (int i = 0; i < card_count; i++) {
        const CardDetails* details = &cards[i];

        int current_y = base_y;
        if (i == selected_index) {
            current_y = base_y - 1; // Highlighted position
        }
        
        draw_single_card_at_coords(start_x + i * (CARD_WIDTH), current_y, details);
    }
}

void redraw_whole_hand(const CardDetails* cards, int card_count, int selected_index, int x, int y) {
    int start_x = (x/2)-((card_count*CARD_WIDTH)/2);
    int base_y = y;

    for (int i = 0; i < card_count; i++) {
        const CardDetails* details = &cards[i];

        int current_y = base_y;
        if (i == selected_index) {
            current_y = base_y - 1; // Highlighted position
        }
        draw_single_card_at_coords(start_x + i * (CARD_WIDTH ), current_y, details);
    }
}

void read_input(int c, int* selected_index, int cols, int y, int prev_selected_index, Hand* current_hand, int server_sock, uint8_t player_id) {
    if (c == '\033') {
            char seq[2];
            usleep(1000); // small delay to allow full escape sequence to arrive
            if (read(STDIN_FILENO, &seq[0], 1) == 0) return;
            if (read(STDIN_FILENO, &seq[1], 1) == 0) return;
            debug_print("Key Pressed: %d ('%c')", c, (c >= 32 && c <= 126) ? c : '?');
            

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':
                        // Up arrow
                        debug_print("Up Arrow Pressed");
                        break;
                    case 'B':
                        // Down arrow
                        debug_print("Down Arrow Pressed");
                        break;
                    case 'C':
                        // Right arrow
                        debug_print("Action: right -> Index %d", *selected_index);
                        *selected_index += 1;
                        break;
                    case 'D':
                        // Left arrow
                        debug_print("Action: LEFT -> Index %d", *selected_index);

                        *selected_index -= 1;
                        break;
                    case 13:
                        // ebter key
                        *selected_index = 0;
                        break;
                 
                }
                if (*selected_index < 0) *selected_index = 0;
                else if (*selected_index >= current_hand->card_count) *selected_index = current_hand->card_count > 0 ? current_hand->card_count - 1 : 0;
                
                if (current_hand->card_count > 0) {
                    redraw_hand(current_hand->cards, current_hand->card_count, *selected_index, cols, y, prev_selected_index);
                }
            }
        }
        if (c==10) {
            // enter key
            debug_print("Action: PLAY CARD at Index %d", *selected_index);
            struct Action play_action = {
                .type = ACTION_PLAY_CARD,
                .player_id = player_id, // will be set by server
                .card_index = *selected_index,
                .chosen_color = 0,
            };
            
            CardDetails* played_card = &current_hand->cards[*selected_index];
            if (strcmp(played_card->color_str, "black") == 0) {
                // Wild card, prompt for color
                draw_color_menu(cols/2 - 10, y - 10);
                int color_choice = -1;
                while (color_choice < '1' || color_choice > '4') {
                    color_choice = get_input();
                }
                switch (color_choice) {
                    case '1': play_action.chosen_color = 196; break; // red
                    case '2': play_action.chosen_color = 40; break; // green
                    case '3': play_action.chosen_color = 20; break; // blue
                    case '4': play_action.chosen_color = 220; break; // yellow
                }
       

            }
            
            struct Packet packet = {
                .type = MSG_ACTION,
                .data.action = play_action
            };
            send_packet(server_sock, &packet);

        }
        if (c==32) {
            // space key
            struct Action draw_action = {
                .type = ACTION_DRAW_CARD,
                .player_id = player_id,
                .card_index = 0, // Ignored
                .chosen_color = 0
            };
            struct Packet packet = {
                .type = MSG_ACTION,
                .data.action = draw_action
            };
            send_packet(server_sock, &packet);
            
            draw_deck(60, 5 );
        }


}

ClientGameDetails* connect_to_server(const char* ip, uint16_t port) {
    //create socket and connect to server
    ClientGameDetails* details = malloc(sizeof(ClientGameDetails));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return details;
    }
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return details;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("failed to connect to server at %s:%d\n", ip, port);
        perror("connect");
        close(sockfd);
        return details;
    }
    struct Packet* welcome_packet;

    int status = read_packet(sockfd, &welcome_packet);
    if (status < 0 || welcome_packet->type != MSG_WELCOME) {
        printf("failed to receive welcome packet from server\n");
        close(sockfd);
        return details;
    }

    printf("connected to server, assigned player ID: %d\n", welcome_packet->data.player_id);
    details->player_id = welcome_packet->data.player_id;
    details->server_sock = sockfd;
    return details;
}

void run_client(const ClientGameDetails details) {
    LOG_INFO("Starting client with player ID %d", details.player_id);

    uint8_t current_player_id;
    uint8_t player_hand_sizes[MAX_PLAYERS];
    uint8_t direction;
    CardDetails top_card;

    struct Packet* state_packet;
    if (read_packet(details.server_sock, &state_packet) < 0 ||
        state_packet->type != MSG_STATE) {
        printf("failed to receive initial game state\n");
        close(details.server_sock);
        return;
    }
    LOG_INFO("Received initial game state from server");
    LOG_INFO("CURRENT PLAYER ID: %d", state_packet->data.game_state.current_player_id);
    LOG_INFO("DIRECTION: %d", state_packet->data.game_state.direction);
    LOG_INFO("TOP CARD: %s of %s", state_packet->data.game_state.top_card.value_str, state_packet->data.game_state.top_card.color_str);
    LOG_INFO("Player hand sizes:");
    for (int i = 0; i < MAX_PLAYERS; i++) {
        LOG_INFO("Player %d: %d cards", i, state_packet->data.game_state.player_hand_sizes[i]);
    }

    memcpy(&current_player_id, &state_packet->data.game_state.current_player_id, sizeof(uint8_t));
    memcpy(&player_hand_sizes, &state_packet->data.game_state.player_hand_sizes, sizeof(uint8_t) * MAX_PLAYERS);
    memcpy(&direction, &state_packet->data.game_state.direction, sizeof(uint8_t));
    memcpy(&top_card, &state_packet->data.game_state.top_card, sizeof(CardDetails));

    struct Packet* hand_packet;
    if (read_packet(details.server_sock, &hand_packet) < 0 ||
        hand_packet->type != MSG_HAND) {
        printf("failed to receive initial hand\n");
        close(details.server_sock);
        return;
    }
    LOG_INFO("Received initial hand from server");
    LOG_INFO("Hand has %d cards", hand_packet->data.player_hand.num_cards);
    LOG_INFO("Cards in hand:");
    for (int i = 0; i < hand_packet->data.player_hand.num_cards; i++) {
        CardDetails* card = &hand_packet->data.player_hand.cards[i];
        LOG_INFO("Card %d: %s of %s", i, card->value_str, card->color_str);
    }


    Hand current_hand;
    current_hand.cards = malloc(sizeof(CardDetails) * hand_packet->data.player_hand.num_cards);
    current_hand.card_count = hand_packet->data.player_hand.num_cards;
    memcpy(current_hand.cards,
           hand_packet->data.player_hand.cards,
           sizeof(CardDetails) * current_hand.card_count);

    LOG_INFO("Copied hand to local state");

    enable_raw_mode();
    printf("\033[?25l");
    printf("\033[2J");

    int rows, cols;
    get_terminal_size(&rows, &cols);

    int selected_index = 0;
    int prev_selected_index = -1;
    int running = 1;

    // Make socket + stdin non-blocking
    fcntl(details.server_sock, F_SETFL, O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    LOG_INFO("Entering  rendering phase");

    redraw_whole_hand(current_hand.cards,
                      current_hand.card_count,
                      selected_index,
                      cols,
                      rows - CARD_HEIGHT);

    draw_deck((cols/2)-8, (rows/2));
    draw_single_card_at_coords(8+(cols/2), rows/2, &top_card);

    fflush(stdout);

    LOG_INFO("Entering main client loop");

    set_socket_timeout(details.server_sock, 3); // Non-blocking with select


    fd_set readfds;
    while (running) {

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(details.server_sock, &readfds);

        int maxfd = (STDIN_FILENO > details.server_sock ?
                     STDIN_FILENO : details.server_sock) + 1;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 16000; // ~60 FPS tick

        int activity = select(maxfd, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("select");
            break;
        }

        // -----------------------
        // KEYBOARD INPUT
        // -----------------------
        if (FD_ISSET(STDIN_FILENO, &readfds)) {

            int c = get_input();
            if (c == 'q') {
                running = 0;
            } else if (c != -1) {

                read_input(c,
                           &selected_index,
                           cols,
                           rows - CARD_HEIGHT,
                           prev_selected_index,
                           &current_hand,
                           details.server_sock,
                           details.player_id);
                prev_selected_index = selected_index;
            }
        }

        // -----------------------
        // SERVER PACKETS
        // -----------------------
        if (FD_ISSET(details.server_sock, &readfds)) {

            struct Packet* packet;
            int status = read_packet(details.server_sock, &packet);

            if (status < 0) {
                debug_print("Error reading packet from server: %d", status);
                printf("Server disconnected.\n");
                running = 0;
            } else {

                switch (packet->type) {

                    case MSG_STATE:
                        memcpy(&top_card,
                               &packet->data.game_state.top_card,
                               sizeof(CardDetails));
                        break;

                    case MSG_HAND: { 

                        int old_count = current_hand.card_count;
                        int start_x = (cols/2) - ((old_count * CARD_WIDTH)/2);
                        for(int i=0; i<old_count; i++) {
                            clear_card_area(start_x + i*(CARD_WIDTH), rows - CARD_HEIGHT);
                            clear_card_area(start_x + i*(CARD_WIDTH), rows - CARD_HEIGHT - 1);
                        }

                        // Update Data
                        current_hand.cards = realloc(current_hand.cards, sizeof(CardDetails) * packet->data.player_hand.num_cards);
                        current_hand.card_count = packet->data.player_hand.num_cards;
                        memcpy(current_hand.cards, packet->data.player_hand.cards, 
                            sizeof(CardDetails) * current_hand.card_count);
                        
                        // Safety Check
                        if (selected_index >= current_hand.card_count)
                            selected_index = current_hand.card_count - 1;
                        if (selected_index < 0) selected_index = 0;

                        // Redraw NEW hand (No \033[2J!)
                        redraw_whole_hand(current_hand.cards, current_hand.card_count, 
                                        selected_index, cols, rows - CARD_HEIGHT);
                        break;
                    } 
                }

                free(packet);

                // redraw after server update
                redraw_whole_hand(current_hand.cards,
                                  current_hand.card_count,
                                  selected_index,
                                  cols,
                                  rows - CARD_HEIGHT);

                draw_single_card_at_coords(8+(cols/2), rows/2, &top_card);

                fflush(stdout);
            }
        }
    }

    printf("\033[?25h");
    disable_raw_mode();
    close(details.server_sock);
}
