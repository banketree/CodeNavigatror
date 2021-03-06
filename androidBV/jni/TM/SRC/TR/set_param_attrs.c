// $Id: set_param_attrs.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#include "set_param_attrs.h"

struct evcpe_set_param_attrs *evcpe_set_param_attrs_new(void)
{
	struct evcpe_set_param_attrs *method;

	if (!(method = calloc(1, sizeof(struct evcpe_set_param_attrs)))) {
		evcpe_error(__func__, "failed to calloc evcpe_set_param_attrs");
		return NULL;
	}
	evcpe_set_param_attr_list_init(&method->parameter_list);
	return method;
}

void evcpe_set_param_attrs_free(struct evcpe_set_param_attrs *method)
{
	if (!method) return;

	evcpe_set_param_attr_list_clear(&method->parameter_list);
	free(method);
}
