/*
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Searches through the log file.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"freenode/os_greplog", FALSE, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://www.freenode.net>"
);

list_t *os_cmdtree, *os_helptree;

static void os_cmd_greplog(sourceinfo_t *si, int parc, char *parv[]);

command_t os_greplog = { "GREPLOG", N_("Searches through the log file."), PRIV_ADMIN, 2, os_cmd_greplog };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_greplog, os_cmdtree);
	help_addentry(os_helptree, "GREPLOG", "help/operserv/greplog", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_greplog, os_cmdtree);
	help_delentry(os_helptree, "GREPLOG");
}

#define MAXMATCHES 100

/* GREPLOG <service> <mask> */
static void os_cmd_greplog(sourceinfo_t *si, int parc, char *parv[])
{
	const char *service, *pattern;
	int matches = 0, lines, linesv;
	FILE *in;
	char str[1024];
	char *p, *q;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GREPLOG");
		command_fail(si, fault_needmoreparams, _("Syntax: GREPLOG <service> <pattern>"));
		return;
	}

	service = parv[0];
	pattern = parv[1];

	snoop("GREPLOG: \2%s\2 \2%s\2 by \2%s\2", service, pattern, get_oper_name(si));

	in = fopen(log_path, "r");
	if (in != NULL)
	{
		lines = linesv = 0;
		while (fgets(str, sizeof str, in) != NULL)
		{
			lines++;
			p = *str == '[' ? strchr(str, ']') : NULL;
			if (p == NULL)
				continue;
			p++;
			if (*p++ != ' ')
				continue;
			q = strchr(p, ' ');
			if (q == NULL)
				continue;
			linesv++;
			*q = '\0';
			if (strcasecmp(service, p))
				continue;
			*q++ = ' ';
			if (match(pattern, q))
				continue;
			matches++;
			command_success_nodata(si, "[%d] %s", matches, str);
			/* Do not remove, it stops inflooping when matching
			 * the NOTICE from raw traffic logging
			 */
			if (matches >= MAXMATCHES)
			{
				command_success_nodata(si, "Too many matches, halting search");
				break;
			}
		}
		fclose(in);
		if (matches == 0 && lines > linesv && lines > 0)
			command_success_nodata(si, "Log file may be corrupted, %d/%d unexpected lines", lines - linesv, lines);
	}
	else
	{
		command_fail(si, fault_nosuch_target, "Failed to open log file");
		matches = -1;
	}

	logcommand(si, CMDLOG_ADMIN, "GREPLOG %s %s (%d matches)", service, pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No lines matched pattern \2%s\2"), pattern);
	else if (matches > 0)
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
