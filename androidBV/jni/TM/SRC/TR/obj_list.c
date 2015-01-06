// $Id: obj_list.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj.h"

#include "obj_list.h"
#include "util.h"
#include "attr.h"

void evcpe_obj_list_clear(struct evcpe_obj_list *list)
{
	struct evcpe_obj_item *item;

	while((item = TAILQ_FIRST(list))) {
		if (item->obj) {
			evcpe_obj_free(item->obj);
		}
		TAILQ_REMOVE(list, item, entry);
		free(item);
	}
}

struct evcpe_obj_item *evcpe_obj_list_get(
		struct evcpe_obj_list *list, int index)
{
	struct evcpe_obj_item *item;
	int i;

	i = 0;
	TAILQ_FOREACH(item, list, entry) {
		if (i == index) {
			return item;
			break;
		}
		i ++;
	}
	return NULL;
}

struct evcpe_obj_item *evcpe_obj_list_get2(
		struct evcpe_obj_list *list, int index)
{
	struct evcpe_obj_item *item;
	struct evcpe_obj *obj;
	struct evcpe_attr *attr;
	const char *value;
	long lvalue;
	unsigned int len;
       int rc;

	TAILQ_FOREACH(item, list, entry) {
		obj = item->obj;
		if ((rc = evcpe_obj_get(obj, EVCPE_NUM_STR, strlen(EVCPE_NUM_STR), &attr))) {
			evcpe_error(__func__, "failed to get attribute: %.*s",
					strlen(EVCPE_NUM_STR), EVCPE_NUM_STR);
			return NULL;
		}
		if ((rc = evcpe_attr_get(attr, &value, &len))) {
			evcpe_error(__func__, "failed to get value: %s", EVCPE_NUM_STR);
			return NULL;
		}
		evcpe_atol(value, len, &lvalue);
		if (lvalue == index) {
			return item;
			break;
		}
	}
	return NULL;
}

int evcpe_obj_list_get_num(
		struct evcpe_obj_list *list)
{
	struct evcpe_obj_item *item;
       
	int i;

	i = 0;
	TAILQ_FOREACH(item, list, entry) {
		i ++;
	}
	return i;
}

