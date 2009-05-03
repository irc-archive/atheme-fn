/*
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * freenode on-registration notices and default settings
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"freenode/regnotice", FALSE, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://www.freenode.net>"
);

static void nick_reg_notice(void *vptr);
static void chan_reg_notice(void *vptr);

void _modinit(module_t *m)
{
	hook_add_event("user_register");
	hook_add_hook("user_register", nick_reg_notice);
	hook_add_event("channel_register");
	hook_add_hook_first("channel_register", chan_reg_notice);
}

void _moddeinit(void)
{
	hook_del_hook("user_register", nick_reg_notice);
	hook_del_hook("channel_register", chan_reg_notice);
}

static void nick_reg_notice(void *vptr)
{
	myuser_t *mu = vptr;

	myuser_notice(nicksvs.nick, mu, " ");
	myuser_notice(nicksvs.nick, mu, "freenode is a service of Peer-Directed Projects Center, a");
	myuser_notice(nicksvs.nick, mu, "not-for-profit organisation registered in England and Wales.");
	myuser_notice(nicksvs.nick, mu, "For frequently-asked questions about the network, please see the");
	myuser_notice(nicksvs.nick, mu, "FAQ page (http://freenode.net/faq.shtml). Should you wish to");
	myuser_notice(nicksvs.nick, mu, "support the PDPC you can contribute to our current fundraiser at");
	myuser_notice(nicksvs.nick, mu, "http://freenode.net/pdpc_donations.shtml.");
}

static void chan_reg_notice(void *vptr)
{
	hook_channel_req_t *hdata = vptr;
	sourceinfo_t *si = hdata->si;
	mychan_t *mc = hdata->mc;

	if (si == NULL || mc == NULL)
		return;

	command_success_nodata(si, " ");
	command_success_nodata(si, "Channel guidelines can be found on the freenode website");
	command_success_nodata(si, "(http://freenode.net/channel_guidelines.shtml).");
	command_success_nodata(si, "freenode is a service of Peer-Directed Projects Center, a");
	command_success_nodata(si, "not-for-profit organisation registered in England and Wales.");
	if (mc->name[1] != '#')
	{
		command_success_nodata(si, "This is a primary namespace channel as per\n"
				"http://freenode.net/policy.shtml#primarychannels");
		command_success_nodata(si, "If you do not own this name, please consider\n"
				"dropping %s and using #%s instead.",
				mc->name, mc->name);
	}
	else
	{
		command_success_nodata(si, "This is an \"about\" channel as per");
		command_success_nodata(si, "http://freenode.net/policy.shtml#topicalchannels");
	}

	mc->mlock_on = CMODE_NOEXT | CMODE_TOPIC | mode_to_flag('c');
	mc->mlock_off |= CMODE_SEC;
	chanacs_change_simple(mc, si->smu, NULL, 0, CA_AUTOOP);
}
