

#ifndef UNO_CLIENT_H
#define UNO_CLIENT_H

#include <stdint.h>
#include <stdio.h>
#include "uno.h"

typedef struct {
    uint8_t player_id;
    int server_sock;
    Hand* current_hand;
} ClientGameDetails;

void enable_raw_mode();

void disable_raw_mode();

void draw_single_card_at_coords(int x, int y, const CardDetails* details);

ClientGameDetails* connect_to_server(const char* ip, uint16_t port);
void clear_card_area(int x, int y);
void get_terminal_size(int* rows, int* cols) ;

void move_cursor(int x, int y);
void set_color(int fg);
int get_input() ;
void draw_centered_text(int x, int y, const char* text);
void draw_card(int x, int y, const char* text, const char* value, int color);
void run_client(const ClientGameDetails details);

#endif // UNO_CLIENT_H
