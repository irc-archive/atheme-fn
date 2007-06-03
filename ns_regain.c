/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGAIN function.
 * Derived from modules/nickserv/ghost.c.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"freenode/ns_regain", FALSE, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://www.freenode.net>"
);

static void ns_cmd_regain(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_regain = { "REGAIN", N_("Reclaims use of a nickname."), AC_NONE, 2, ns_cmd_regain };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_regain, ns_cmdtree);
	help_addentry(ns_helptree, "REGAIN", "help/nickserv/regain", NULL);
}

void _moddeinit()
{
	command_delete(&ns_regain, ns_cmdtree);
	help_delentry(ns_helptree, "REGAIN");
}

void ns_cmd_regain(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *target = parv[0];
	char *password = parv[1];
	user_t *target_u;
	mynick_t *mn;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "REGAIN");
		return;
	}

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGAIN");
		command_fail(si, fault_needmoreparams, _("Syntax: REGAIN <target> [password]"));
		return;
	}

	if (nicksvs.no_nick_ownership)
		mn = NULL, mu = myuser_find(target);
	else
	{
		mn = mynick_find(target);
		mu = mn != NULL ? mn->owner : NULL;
	}
	target_u = user_find_named(target);
	if (!mu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}

	if (!target_u)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
		return;
	}
	else if (target_u == si->su)
	{
		command_fail(si, fault_badparams, _("You may not ghost yourself."));
		return;
	}

	if ((!nicksvs.no_nick_ownership && mn && mu == si->smu) || /* we're identified under their nick's account */
			(!nicksvs.no_nick_ownership && password && mn && verify_password(mu, password))) /* we have their nick's password */
	{
		logcommand(si, CMDLOG_DO, "REGAIN %s!%s@%s", target_u->nick, target_u->user, target_u->vhost);

		skill(nicksvs.nick, target, "REGAIN command used by %s",
				si->su != NULL && !strcmp(si->su->user, target_u->user) && !strcmp(si->su->vhost, target_u->vhost) ? si->su->nick : get_source_mask(si));
		user_delete(target_u);

		command_success_nodata(si, _("\2%s\2 has been regained."), target);
		fnc_sts(si->service->me, si->su, target, FNC_REGAIN);

		/* don't update the nick's last seen time */
		mu->lastlogin = CURRTIME;

		return;
	}

	if (password && mu)
	{
		logcommand(si, CMDLOG_DO, "failed REGAIN %s (bad password)", target);
		command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), mu->name);
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed REGAIN %s (invalid login)", target);
		command_fail(si, fault_noprivs, _("You may not regain \2%s\2."), target);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
