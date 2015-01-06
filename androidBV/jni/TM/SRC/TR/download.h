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

#ifndef EVCPE_DOWNLOAD_H_
#define EVCPE_DOWNLOAD_H_

#include "data.h"

struct evcpe_download {
	char commandkey[33];
	char file_type[65];
	char url[257];
	char username[257];
	char password[257];
	unsigned int file_size;
	char target_filename[257];
	unsigned int delay_seconds;
	char success_url[257];
	char failure_url[257];
};

struct evcpe_download *evcpe_download_new(void);

void evcpe_download_free(struct evcpe_download *method);

struct evcpe_download_response {
	int status;
	struct timeval starttime;
	struct timeval completetime;
};

struct evcpe_download_response *evcpe_download_response_new(void);

void evcpe_download_response_free(
		struct evcpe_download_response *method);

#endif /* EVCPE_GET_PARAM_NAMES_H_ */
