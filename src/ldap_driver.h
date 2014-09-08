/*
 * Authors: Petr Spacek <pspacek@redhat.com>
 *
 * Copyright (C) 2013  Red Hat
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

#ifndef LDAP_DRIVER_H_
#define LDAP_DRIVER_H_

#include <dns/diff.h>
#include <dns/types.h>

#include "util.h"

/* values shared by all LDAP database instances */
#define LDAP_DB_TYPE		dns_dbtype_zone
#define LDAP_DB_RDATACLASS	dns_rdataclass_in
#define LDAP_DB_ARGC		1

typedef struct ldapdb ldapdb_t;

isc_result_t
ldapdb_create(isc_mem_t *mctx, dns_name_t *name, dns_dbtype_t type,
	      dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
	      void *driverarg, dns_db_t **dbp) ATTR_NONNULL(1,2,6,8);

dns_db_t *
ldapdb_get_rbtdb(dns_db_t *db) ATTR_NONNULLS;

#endif /* LDAP_DRIVER_H_ */