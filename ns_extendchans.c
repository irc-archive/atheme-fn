/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows giving a higher channel limit to a user.
 * Derived from nickserv/vhost.
 *
 * $Id$
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"freenode/ns_extendchans", FALSE, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://www.freenode.net>"
);

list_t *ns_cmdtree, *ns_helptree;

static void extendchans_on_identify(void *vptr);
static void ns_cmd_extendchans(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_listextendchans(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_extendchans = { "EXTENDCHANS", N_("Enables or disables extendchans for a user."), PRIV_USER_ADMIN, 2, ns_cmd_extendchans };
command_t ns_listextendchans = { "LISTEXTENDCHANS", N_("Lists accounts with extendchans enabled."), PRIV_USER_AUSPEX, 1, ns_cmd_listextendchans };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	hook_add_event("user_identify");
	hook_add_hook("user_identify", extendchans_on_identify);
	command_add(&ns_extendchans, ns_cmdtree);
	command_add(&ns_listextendchans, ns_cmdtree);
	help_addentry(ns_helptree, "EXTENDCHANS", "help/nickserv/extendchans", NULL);
	help_addentry(ns_helptree, "LISTEXTENDCHANS", "help/nickserv/listextendchans", NULL);
}

void _moddeinit(void)
{
	hook_del_hook("user_identify", extendchans_on_identify);
	command_delete(&ns_extendchans, ns_cmdtree);
	command_delete(&ns_listextendchans, ns_cmdtree);
	help_delentry(ns_helptree, "EXTENDCHANS");
	help_delentry(ns_helptree, "LISTEXTENDCHANS");
}

static void do_extendchans(user_t *u, boolean_t enable)
{
	sts(":%s MODE %s +*", nicksvs.nick, nicksvs.nick);
	sts(":%s MODE %s %cu", nicksvs.nick, u->nick, enable ? '+' : '-');
	sts(":%s MODE %s -*", nicksvs.nick, nicksvs.nick);
}

static void do_extendchans_all(myuser_t *mu, boolean_t enable)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;
		do_extendchans(u, enable);
	}
}

/* EXTENDCHANS <nick> [ON|OFF] */
static void ns_cmd_extendchans(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EXTENDCHANS");
		command_fail(si, fault_needmoreparams, _("Syntax: EXTENDCHANS <nick> [ON|OFF]"));
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
		return;
	}

	if (parc == 1)
	{
		command_success_nodata(si, "ExtendChans is \2%s\2 for %s",
				metadata_find(mu, METADATA_USER, "private:extendchans") ? "ON" : "OFF",
				mu->name);
		return;
	}

	if (!strcasecmp(parv[1], "OFF"))
	{
		metadata_delete(mu, METADATA_USER, "private:extendchans");
		command_success_nodata(si, _("Disabled extendchans for \2%s\2."), mu->name);
		snoop("EXTENDCHANS:OFF: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "EXTENDCHANS %s OFF", mu->name);
		do_extendchans_all(mu, FALSE);
	}
	else if (!strcasecmp(parv[1], "ON"))
	{
		metadata_add(mu, METADATA_USER, "private:extendchans", "1");
		command_success_nodata(si, _("Enabled extendchans for \2%s\2."),
				mu->name);
		snoop("EXTENDCHANS:ON: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "EXTENDCHANS %s ON", mu->name);
		do_extendchans_all(mu, TRUE);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "EXTENDCHANS");
		command_fail(si, fault_badparams, _("Syntax: EXTENDCHANS <nick> [ON|OFF]"));
		return;
	}
	return;
}

static void ns_cmd_listextendchans(sourceinfo_t *si, int parc, char *parv[])
{
	const char *pattern;
	mowgli_dictionary_iteration_state_t state;
	myuser_t *mu;
	metadata_t *md;
	int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	snoop("LISTEXTENDCHANS: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
	MOWGLI_DICTIONARY_FOREACH(mu, &state, mulist)
	{
		md = metadata_find(mu, METADATA_USER, "private:extendchans");
		if (md == NULL)
			continue;
		if (!match(pattern, mu->name))
		{
			command_success_nodata(si, "- %-30s", mu->name);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTEXTENDCHANS %s (%d matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No extendchans users matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

static void extendchans_on_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	metadata_t *md;

	if (!(md = metadata_find(mu, METADATA_USER, "private:extendchans")))
		return;

	do_extendchans(u, TRUE);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
