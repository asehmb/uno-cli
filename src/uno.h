
#ifndef UNO_H
#define UNO_H

#include <stdlib.h>

#define DECK_SIZE 108

typedef struct {
    uint8_t color_code;
    uint8_t discarded;
    char color_str[8];
    char value_str[8];
    char original_text[16];
} CardDetails;

typedef struct {
    CardDetails* cards;
    int card_count;
    int selected_index; // for UI highlighting
    
} Hand;



struct cardStack{
    CardDetails* cards;
    int stack_top_index;
    int size;
    int* card_indices_queue;
};

struct GameDetails {
    struct cardStack deck_stack;
    struct cardStack discard_pile;
    Hand hands[4];
    int current_player;
    int direction; // 1 for clockwise, -1 for counter-clockwise
};





int get_direction();

void get_card_details(const char* card_text, CardDetails* details);

void shuffle_deck(CardDetails* cards, int count);

void createDeck();

Hand* get_player_hand(uint8_t player_num);

CardDetails* get_top_discard();

int cleanup();

int play_card(int player_num, int card_index); // Changed return type to int
int can_play_card(int player_num, int card_index); // Added for validation
void next_player(); // Added to advance player considering direction

CardDetails* pickup_card(int player_num);

void init_game();

int get_deck_size();

int get_current_player();

int bot_play(int player_num);

void change_color(uint8_t color_code);


static const char* deck[] = {
    "red 0",
        "red 1",
        "red 1",
        "red 2",
        "red 2",
        "red 3",
        "red 3",
        "red 4",
        "red 4",
        "red 5",
        "red 5",
        "red 6",
        "red 6",
        "red 7",
        "red 7",
        "red 8",
        "red 8",
        "red 9",
        "red 9",
        "yellow 0",
        "yellow 1",
        "yellow 1",
        "yellow 2",
        "yellow 2",
        "yellow 3",
        "yellow 3",
        "yellow 4",
        "yellow 4",
        "yellow 5",
        "yellow 5",
        "yellow 6",
        "yellow 6",
        "yellow 7",
        "yellow 7",
        "yellow 8",
        "yellow 8",
        "yellow 9",
        "yellow 9",
        "green 0",
        "green 1",
        "green 1",
        "green 2",
        "green 2",
        "green 3",
        "green 3",
        "green 4",
        "green 4",
        "green 5",
        "green 5",
        "green 6",
        "green 6",
        "green 7",
        "green 7",
        "green 8",
        "green 8",
        "green 9",
        "green 9",
        "blue 0",
        "blue 1",
        "blue 1",
        "blue 2",
        "blue 2",
        "blue 3",
        "blue 3",
        "blue 4",
        "blue 4",
        "blue 5",
        "blue 5",
        "blue 6",
        "blue 6",
        "blue 7",
        "blue 7",
        "blue 8",
        "blue 8",
        "blue 9",
        "blue 9",
        "red Draw2",
        "red Draw2",
        "red Reverse",
        "red Reverse",
        "red Skip",
        "red Skip",
        "yellow Draw2",
        "yellow Draw2",
        "yellow Reverse",
        "yellow Reverse",
        "yellow Skip",
        "yellow Skip",
        "green Draw2",
        "green Draw2",
        "green Reverse",
        "green Reverse",
        "green Skip",
        "green Skip",
        "blue Draw2",
        "blue Draw2",
        "blue Reverse",
        "blue Reverse",
        "blue Skip",
        "blue Skip",
        "black wild",
        "black wild",
        "black wild",
        "black wild",
        "black 4",
        "black 4",
        "black 4",
        "black 4"
};



#endif // UNO_H
