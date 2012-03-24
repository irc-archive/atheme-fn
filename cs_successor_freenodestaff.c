/*
 * Copyright (c) 2012 Marien Zwart <marien.zwart@gmail.com>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Forces the successor for single-# channels to be freenode-staff,
 * if an account by that name exists.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"freenode/cs_successor_freenodestaff", false, _modinit, _moddeinit,
	"$Id$",
	"freenode <http://freenode.net>"
);

static void channel_pick_successor_hook(hook_channel_succession_req_t *req)
{
	return_if_fail(req != NULL);
	return_if_fail(req->mc != NULL);

	/* Leave double-# channels alone. */
	if (req->mc->name[0] == '#' && req->mc->name[1] == '#')
		return;

	/* Use freenode-staff if it exists.
	 * If myuser_find returns NULL the normal successor logic is used.
	 * If some other user of this hook picked a successor
	 * we intentionally overrule it.
	 */
	req->mu = myuser_find("freenode-staff");
}

void _modinit(module_t *m)
{
	hook_add_first_channel_pick_successor(channel_pick_successor_hook);
}

void _moddeinit()
{
	hook_del_channel_pick_successor(channel_pick_successor_hook);
}
