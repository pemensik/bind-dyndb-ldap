/*
 * Authors: Martin Nagy <mnagy@redhat.com>
 *
 * Copyright (C) 2008, 2009  Red Hat
 * see file 'COPYING' for use and warranty information
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 or later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LD_LOG_H_
#define _LD_LOG_H_

#include <isc/error.h>

#define fatal_error(...) \
    isc_error_fatal(__FILE__, __LINE__, __VA_ARGS__)

#define log_bug(fmt, ...) \
	log_error("bug in %s(): " fmt, __func__,##__VA_ARGS__)

#define log_error_r(fmt, ...) \
	log_error(fmt ": %s", ##__VA_ARGS__, isc_result_totext(result))

/* Basic logging functions */
void log_debug(int level, const char *format, ...) ISC_FORMAT_PRINTF(2, 3);
void log_error(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

#endif /* !_LD_LOG_H_ */