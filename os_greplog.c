/*
 * Copyright (c) 2007-2008 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Searches through the logs.
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

command_t os_greplog = { "GREPLOG", N_("Searches through the logs."), PRIV_CHAN_AUSPEX, 3, os_cmd_greplog };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_greplog, os_cmdtree);
	help_addentry(os_helptree, "GREPLOG", "help/oservice/greplog", NULL);
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
	const char *service, *pattern, *baselog;
	int maxdays, matches = -1, matches1, day, days, lines, linesv;
	FILE *in;
	char str[1024];
	char *p, *q;
	const char *commands_log = "var/commands.log"; /* XXX */
	const char *account_log = "var/account.log"; /* XXX */
	char logfile[256];
	time_t t;
	struct tm tm;
	list_t loglines = { NULL, NULL, 0 };
	node_t *n, *tn;

	/* require both user and channel auspex */
	if (!has_priv(si, PRIV_USER_AUSPEX))
	{
		command_fail(si, fault_noprivs, _("You do not have %s privilege."), PRIV_USER_AUSPEX);
		return;
	}

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GREPLOG");
		command_fail(si, fault_needmoreparams, _("Syntax: GREPLOG <service> <pattern> [days]"));
		return;
	}

	service = parv[0];
	pattern = parv[1];

	if (parc >= 3)
	{
		days = atoi(parv[2]);
		maxdays = !strcmp(service, "*") ? 120 : 30;
		if (days < 0 || days > maxdays)
		{
			command_fail(si, fault_badparams, _("Too many days, maximum is %d."), maxdays);
			return;
		}
	}
	else
		days = 0;

	snoop("GREPLOG: \2%s\2 \2%s\2 by \2%s\2", service, pattern, get_oper_name(si));

	for (day = 0; day <= days; day++)
	{
		baselog = !strcmp(service, "*") ? account_log : commands_log;
		if (day == 0)
			strlcpy(logfile, baselog, sizeof logfile);
		else
		{
			t = CURRTIME - day * 86400;
			tm = *gmtime(&t);
			snprintf(logfile, sizeof logfile, "%s.%04u%02u%02u",
					baselog, tm.tm_year + 1900,
					tm.tm_mon + 1, tm.tm_mday);
		}
		in = fopen(logfile, "r");
		if (in == NULL)
		{
			command_success_nodata(si, "Failed to open log file %s", logfile);
			continue;
		}
		if (matches == -1)
			matches = 0;
		matches1 = matches;
		lines = linesv = 0;
		while (fgets(str, sizeof str, in) != NULL)
		{
			p = strchr(str, '\n');
			if (p != NULL)
				*p = '\0';
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
			if (strcmp(service, "*") && strcasecmp(service, p))
				continue;
			*q++ = ' ';
			if (match(pattern, q))
				continue;
			matches++;
			node_add_head(sstrdup(str), node_create(), &loglines);
			if (matches > MAXMATCHES)
			{
				n = loglines.tail;
				node_del(n, &loglines);
				free(n->data);
				node_free(n);
				break;
			}
		}
		fclose(in);
		matches = matches1;
		LIST_FOREACH_SAFE(n, tn, loglines.head)
		{
			p = n->data;
			matches++;
			command_success_nodata(si, "[%d] %s", matches, p);
			free(p);
			node_free(n);
		}
		if (matches == 0 && lines > linesv && lines > 0)
			command_success_nodata(si, "Log file may be corrupted, %d/%d unexpected lines", lines - linesv, lines);
		if (matches >= MAXMATCHES)
		{
			command_success_nodata(si, "Too many matches, halting search");
			break;
		}
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
