// $Id: get_param_names.h 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#ifndef EVCPE_TRANSFERCOMPLETE_H_
#define EVCPE_TRANSFERCOMPLETE_H_

#include "data.h"
#include "fault.h"
#include <time.h>

struct evcpe_transfer {
	char flag;
	int result;
	struct tm *starttime;
	struct tm *completetime;
};

struct evcpe_transfercomplete {
	char commandkey[33];
	struct evcpe_fault fault;
	struct tm *starttime;
	struct tm *completetime;
};

struct evcpe_transfercomplete *evcpe_transfercomplete_new(void);

void evcpe_transfercomplete_free(struct evcpe_transfercomplete *method);

struct evcpe_transfercomplete_response {
};

struct evcpe_transfercomplete_response *evcpe_transfercomplete_response_new(void);

void evcpe_transfercomplete_response_free(
		struct evcpe_transfercomplete_response *method);


#endif /* EVCPE_GET_PARAM_NAMES_H_ */
