/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-02-18  **
**              **
**    Edited:   **
**  2024-02-19  **
*****************/

/* Logging and other useful macros */

#ifndef _U_T_I_L_S_H_
#define _U_T_I_L_S_H_

#include <stdio.h>   // fprintf
#include <string.h>  // strcmp

#ifndef NDEBUG

/*-----------------------------LOGGING----------------------------------------*/

/* set to 1 if you want to log the function name */
#define LOG_FN_NAME 1

/* spaces to pad the function name */
#define FN_NAME_PAD 15

/* log levels */
#define DEBUG 0
#define INFO 1
#define WARNING 2
#define ERROR 3
#define FATAL 4  // fatal error

/* implicit log level (can be overridden by -DLOGLEVEL=lvl compiler flag) */
#ifndef LOGLEVEL
#define LOGLEVEL DEBUG
#endif  // #ifndef LOGLEVEL

#if LOG_FN_NAME

/* logs plain string (like puts) or does nothing if NDEBUG is defined */
#define log(lvl, msg) do { \
    if (lvl >= LOGLEVEL) { \
        fprintf(stderr, #lvl ": " __FILE__ ":%04d (%-*s): %s\n", __LINE__, \
        FN_NAME_PAD, __func__, msg); \
    } \
} while (0)

/* logs format (like printf) or does nothing if NDEBUG is defined */
#define logf(lvl, msg, ...) do { \
    if (lvl >= LOGLEVEL) { \
        fprintf(stderr, #lvl ": " __FILE__ ":%04d (%-*s): " msg "\n", \
        __LINE__, FN_NAME_PAD, __func__, __VA_ARGS__); \
    } \
} while (0)

#else  // if LOG_FN_NAME

/* logs plain string (like puts) or does nothing if NDEBUG is defined */
#define log(lvl, msg) do { \
    if (lvl >= LOGLEVEL) { \
        fprintf(stderr, #lvl ": " __FILE__ ":%04d: %s\n", __LINE__, msg); \
    } \
} while (0)

/* logs format (like printf) or does nothing if NDEBUG is defined */
#define logf(lvl, msg, ...) do { \
    if (lvl >= LOGLEVEL) { \
        fprintf(stderr, #lvl ": " __FILE__ ":%04d: " msg "\n", __LINE__, \
                __VA_ARGS__); \
    } \
} while (0)

#endif  // if LOG_FN_NAME

/*----------------------------------------------------------------------------*/

#else  // ifndef NDEBUG

#define log(lvl, msg) {}
#define logf(lvl, msg, ...) {}


#endif  // ifndef NDEBUG

/* absolute value */
#define abs(x) (((x) >= 0) ? (x) : (-(x)))

/* minimum */
#define min(x, y) ( (x) < (y) ? (x) : (y) )

/* maximum */
#define max(x, y) ( (x) > (y) ? (x) : (y) )

/* memory allocation fail message (print with `report_error()`)*/
#define MEMFAIL_MSG "couldn't allocate memory"

/* report an error (`msg`) to stderr with where in the code it occured */
#define report_error(msg) fprintf(stderr, __FILE__ ":%03d: %s\n", __LINE__, \
                                  msg)

/* my assert that does not halt the program, rather print ok or not ok */
#define my_assert(expr) if (expr) printf("    ok: " #expr "\n"); \
                             else printf("not ok: " #expr "\n")

/* returns true if two strings are not null and equal */
#define are_equal(str1, str2) \
((str1) != NULL && (str2) != NULL && strcmp((str1), (str2)) == 0)

#define not !
#define and &&
#define or ||

#endif  // ifndef _U_T_I_L_S_H_
