/*
 * Copyright (c) 2007 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ChanServ ACCESS command
 *
 * $Id$
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"freenode/cs_access", FALSE, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://www.freenode.net>"
);

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access = { "ACCESS", "Manipulates channel access lists.",
                         AC_NONE, 4, cs_cmd_access };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");
	help_addentry(cs_helptree, "ACCESS", "help/cservice/access", NULL);
	command_add(&cs_access, cs_cmdtree);
}

void _moddeinit()
{
	help_delentry(cs_helptree, "ACCESS");
	command_delete(&cs_access, cs_cmdtree);
}

static void compat_cmd(sourceinfo_t *si, const char *cmdname, char *channel, char *arg1, char *arg2, char *arg3)
{
	int newparc;
	char *newparv[5];
	command_t *cmd;

	newparv[0] = channel;
	newparv[1] = arg1;
	newparv[2] = arg2;
	newparv[3] = arg3;
	newparv[4] = NULL;
	/* this assumes arg3!=NULL implies arg2!=NULL implies arg1!=NULL */
	newparc = 1 + (arg1 != NULL) + (arg2 != NULL) + (arg3 != NULL);
	cmd = command_find(si->service->cmdtree, cmdname);
	if (cmd != NULL)
		command_exec(si->service, si, cmd, newparc, newparv);
	else
		command_fail(si, fault_unimplemented, _("Command \2%s\2 not loaded?"), cmdname);
}

static const char *get_template_name(mychan_t *mc, unsigned int level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, METADATA_CHANNEL, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if (level == flags_to_bitmask(ss, chanacs_flags, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}
	if (level == chansvs.ca_sop)
		return "SOP";
	if (level == chansvs.ca_aop)
		return "AOP";
	/* if vop==hop, prefer vop */
	if (level == chansvs.ca_vop)
		return "VOP";
	if (level == chansvs.ca_hop)
		return "HOP";
	return NULL;
}

static void access_list(sourceinfo_t *si, mychan_t *mc, int parc, char *parv[])
{
	node_t *n;
	chanacs_t *ca;
	const char *str1, *str2;
	int i = 1;
	int operoverride = 0;

	/* Copied from modules/chanserv/flags.c */
	/* Note: This overrides the normal need of +A access */
	command_success_nodata(si, _("Entry Nickname/Host          Flags"));
	command_success_nodata(si, "----- ---------------------- -----");

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		/* Change: don't show akicks */
		if (ca->level == CA_AKICK)
			continue;
		str1 = get_template_name(mc, ca->level);
		str2 = ca->ts ? time_ago(ca->ts) : "?";
		if (str1 != NULL)
			command_success_nodata(si, _("%-5d %-22s %s (%s) [modified %s ago]"), i, ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags), str1,
				str2);
		else
			command_success_nodata(si, _("%-5d %-22s %s [modified %s ago]"), i, ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags),
				str2);
		i++;
	}

	command_success_nodata(si, "----- ---------------------- -----");
	command_success_nodata(si, _("End of \2%s\2 FLAGS listing."), mc->name);
	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "%s ACCESS LIST (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "%s ACCESS LIST", mc->name);
}

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan, *cmd;
	mychan_t *mc;
	char killit[] = "-*";
	char deftemplate[] = "OP";
	char defaccess[] = "+votirA";

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> ADD|DEL|LIST [nick] [level]"));
		return;
	}
	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ACCESS");
		command_fail(si, fault_badparams, _("Syntax: ACCESS <#channel> ADD|DEL|LIST [nick] [level]"));
		return;
	}

	mc = mychan_find(chan);
	if (mc == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
		return;
	}

	if (!strcasecmp(cmd, "LIST"))
		access_list(si, mc, parc - 2, parv + 2);
	else if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> ADD|DEL <nick> [level]"));
		return;
	}
	else if (!strcasecmp(cmd, "ADD"))
		compat_cmd(si, "FLAGS", chan, parv[2], parc > 3 ? parv[3] : (get_template_flags(mc, deftemplate) ? deftemplate : defaccess), NULL);
	else if (!strcasecmp(cmd, "DEL"))
		compat_cmd(si, "FLAGS", chan, parv[2], killit, NULL);
	else
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
}
