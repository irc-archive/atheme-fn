/*
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * freenode on-registration notices
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
	hook_add_hook("channel_register", chan_reg_notice);
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
	myuser_notice(nicksvs.nick, mu, "Freenode is a service of Peer-Directed Projects Center, an");
	myuser_notice(nicksvs.nick, mu, "IRS 501(c)(3) (tax-exempt) charitable and educational organization.");
	myuser_notice(nicksvs.nick, mu, "For frequently-asked questions about the network, please see the");
	myuser_notice(nicksvs.nick, mu, "FAQ page (http://freenode.net/faq.shtml).");
}

static void chan_reg_notice(void *vptr)
{
	mychan_t *mc = vptr;
	myuser_t *mu = mc->founder; /* XXX what if they're logged in twice */

	myuser_notice(chansvs.nick, mu, " ");
	myuser_notice(chansvs.nick, mu, "Channel guidelines can be found on the freenode website");
	myuser_notice(chansvs.nick, mu, "(http://freenode.net/channel_guidelines.shtml).");
	myuser_notice(chansvs.nick, mu, "Freenode is a service of Peer-Directed Projects Center, an");
	myuser_notice(chansvs.nick, mu, "IRS 501(c)(3) (tax-exempt) charitable and educational organization.");
}