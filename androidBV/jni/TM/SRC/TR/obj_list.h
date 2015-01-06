// $Id: obj_list.h 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#ifndef EVCPE_OBJ_LIST_H_
#define EVCPE_OBJ_LIST_H_

#include <sys/queue.h>

struct evcpe_obj;

struct evcpe_obj_item {
	struct evcpe_obj *obj;
	TAILQ_ENTRY(evcpe_obj_item) entry;
};

TAILQ_HEAD(evcpe_obj_list, evcpe_obj_item);

struct evcpe_obj_item *evcpe_obj_list_get(
		struct evcpe_obj_list *list, int index);
struct evcpe_obj_item *evcpe_obj_list_get2(
		struct evcpe_obj_list *list, int index);

void evcpe_obj_list_clear(struct evcpe_obj_list *list);

int evcpe_obj_list_get_num(struct evcpe_obj_list *list);

#endif /* EVCPE_OBJ_LIST_H_ */
