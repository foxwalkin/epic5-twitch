/*
 * levels.h: Unified levels system
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1997, 2003 EPIC Software Labs
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef __levels_h__
#define __levels_h__

#define LEVEL_NONE	0
#define LEVEL_CURRENT	0
#define LEVEL_CRAP	1
#define LEVEL_PUBLIC	2
#define LEVEL_MSG	3
#define LEVEL_NOTICE	4
#define LEVEL_WALL	5
#define LEVEL_WALLOP	6
#define LEVEL_NOTE	7
#define LEVEL_OPNOTE	8
#define LEVEL_SNOTE	9
#define LEVEL_ACTION	10
#define LEVEL_DCC	11
#define LEVEL_CTCP	12
#define LEVEL_INVITE	13
#define LEVEL_JOIN	14
#define LEVEL_NICK	15
#define LEVEL_TOPIC	16
#define LEVEL_PART	17
#define LEVEL_QUIT	18
#define LEVEL_KICK	19
#define LEVEL_MODE	20
#define LEVEL_USER1	21
#define LEVEL_USER2	22
#define LEVEL_USER3	23
#define LEVEL_USER4	24
#define LEVEL_HELP	25
#define LEVEL_ALL       0x7FFFFFFF
#define NUMBER_OF_LEVELS	26

static  const char *level_types[NUMBER_OF_LEVELS] =
{
        "NONE",
        "CRAP",
        "PUBLIC",
        "MSG",
        "NOTICE",
        "WALL",
        "WALLOP",
        "NOTE",
        "OPNOTE",
        "SNOTE",
        "ACTION",
        "DCC",
        "CTCP",
        "INVITE",
        "JOIN",
        "NICK",
        "TOPIC",
        "PART",
        "QUIT",
        "KICK",
        "MODE",
        "USER1",
        "USER2",
        "USER3",
        "USER4",
        "HELP"
};

/* Convert a #define LEVEL_* integer into a mask */
#define _Y(x) ( x == LEVEL_ALL? x : x == LEVEL_NONE? x : (1 << ( x - 1 )))

/* Convert a level name (ie, _X(CRAP)) into a mask */
#define _X(x) (_Y(LEVEL_ ## x))

typedef struct Mask { int mask; } Mask;

#endif
