#ifndef CHANEL_H
#define CHANEL_H

#include <stdint.h>
#include <stdio.h>

enum DUPLEX_CHANEL_STATE {
    NOT_OPENED = 0,
    OPENED,
    CH1_CLOSED,
    CH2_CLOSED,
    CLOSED,
};

#define UNDEFINED_PROCESS_ID -1

/** First process reads ch1[0] and writes to ch2[1],
second process reads ch2[0] and writes to ch1[1].
*/
struct duplex_chanel {
    int  ch1[2];
    int  ch2[2];
};

struct duplex_chanel_list {
    struct duplex_chanel* d_ch;
    // struct duplex_chanel_list* prev;
    struct duplex_chanel_list* next;
    int8_t first_id;
    int8_t second_id;
    enum DUPLEX_CHANEL_STATE state;
};


void log_all_pipes_desc(struct duplex_chanel_list* const ch_list, FILE* stream);

int open_chanel(struct duplex_chanel_list** const ch);

int open_chanels_for_processes(const short all_processes_amount,
                             const int8_t first_id, const int8_t last_id,
                             struct duplex_chanel_list** const ch_list);

int close_duplex_chanel(struct duplex_chanel_list* ch);

int close_chanel_part(struct duplex_chanel_list* ch, int ch_num);

int close_all_duplex_chanels(struct duplex_chanel_list** const ch_list);


#endif
