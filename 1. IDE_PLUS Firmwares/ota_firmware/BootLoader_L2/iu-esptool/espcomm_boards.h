/***************************************************************************
 ***
 ***    espcomm_boards.h
 ***    - board-specific upload methods interface
 ***
 **/

#ifndef ESPCOMM_BOARDS_H
#define ESPCOMM_BOARDS_H

struct espcomm_board_;
typedef struct espcomm_board_ espcomm_board_t;

espcomm_board_t* espcomm_board_by_name(const char* name);
espcomm_board_t* espcomm_board_first();
espcomm_board_t* espcomm_board_next(espcomm_board_t* board);

const char* espcomm_board_name(espcomm_board_t* board);
void espcomm_board_reset_into_bootloader(espcomm_board_t* board);
void espcomm_board_reset_into_app(espcomm_board_t* board);


#endif//ESPCOMM_BOARDS_H
