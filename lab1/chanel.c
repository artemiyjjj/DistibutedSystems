#include "chanel.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


int open_chanel(struct duplex_chanel_list** const dc_list) {
    struct duplex_chanel_list* dc = *dc_list;
    dc -> d_ch = malloc(sizeof(struct duplex_chanel));
    if (dc -> d_ch == NULL) {
        return -1;
    }
    if (pipe(dc -> d_ch -> ch1) != 0 || fcntl(dc -> d_ch -> ch1[0], F_SETFL, O_NONBLOCK) != 0) {
        free(dc -> d_ch);
        return 1;
    }
    if (pipe(dc -> d_ch -> ch2) != 0 || fcntl(dc -> d_ch -> ch2[0], F_SETFL, O_NONBLOCK) != 0) {
        free(dc -> d_ch);
        return 1;
    }
    dc -> state = OPENED;
    return 0;
}

int close_chanel_part(struct duplex_chanel_list* dc, int ch_num) {
    if (ch_num == 1) {
        dc -> state = CH1_CLOSED;
        return close(dc -> d_ch -> ch1[0]) | close(dc -> d_ch -> ch2[1]);
    } else if (ch_num == 2) {
        dc -> state = CH2_CLOSED;
        return close(dc -> d_ch -> ch2[0]) | close(dc -> d_ch -> ch1[1]);
    } else {
        return -1;
    }
}

// redo to pass ** and delete from list
int close_duplex_chanel(struct duplex_chanel_list* dc) {
    switch (dc -> state) {
        case OPENED: {
            close_chanel_part(dc, 1);
            close_chanel_part(dc, 2);
            break;
        };
        case CH1_CLOSED: {
            close_chanel_part(dc, 2);
            break;
        };
        case CH2_CLOSED: {
            close_chanel_part(dc, 1);
            break;
        };
        case CLOSED: {
            break;
        };
        default: {
            return -1;
        }
    }
    free(dc -> d_ch);
    return 0;
}

int close_all_duplex_chanels(struct duplex_chanel_list** const ch_list) {
    struct duplex_chanel_list* next_ch_list;
    struct duplex_chanel_list* cur_list = *ch_list;

    while (cur_list != NULL) {
        next_ch_list = cur_list -> next;
        if (close_duplex_chanel(cur_list) != 0) {
            return -1;
        }
        cur_list = next_ch_list;
    }
    return 0;
}


void log_all_pipes_desc(struct duplex_chanel_list* const ch_list, FILE* stream) {
    struct duplex_chanel_list* cur_ch = ch_list;
    
    while (cur_ch != NULL) {
        struct duplex_chanel* d_ch = cur_ch -> d_ch;
        if (cur_ch -> state == OPENED) {
            fprintf(stream, "Chanel for %d:{fd[0]=%d, fd[1]=%d}, chanel for %d:{fd[2]=%d, fd[3]=%d}\n",
                    cur_ch -> first_id, d_ch -> ch1[0], d_ch -> ch2[1],
                    cur_ch -> second_id, d_ch -> ch2[0], d_ch -> ch1[1]);
        }
        cur_ch = cur_ch -> next;
    }
    fflush(stream);
}

/**
 * @brief Allocate new list entity and add to the end of list
 * 
 * @param d_ch start of list
 * @param new_list pointer to newly created list entity
 * @return 0 on success, any non-zero value on error
 */
static int add_duplex_chanel_list(struct duplex_chanel_list** const d_ch,
                                  struct duplex_chanel_list** new_list) {
    struct duplex_chanel_list*  prev_ch_list = NULL;
    struct duplex_chanel_list*  cur_ch_list = *d_ch;

    while (cur_ch_list != NULL) {
        prev_ch_list = cur_ch_list;
        cur_ch_list = cur_ch_list -> next;
    }

    *new_list = malloc(sizeof(struct duplex_chanel_list));
    if (*new_list == NULL) {
        return -1;
    }
    if (prev_ch_list != NULL) {
        prev_ch_list -> next = *new_list;
    } else { // if list was empty
        *d_ch = *new_list;
    }
    return 0;
}

int open_chanels_for_processes(const short all_processes_amount,
                             const int8_t first_id, const int8_t last_id,
                             struct duplex_chanel_list **const ch_list) {
    if (first_id >= last_id || first_id < 0) {
        return -1;
    }
    const short     needed_chanels_amount = (all_processes_amount * (all_processes_amount - 1)) / 2;
    short           created_chanels = 0;
    int8_t        cur_first = first_id;
    int8_t        cur_second = first_id + 1;
    struct duplex_chanel_list*  cur_d_ch_list = *ch_list;

    while (created_chanels != needed_chanels_amount) {
        if (add_duplex_chanel_list(ch_list, &cur_d_ch_list) != 0) {
            return -2;
        }
        if (open_chanel(&cur_d_ch_list) != 0) {
            return 1;
        }
        created_chanels++;
        cur_d_ch_list -> next = NULL;
        cur_d_ch_list -> first_id = cur_first;
        cur_d_ch_list -> second_id = cur_second;
        if (cur_second == last_id) {
            cur_first++;
            cur_second = cur_first + 1;
        } else {
            cur_second++;
        }
    }

    return 0;
}

