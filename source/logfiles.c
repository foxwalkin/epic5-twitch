/* $EPIC: logfiles.c,v 1.1 2002/09/03 11:43:12 jnelson Exp $ */
/*
 * logfiles.c - General purpose log files
 *
 * Copyright � 2002 EPIC Software Labs
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, the above paragraph (the one permitting redistribution),
 *    this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the author(s) may not be used to endorse or promote 
 *    products derived from this software without specific prior written
 *    permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE.
 */
#include "irc.h"
#include "log.h"
#include "vars.h"
#include "output.h"
#include "ircaux.h"
#include "alias.h"
#include "list.h"
#include "server.h"
#include "window.h"

static char *onoff[] = { "OFF", "ON" };

struct Logfile {
	struct Logfile *next;
	int	refnum;
	char *	name;
	char *	filename;
	FILE *	log;
	int	servref;

	WNickList *targets;
	int	level;

	char *	rewrite;
	int	mangler;
	char *	mangle_desc;
	int	active;
};

typedef struct Logfile Logfile;
Logfile *logfiles = NULL;
int	logref = 0;

Logfile *	new_logfile (void)
{
	Logfile *log;

	log = (Logfile *)new_malloc(sizeof(Logfile));
	log->next = logfiles;
	log->refnum = ++logref;
	log->name = m_sprintf("%d", log->refnum);
	log->filename = NULL;
	log->log = NULL;
	log->servref = from_server;
	log->targets = NULL;
	log->rewrite = NULL;
	log->mangler = 0;
	log->mangle_desc = NULL;
	log->active = 0;

	logfiles = log;
	return log;
}

void	delete_logfile (Logfile *log)
{
	Logfile *prev;
	WNickList *next;

	if (log == logfiles)
		logfiles = log->next;
	else
	{
		for (prev = logfiles; prev; prev = prev->next)
			if (prev->next == log)
				break;
		prev->next = log->next;
	}

	new_free(&log->name);
	if (log->active)
		do_log(0, log->filename, &log->log);
	new_free(&log->filename);

	while (log->targets)
	{
		next = log->targets->next;
		new_free(&log->targets->nick);
		new_free((char **)&log->targets);
		log->targets = next;
	}

	new_free(&log->rewrite);
	new_free(&log->mangle_desc);
}

Logfile *	get_log_by_desc (const char *desc)
{
	Logfile *log;

	if (is_number(desc))
	{
		int	number = atol(desc);

		for (log = logfiles; log; log = log->next)
			if (log->refnum == number)
				return log;
	}
	else
	{
		for (log = logfiles; log; log = log->next)
			if (!my_stricmp(log->name, desc))
				return log;
	}

	return NULL;
}

int	is_logfile_name_unique (const char *desc)
{
	Logfile *log;

	for (log = logfiles; log; log = log->next)
		if (!my_stricmp(log->name, desc))
			return 0;
	return 1;
}


/************************************************************************/
typedef Logfile *(*logfile_func) (Logfile *, char **);

static Logfile *logfile_add (Logfile *log, char **args);
static Logfile *logfile_describe (Logfile *log, char **args);
static Logfile *logfile_filename (Logfile *log, char **args);
static Logfile *logfile_kill (Logfile *log, char **args);
static Logfile *logfile_list (Logfile *log, char **args);
static Logfile *logfile_mangle (Logfile *log, char **args);
static Logfile *logfile_name (Logfile *log, char **args);
static Logfile *logfile_new (Logfile *log, char **args);
static Logfile *logfile_off (Logfile *log, char **args);
static Logfile *logfile_on (Logfile *log, char **args);
static Logfile *logfile_refnum (Logfile *log, char **args);
static Logfile *logfile_remove (Logfile *log, char **args);
static Logfile *logfile_rewrite (Logfile *log, char **args);

static Logfile *	logfile_add (Logfile *log, char **args)
{
        char            *ptr;
        WNickList       *new_w;
        char            *arg = next_arg(*args, args);

	if (!log)
	{
		say("ADD: You need to specify a logfile first");
		return NULL;
	}

        if (!arg)
                say("ADD: Add nicknames/channels to be logged to this file");

        else while (arg)
        {
                if ((ptr = strchr(arg, ',')))
                        *ptr++ = 0;
                if (!find_in_list((List **)&log->targets, arg, !USE_WILDCARDS))
                {
                        say("Added %s to log name list", arg);
                        new_w = (WNickList *)new_malloc(sizeof(WNickList));
                        new_w->nick = m_strdup(arg);
                        add_to_list((List **)&(log->targets), (List *)new_w);
                }
                else
                        say("%s already on log name list", arg);

                arg = ptr;
        }

        return log;
}

static Logfile *	logfile_describe (Logfile *log, char **args)
{
	WNickList *tmp;

	if (!log)
	{
		say("DESCRIBE: You need to specify a logfile first");
		return NULL;
	}

	say("Logfile refnum %d is %s", log->refnum, onoff[log->active]);
	say("\tLogical name: %s", log->name);
	say("\t    Filename: %s", log->filename ? log->filename : "<NONE>");
	for (tmp = log->targets; tmp; tmp = tmp->next)
		say("\t      Target: %s", tmp->nick);
	say("\tRewrite Rule: %s", log->rewrite ? log->rewrite : "<NONE>");
	say("\tMangle rules: %s", log->mangle_desc ? log->mangle_desc : "<NONE>");
	return log;
}

static Logfile *	logfile_filename (Logfile *log, char **args)
{
	char *	arg = next_arg(*args, args);

	if (!log)
	{
		say("FILENAME: You need to specify a logfile first");
		return NULL;
	}

	if (!arg)
	{
		if (log->filename)
			say("Log %s is attached to %s", log->name, log->filename);
		else
			say("Log %s does not have a filename", log->name);
		return log;
	}

	if (log->active)
		logfile_off(log, NULL);

	malloc_strcpy(&log->filename, arg);

	if (log->active)
		logfile_on(log, NULL);
	return log;
}

static Logfile *	logfile_kill (Logfile *log, char **args)
{
	delete_logfile(log);
	return NULL;
}

static Logfile *	logfile_list (Logfile *log, char **args)
{
	Logfile *l;
	char *nicks = NULL;
	WNickList *tmp;

	say("Logfiles:");
	for (l = logfiles; l; l = l->next)
	{
		for (tmp = log->targets; tmp; tmp = tmp->next)
			m_s3cat(&nicks, ",", tmp->nick);
		say("Log %2d [%10s] is %s, file [%20s] targets [%s]", 
			log->refnum, log->name, onoff[log->active],
			log->filename ? log->filename : "<NONE>", 
			nicks ? nicks : "<NONE>");
		new_free(&nicks);
	}

	return log;
}

static Logfile *	logfile_mangle (Logfile *log, char **args)
{
	char *	arg = next_arg(*args, args);

	if (!log)
	{
		say("MANGLE: You need to specify a logfile first");
		return NULL;
	}

	if (!arg)
		say("MANGLE: This logfile mangles %s", log->mangle_desc);
	else
	{
		new_free(&log->mangle_desc);
		log->mangler = parse_mangle(arg, log->mangler, &log->mangle_desc);
		say("MANGLE: Now mangling %s", log->mangle_desc);
	}
	return log;
}

static Logfile *	logfile_name (Logfile *log, char **args)
{
        char *arg;

	if (!log)
	{
		say("NAME: You need to specify a logfile first");
		return NULL;
	}

        if (!(arg = next_arg(*args, args)))
                say("You must specify a name for the logfile!");
	else
        {
                /* /log name -  unsets the window name */
                if (!strcmp(arg, "-"))
                        new_free(&log->name);

                /* /log name to existing name -- ignore this. */
                else if (log->name && (my_stricmp(log->name, arg) == 0))
                        return log;

                else if (is_logfile_name_unique(arg))
                        malloc_strcpy(&log->name, arg);

                else
                        say("%s is not unique!", arg);
        }

        return log;
}

static Logfile *	logfile_new (Logfile *log, char **args)
{
	return new_logfile();
}

static Logfile *	logfile_off (Logfile *log, char **args)
{
	if (!log)
	{
		say("OFF: You need to specify a logfile first");
		return NULL;
	}

	if (!log->filename)
	{
		say("OFF: You need to specify a filename for this log first");
		return log;
	}

	do_log(0, log->filename, &log->log);
	log->active = 0;
	return log;
}

static Logfile *	logfile_on (Logfile *log, char **args)
{
	if (!log)
	{
		say("ON: You need to specify a logfile first");
		return NULL;
	}

	if (!log->filename)
	{
		say("ON: You need to specify a filename for this log first");
		return log;
	}

	do_log(1, log->filename, &log->log);
	log->active = 1;
	return log;
}

static Logfile *	logfile_refnum (Logfile *log, char **args)
{
	char *arg = next_arg(*args, args);

	log = get_log_by_desc(arg);
	return log;
}

static Logfile *	logfile_remove (Logfile *log, char **args)
{
	char *arg = next_arg(*args, args);
	char            *ptr;
	WNickList       *new_nl;

	if (!log)
	{
		say("REMOVE: You need to specify a logfile first");
		return NULL;
	}

        if (!arg)
                say("Remove: Remove nicknames/channels logged to this file");

	else while (arg)
        {
		if ((ptr = strchr(arg, ',')) != NULL)
			*ptr++ = 0;

		if ((new_nl = (WNickList *)remove_from_list((List **)&(log->targets), arg)))
		{
			say("Removed %s from window name list", new_nl->nick);
			new_free(&new_nl->nick);
			new_free((char **)&new_nl);
		}
		else
			say("%s is not on the list for this log!", arg);

		arg = ptr;
        }

        return log;
}

static Logfile *	logfile_rewrite (Logfile *log, char **args)
{
        char *arg = new_next_arg(*args, args);

	if (!log)
	{
		say("REWRITE: You need to specify a logfile first");
		return NULL;
	}

	malloc_strcpy(&log->rewrite, arg);
	return log;
}

typedef struct logfile_ops_T {
	char *		command;
	logfile_func	func;
} logfile_ops;

static const logfile_ops options [] = {
	{ "ADD",	logfile_add		},
	{ "DESCRIBE",	logfile_describe	},
	{ "FILENAME",	logfile_filename	},
	{ "KILL",	logfile_kill		},
	{ "LIST",	logfile_list		},
	{ "MANGLE",	logfile_mangle		},
	{ "NAME",	logfile_name		},
	{ "NEW",	logfile_new		},
	{ "OFF",	logfile_off		},
	{ "ON",		logfile_on		},
	{ "REFNUM",	logfile_refnum		},
	{ "REMOVE",	logfile_remove		},
	{ "REWRITE",	logfile_rewrite		},
	{ NULL,		NULL			}
};

BUILT_IN_COMMAND(logcmd)
{
        char    *arg;
        int     nargs = 0;
        Logfile	*log = NULL;

        message_from(NULL, LOG_CURRENT);

        while ((arg = next_arg(args, &args)))
        {
                int i;
                int len = strlen(arg);

                if (*arg == '-' || *arg == '/')         /* Ignore - or / */
                        arg++, len--;

                for (i = 0; options[i].func ; i++)
                {
                        if (!my_strnicmp(arg, options[i].command, len))
                        {
                                log = options[i].func(log, &args);
                                nargs++;
                                break;
                        }
                }

                if (!options[i].func)
                {
			Logfile *s_log;
                        if ((s_log = get_log_by_desc(arg)))
                        {
                                nargs++;
				log = s_log;
                        }
                        else
                                yell("LOG: Invalid option: [%s]", arg);
                }
        }

        if (!nargs)
                logfile_list(NULL, NULL);
}

/****************************************************************************/
void	add_to_logs (int winref, int servref, const char *target, int level, const char *orig_str)
{
	Logfile *log;

	for (log = logfiles; log; log = log->next)
	{
		if (log->servref != -1 && (log->servref != servref))
			continue;

		if (log->level && (log->level & level) != 0)
			continue;

		if (!find_in_list((List **)&log->targets, target, USE_WILDCARDS))
			continue;

		/* OK!  We want to log it now! */
		add_to_log(log->log, winref, orig_str, log->mangler, log->rewrite);
	}
}

/*****************************************************************************/
#define EMPTY empty_string
#define EMPTY_STRING m_strdup(EMPTY)
#define RETURN_EMPTY return m_strdup(EMPTY)
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = my_atol(safe_new_next_arg(y, &y));}
#define GET_FLOAT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atof(safe_new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY(y); x = new_next_arg(y, &y);RETURN_IF_EMPTY(x);}
#define RETURN_MSTR(x) return ((x) ? (x) : EMPTY_STRING);
#define RETURN_STR(x) return m_strdup((x) ? (x) : EMPTY)
#define RETURN_INT(x) return m_strdup(ltoa((x)))

/* Used by function_logctl */
/*
 * $logctl(REFNUM log-desc)
 * $logctl(ADD log-desc [target])
 * $logctl(DELETE log-desc [target])
 * $logctl(GET <refnum> [LIST]
 * $logctl(SET <refnum> [ITEM] [VALUE])
 * $logctl(MATCH [pattern])
 * $logctl(PMATCH [pattern])
 *
 * [LIST] and [ITEM] are one of the following
 *	REFNUM		The refnum for the log (GET only)
 *	NAME		The logical name for the log
 *	FILENAME	The filename this log writes to
 *	SERVER		The server this log associates with (-1 for any)
 *	TARGETS		All of the targets for this log
 *	LEVEL		The Lastlog Level for this log
 *	REWRITE		The rewrite rule for this log
 *	MANGLE		The mangle rule for this log
 *	STATUS		1 if log is on, 0 if log is off.
 */
char *logctl	(char *input)
{
	int	refnum;
	char	*refstr;
	char	*listc;
	int	val;
	Logfile	*log;

	GET_STR_ARG(listc, input);
        if (!my_strnicmp(listc, "REFNUM", 1)) {
		GET_STR_ARG(refstr, input);
		if (!(log = get_log_by_desc(input)))
			RETURN_EMPTY;
		RETURN_INT(log->refnum);
        } else if (!my_strnicmp(listc, "ADD", 2)) {
		XXX
        } else if (!my_strnicmp(listc, "DELETE", 2)) {
		XXX
        } else if (!my_strnicmp(listc, "GET", 2)) {
                GET_STR_ARG(refstr, input);
		if (!(log = get_log_by_desc(input)))
			RETURN_EMPTY;

                GET_STR_ARG(listc, input);
                if (!my_strnicmp(listc, "REFNUM", 1)) {
			RETURN_INT(log->refnum);
                } else if (!my_strnicmp(listc, "NAME", 3)) {
			RETURN_STR(log->name);
                } else if (!my_strnicmp(listc, "FILENAME", 3)) {
			RETURN_STR(log->filename);
                } else if (!my_strnicmp(listc, "SERVER", 3)) {
			RETURN_INT(log->servref);
                } else if (!my_strnicmp(listc, "TARGETS", 3)) {
			for (tmp = log->targets; tmp; tmp = tmp->next)
				m_s3cat(&nicks, ",", tmp->nick);
                } else if (!my_strnicmp(listc, "LEVEL", 3)) {
			RETURN_STR(bits_to_lastlog_level(log->level));
                } else if (!my_strnicmp(listc, "REWRITE", 3)) {
			RETURN_STR(log->rewrite);
                } else if (!my_strnicmp(listc, "MANGLE", 3)) {
			RETURN_STR(log->mangle_desc);
                } else if (!my_strnicmp(listc, "STATUS", 3)) {
			RETURN_INT(log->active);
		}
        } else if (!my_strnicmp(listc, "SET", 1)) {
                GET_STR_ARG(refstr, input);
		if (!(log = get_log_by_desc(input)))
			RETURN_EMPTY;

		GET_STR_ARG(listc, input);
                if (!my_strnicmp(listc, "NAME", 3)) {
			logfile_name(log, &input);
			RETURN_INT(1);
                } else if (!my_strnicmp(listc, "FILENAME", 3)) {
			logfile_filename(log, &input);
			RETURN_INT(1);
                } else if (!my_strnicmp(listc, "SERVER", 3)) {
			logfile_server(log, &input);
			RETURN_INT(1);
                } else if (!my_strnicmp(listc, "TARGETS", 3)) {
			XXX Delete all current targets XXX
			XXX Add all targets in 'input' XXX
                } else if (!my_strnicmp(listc, "LEVEL", 3)) {
			logfile_level(log, &input);
                } else if (!my_strnicmp(listc, "REWRITE", 3)) {
			logfile_rewrite(log, &input);
                } else if (!my_strnicmp(listc, "MANGLE", 3)) {
			logfile_mangle(log, &input);
                } else if (!my_strnicmp(listc, "STATUS", 3)) {
			GET_INT_ARG(val, input);
			if (val)
				logfile_on(log, &input);
			else
				logfile_off(log, &input);
		}
        } else if (!my_strnicmp(listc, "MATCH", 1)) {
                RETURN_EMPTY;           /* Not implemented for now. */
        } else if (!my_strnicmp(listc, "PMATCH", 1)) {
                RETURN_EMPTY;           /* Not implemented for now. */
        } else
                RETURN_EMPTY;

        RETURN_EMPTY;
}

