// $Id: get_param_names.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
/*
 * Copyright (C) 2010 AXIM Communications Inc.
 * Copyright (C) 2010 Cedric Shih <cedric.shih@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

#include "log.h"

#include "transfercomplete.h"

struct evcpe_transfercomplete *evcpe_transfercomplete_new(void)
{
	struct evcpe_transfercomplete *transfercomplete;
	evcpe_debug(__func__, "constructing evcpe_transfercomplete");
	if (!(transfercomplete = calloc(1, sizeof(struct evcpe_transfercomplete)))) {
		evcpe_error(__func__, "failed to calloc evcpe_transfercomplete");
		return NULL;
	}
    
	return transfercomplete;
}

void evcpe_transfercomplete_free(struct evcpe_transfercomplete *transfercomplete)
{
	if (!transfercomplete) return;
	evcpe_debug(__func__, "destructing evcpe_transfercomplete");
    
	free(transfercomplete);
}

struct evcpe_transfercomplete_response *evcpe_transfercomplete_response_new(void)
{
	struct evcpe_transfercomplete_response *method;
	evcpe_debug(__func__, "constructing evcpe_transfercomplete_response");
	if (!(method = calloc(1, sizeof(struct evcpe_transfercomplete_response)))) {
		evcpe_error(__func__, "failed to calloc evcpe_transfercomplete");
		return NULL;
	}
	return method;
}

void evcpe_transfercomplete_response_free(
		struct evcpe_transfercomplete_response *method)
{
	if (!method) return;
	evcpe_debug(__func__, "destructing evcpe_transfercomplete_response");
	free(method);
}


