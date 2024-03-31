/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-03  **
*****************/

/**
 * a module for stdin processing functions
*/
#include "ipk24chat.h"  // conf_t
#include "msg.h"  // msg_t

#ifndef _S_H_E_L_L_H_
#define _S_H_E_L_L_H_

/* initial buffer for the line (if needed, getline reallocs, so this doesnt
matter all that much) */
#define INIT_LINE_BUFSIZE 1024


/**
 * @brief Parses /auth command and returns corresponding msg_t
 * if the command is invalid, returns NULL, if there is an internal error,
 * returns NULL and sets `error_occured` to true. Returned msg_t has
 * `id` field set to 0 and is expected to be then edited by the caller
 * @note returned msg_t needs to be freed with `msg_dtor`
*/
msg_t *parse_auth(const char *line, bool *error_occured);

/**
 * same as parse_auth, but for /join
*/
msg_t *parse_join(const char *line, bool *error_occured);

/**
 * parses /rename, returns a pointer to the display name string
 * if the command is invalid, returns NULL, if there is an internal error,
 * returns NULL and sets `error_occured` to true.
*/
char *parse_rename(char *line, bool *error_occurred);


bool is_auth(const char *s);
bool is_join(const char *s);
bool is_rename(const char *s);
bool is_help(const char *s);

/* strips the trailing line feed of a string if there is one */
void rstriplf(char *s);

/* returns if `s` starts with `prefix` */
bool startswith(const char *s, const char *prefix);

/**
 * returns if a line is valid to be sent as a MSG message
 * if it's not, prints appropriate error message to stderr
*/
bool message_valid(const char *line);

#endif  // ifndef _S_H_E_L_L_H_