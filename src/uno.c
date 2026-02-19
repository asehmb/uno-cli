

#include "uno.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


struct GameDetails game_details;


void add_to_hand(uint8_t player_num, CardDetails* card) {
    if (player_num >= 4) return; // invalid player number
    game_details.hands[player_num].cards = 
        realloc(game_details.hands[player_num].cards,
                (game_details.hands[player_num].card_count + 1) * sizeof(CardDetails));
    game_details.hands[player_num].cards[game_details.hands[player_num].card_count] = *card;
    game_details.hands[player_num].card_count++;
}

void remove_from_hand(uint8_t player_num, int card_index) {
    if (player_num >= 4 || card_index >= game_details.hands[player_num].card_count) return; // invalid
    for (uint8_t i = card_index; i < game_details.hands[player_num].card_count - 1; i++) {
        game_details.hands[player_num].cards[i] = game_details.hands[player_num].cards[i + 1];
    }
    game_details.hands[player_num].card_count--;
    game_details.hands[player_num].cards = 
        realloc(game_details.hands[player_num].cards,
                game_details.hands[player_num].card_count * sizeof(CardDetails));
}

Hand* get_player_hand(uint8_t player_num) {
    if (player_num >= 4) return NULL; // invalid
    return &game_details.hands[player_num];
}

void get_card_details(const char* card_text, CardDetails* details) {
    sscanf(card_text, "%s %s", details->color_str, details->value_str);
    details->color_code = 15; // Default white

    if (strcmp(details->color_str, "red") == 0) details->color_code = 196;
    else if (strcmp(details->color_str, "green") == 0) details->color_code = 40;
    else if (strcmp(details->color_str, "blue") == 0) details->color_code = 20;
    else if (strcmp(details->color_str, "yellow") == 0) details->color_code = 220;
    strncpy(details->original_text, card_text, sizeof(details->original_text) - 1);
}

void shuffle_deck(CardDetails* cards, int count) {
    srand(time(NULL));
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        CardDetails temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
}

// dequeu card from deck and add to hand
CardDetails* draw_card_from_deck() {
    if (game_details.deck_stack.stack_top_index < 0) {
        //refill deck from discard pile except the top card
        // save last played card to avoid losing it during shuffle
        CardDetails* last_played_card = &game_details.discard_pile.cards[game_details.discard_pile.stack_top_index];
        shuffle_deck(game_details.discard_pile.cards, game_details.discard_pile.stack_top_index + 1);
        CardDetails* temp = game_details.deck_stack.cards;
        game_details.deck_stack.cards = game_details.discard_pile.cards;
        game_details.discard_pile.cards = temp;
        game_details.deck_stack.stack_top_index = game_details.discard_pile.stack_top_index - 1; // -1 to keep the last played card in discard pile
        game_details.discard_pile.stack_top_index = 0; // reset discard pile to only have the last played card
        game_details.discard_pile.cards[0] = *last_played_card;
        
    }
    CardDetails* drawn_card = &game_details.deck_stack.cards[
        game_details.deck_stack.card_indices_queue[game_details.deck_stack.stack_top_index]
        ];
    game_details.deck_stack.stack_top_index--;
    return drawn_card;
}

// enqueue card back to deck from hand
void discard_card_to_pile(CardDetails* card) {
    // discard pile will not be full as someone must've won before that happens, so no need to check for overflow
    game_details.discard_pile.stack_top_index++;
    game_details.discard_pile.cards[game_details.discard_pile.stack_top_index] = *card;

}

void createDeck() {
    CardDetails* cards = malloc(DECK_SIZE * sizeof(CardDetails));
    for (int i = 0; i < DECK_SIZE; i++) {
        get_card_details(deck[i], &cards[i]);
        cards[i].discarded = 0;
    }
    shuffle_deck(cards, DECK_SIZE);
    game_details.deck_stack.size = DECK_SIZE;
    game_details.deck_stack.cards = cards;
    game_details.deck_stack.stack_top_index = DECK_SIZE - 1;
    game_details.deck_stack.card_indices_queue = malloc(DECK_SIZE * sizeof(int));
    for (int i = 0; i < DECK_SIZE; i++) {
        game_details.deck_stack.card_indices_queue[i] = i;
    }
    for (int i = 0; i < 4; i++) {
        game_details.hands[i].cards = malloc(sizeof(CardDetails) * 7); // starting hand size
        game_details.hands[i].card_count = 0;
    }
    game_details.discard_pile.cards = calloc(DECK_SIZE, sizeof(CardDetails));
    game_details.discard_pile.stack_top_index = -1;

}

int cleanup() {
    free(game_details.deck_stack.cards);
    for (int i = 0; i < 4; i++) {
        free(game_details.hands[i].cards);
    }
    return 0;
}

// Returns 1 if card can be played, 0 otherwise
int can_play_card(int player_num, int card_index) {
    if (player_num >= 4 || card_index >= game_details.hands[player_num].card_count) return 0; // invalid

    CardDetails* played_card = &game_details.hands[player_num].cards[card_index];
    CardDetails* top_discard = get_top_discard();

    // Wild cards can always be played
    if (strcmp(played_card->color_str, "black") == 0) return 1;

    // Matching color or matching value
    if (strcmp(played_card->color_str, top_discard->color_str) == 0 ||
        strcmp(played_card->value_str, top_discard->value_str) == 0) {
        return 1;
    }

    return 0;
}

// Returns an int representing the card effect:
// 0: Normal card
// 1: Skip card
// 2: Reverse card
// 3: Draw 2 card
// 4: Wild card
// 5: Wild Draw 4 card
int play_card(int player_num, int card_index) {
    if (!can_play_card(player_num, card_index)) return -1; // Cannot play card

    CardDetails* played_card = &game_details.hands[player_num].cards[card_index];
    discard_card_to_pile(played_card);
    remove_from_hand(player_num, card_index);

    if (strcmp(played_card->value_str, "Skip") == 0) {
        game_details.current_player = (game_details.current_player + game_details.direction + 4) % 4; // Skip next player
        return 1;
    } else if (strcmp(played_card->value_str, "Reverse") == 0) {
        game_details.direction *= -1; // Toggle direction
        return 2;
    } else if (strcmp(played_card->value_str, "Draw2") == 0) {
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        return 3;
    } else if (strcmp(played_card->value_str, "wild") == 0) {
        return 4;
    } else if (strcmp(played_card->value_str, "4") == 0 && strcmp(played_card->color_str, "black") == 0) { // Wild Draw 4
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        pickup_card((game_details.current_player + game_details.direction + 4) % 4);
        return 5;
    } else {
        return 0; // Normal card
    }
}

void next_player() {
    game_details.current_player = (game_details.current_player + game_details.direction + 4) % 4;
}


CardDetails* pickup_card(int player_num) {
    if (player_num >= 4) return NULL; // invalid
    CardDetails* card = draw_card_from_deck();
    add_to_hand(player_num, card);

    return &game_details.hands[player_num].cards[game_details.hands[player_num].card_count - 1];
}

void init_game() {
    createDeck();
    game_details.discard_pile.stack_top_index = 0;
    game_details.discard_pile.cards[0] = *draw_card_from_deck(); // draw first card to start discard pile
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 4; j++) {
            CardDetails* card = draw_card_from_deck();
            add_to_hand(j, card);
        }
    }
    game_details.current_player = 0; // Start with player 0
    game_details.direction = 1; // Clockwise
    return;

}

int get_deck_size() {
    return game_details.deck_stack.stack_top_index + 1;
}

int get_current_player() {
    return game_details.current_player;
}

CardDetails* get_top_discard() {
    if (game_details.discard_pile.stack_top_index < 0) return NULL;
    return &game_details.discard_pile.cards[game_details.discard_pile.stack_top_index];
}

void change_color(uint8_t color_code) {
    CardDetails* top_card = get_top_discard();
    if (top_card == NULL) return;
    top_card->color_code = color_code;
    if (color_code == 196) {
        strcpy(top_card->color_str, "red");
    } else if (color_code == 40) {
        strcpy(top_card->color_str, "green");
    } else if (color_code == 20) {
        strcpy(top_card->color_str, "blue");
    } else if (color_code == 220) {
        strcpy(top_card->color_str, "yellow");
    }
}


int bot_play(int player_num) {
    if (player_num == 0 || player_num >= 4) return -1; // Not a bot

    Hand* bot_hand = get_player_hand(player_num);
    for (int i = 0; i < bot_hand->card_count; i++) {
        if (can_play_card(player_num, i)) {
            int card_effect = play_card(player_num, i);
            if (card_effect != -1) {
                switch (card_effect) {
                    case 1: // Skip
                        next_player();
                        break;
                    case 2: // Reverse
                        // Direction is already changed in play_card
                        break;
                    case 3: // Draw 2
                        next_player();
                        pickup_card(get_current_player());
                        pickup_card(get_current_player());
                        break;
                    case 5: // Wild Draw 4
                        next_player();
                        pickup_card(get_current_player());
                        pickup_card(get_current_player());
                        pickup_card(get_current_player());
                        pickup_card(get_current_player());
                        break;
                }
            }
            return card_effect;
        }
    }

    // No playable card, draw one
    pickup_card(player_num);
    return 0; // Signifies a card was drawn
}


int get_direction() {
    return game_details.direction;
}
