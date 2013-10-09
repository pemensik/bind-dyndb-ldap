/*
 * Authors: Martin Nagy <mnagy@redhat.com>
 *          Adam Tkac   <atkac@redhat.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#error "Can't compile without config.h"
#endif

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/dynamic_db.h>
#include <dns/dbiterator.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/soa.h>
#include <dns/types.h>

#include <string.h> /* For memcpy */

#include "compat.h"
#include "ldap_driver.h"
#include "ldap_helper.h"
#include "ldap_convert.h"
#include "log.h"
#include "rdlist.h"
#include "util.h"
#include "zone_manager.h"

#ifdef HAVE_VISIBILITY
#define VISIBLE __attribute__((__visibility__("default")))
#else
#define VISIBLE
#endif

#define LDAPDB_MAGIC			ISC_MAGIC('L', 'D', 'P', 'D')
#define VALID_LDAPDB(ldapdb) \
	((ldapdb) != NULL && (ldapdb)->common.impmagic == LDAPDB_MAGIC)

typedef struct {
	dns_db_t			common;
	isc_refcount_t			refs;
	ldap_instance_t			*ldap_inst;
	dns_db_t			*rbtdb;
} ldapdb_t;

dns_db_t * ATTR_NONNULLS
ldapdb_get_rbtdb(dns_db_t *db) {
	ldapdb_t *ldapdb = (ldapdb_t *)db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return ldapdb->rbtdb;
}

/**
 * Get full DNS name from the node.
 *
 * @warning
 * The code silently expects that "node" came from RBTDB and thus
 * assumption dns_dbnode_t (from RBTDB) == dns_rbtnode_t is correct.
 *
 * This should work as long as we use only RBTDB and nothing else.
 */
static isc_result_t
ldapdb_name_fromnode(dns_dbnode_t *node, dns_name_t *name) {
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *) node;
	return dns_rbt_fullnamefromnode(rbtnode, name);
}

/*
 * Functions.
 *
 * Most of them don't need db parameter but we are checking if it is valid.
 * Invalid db parameter indicates bug in code.
 */

/* !!! Verify that omitting internal RBTDB will not cause havoc. */
static void
attach(dns_db_t *source, dns_db_t **targetp)
{
	ldapdb_t *ldapdb = (ldapdb_t *)source;

	REQUIRE(VALID_LDAPDB(ldapdb));

	isc_refcount_increment(&ldapdb->refs, NULL);
	*targetp = source;
}

/* !!! Verify that internal RBTDB cannot leak somehow. */
static void ATTR_NONNULLS
free_ldapdb(ldapdb_t *ldapdb)
{
#ifdef RBTDB_DEBUG
#define PATH "/var/named/dump/"
	isc_result_t result;
	dns_dbversion_t *version = NULL;
	char filename[DNS_NAME_FORMATSIZE + sizeof(PATH)] = PATH;

	dns_name_format(&ldapdb->common.origin, filename + sizeof(PATH) - 1,
			DNS_NAME_FORMATSIZE);
	dns_db_currentversion(ldapdb->rbtdb, &version);
	log_error("dump to '%s' started", filename);
	result = dns_db_dump2(ldapdb->rbtdb, version, filename, dns_masterformat_text);
	log_error_r("dump to '%s' finished", filename);
	dns_db_closeversion(ldapdb->rbtdb, &version, ISC_FALSE);
#undef PATH
#endif
	dns_db_detach(&ldapdb->rbtdb);
	dns_name_free(&ldapdb->common.origin, ldapdb->common.mctx);
	isc_mem_putanddetach(&ldapdb->common.mctx, ldapdb, sizeof(*ldapdb));
}

/* !!! Verify that omitting internal RBTDB will not cause havoc. */
static void
detach(dns_db_t **dbp)
{
	ldapdb_t *ldapdb = (ldapdb_t *)(*dbp);
	unsigned int refs;

	REQUIRE(VALID_LDAPDB(ldapdb));

	isc_refcount_decrement(&ldapdb->refs, &refs);

	if (refs == 0)
		free_ldapdb(ldapdb);

	*dbp = NULL;
}



/**
 * This method should never be called, because LDAP DB is "persistent".
 * See ispersistent() function.
 */

/* !!! This could be required for optimizations (like on-disk cache). */
static isc_result_t
beginload(dns_db_t *db, dns_addrdatasetfunc_t *addp, dns_dbload_t **dbloadp)
{

	UNUSED(db);
	UNUSED(addp);
	UNUSED(dbloadp);

	fatal_error("ldapdb: method beginload() should never be called");

	/* Not reached */
	return ISC_R_SUCCESS;
}

/**
 * This method should never be called, because LDAP DB is "persistent".
 * See ispersistent() function.
 */

/* !!! This could be required for optimizations (like on-disk cache). */
static isc_result_t
endload(dns_db_t *db, dns_dbload_t **dbloadp)
{

	UNUSED(db);
	UNUSED(dbloadp);

	fatal_error("ldapdb: method endload() should never be called");

	/* Not reached */
	return ISC_R_SUCCESS;
}


/* !!! This could be required for optimizations (like on-disk cache). */
static isc_result_t
dump(dns_db_t *db, dns_dbversion_t *version, const char *filename
#if LIBDNS_VERSION_MAJOR >= 31
     , dns_masterformat_t masterformat
#endif
     )
{

	UNUSED(db);
	UNUSED(version);
	UNUSED(filename);
#if LIBDNS_VERSION_MAJOR >= 31
	UNUSED(masterformat);
#endif

	fatal_error("ldapdb: method dump() should never be called");

	/* Not reached */
	return ISC_R_SUCCESS;
}

static void
currentversion(dns_db_t *db, dns_dbversion_t **versionp)
{
	ldapdb_t *ldapdb = (ldapdb_t *)db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_currentversion(ldapdb->rbtdb, versionp);
}

static isc_result_t
newversion(dns_db_t *db, dns_dbversion_t **versionp)
{
	ldapdb_t *ldapdb = (ldapdb_t *)db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_newversion(ldapdb->rbtdb, versionp);
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp)
{
	ldapdb_t *ldapdb = (ldapdb_t *)db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_attachversion(ldapdb->rbtdb, source, targetp);
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp, isc_boolean_t commit)
{
	ldapdb_t *ldapdb = (ldapdb_t *)db;

	REQUIRE(VALID_LDAPDB(ldapdb));
	dns_db_closeversion(ldapdb->rbtdb, versionp, commit);
}

static isc_result_t
findnode(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
	 dns_dbnode_t **nodep)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findnode(ldapdb->rbtdb, name, create, nodep);
}

static isc_result_t
find(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
     dns_dbnode_t **nodep, dns_name_t *foundname, dns_rdataset_t *rdataset,
     dns_rdataset_t *sigrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_find(ldapdb->rbtdb, name, version, type, options, now,
			   nodep, foundname, rdataset, sigrdataset);
}

static isc_result_t
findzonecut(dns_db_t *db, dns_name_t *name, unsigned int options,
	    isc_stdtime_t now, dns_dbnode_t **nodep, dns_name_t *foundname,
	    dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findzonecut(ldapdb->rbtdb, name, options, now, nodep,
				  foundname, rdataset, sigrdataset);
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_attachnode(ldapdb->rbtdb, source, targetp);

}

static void
detachnode(dns_db_t *db, dns_dbnode_t **targetp)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_detachnode(ldapdb->rbtdb, targetp);
}

static isc_result_t
expirenode(dns_db_t *db, dns_dbnode_t *node, isc_stdtime_t now)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_expirenode(ldapdb->rbtdb, node, now);
}

static void
printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_printnode(ldapdb->rbtdb, node, out);
}

static isc_result_t
createiterator(dns_db_t *db,
#if LIBDNS_VERSION_MAJOR >= 50
	       unsigned int options,
#else
	       isc_boolean_t relative_names,
#endif
	       dns_dbiterator_t **iteratorp)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));
#if LIBDNS_VERSION_MAJOR >= 50
	return dns_db_createiterator(ldapdb->rbtdb, options, iteratorp);
#else
	return dns_db_createiterator(ldapdb->rbtdb, relative_names, iteratorp);
#endif
}

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers, isc_stdtime_t now,
	     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findrdataset(ldapdb->rbtdb, node, version, type, covers,
				   now, rdataset, sigrdataset);
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     isc_stdtime_t now, dns_rdatasetiter_t **iteratorp)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_allrdatasets(ldapdb->rbtdb, node, version, now, iteratorp);
}

/* TODO: Add 'tainted' flag to the LDAP instance if something went wrong. */
static isc_result_t
addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	    isc_stdtime_t now, dns_rdataset_t *rdataset, unsigned int options,
	    dns_rdataset_t *addedrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;
	dns_fixedname_t fname;
	dns_rdatalist_t *rdlist = NULL;
	isc_result_t result;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_fixedname_init(&fname);

	CHECK(dns_db_addrdataset(ldapdb->rbtdb, node, version, now,
				  rdataset, options, addedrdataset));

	CHECK(ldapdb_name_fromnode(node, dns_fixedname_name(&fname)));
	result = dns_rdatalist_fromrdataset(rdataset, &rdlist);
	INSIST(result == ISC_R_SUCCESS);
	CHECK(write_to_ldap(dns_fixedname_name(&fname), ldapdb->ldap_inst, rdlist));

cleanup:
	return result;

}

static isc_result_t
node_isempty(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     isc_stdtime_t now, isc_boolean_t *isempty) {
	dns_rdatasetiter_t *rds_iter = NULL;
	dns_fixedname_t fname;
	char buff[DNS_NAME_FORMATSIZE];
	isc_result_t result;

	dns_fixedname_init(&fname);

	CHECK(ldapdb_name_fromnode(node, dns_fixedname_name(&fname)));

	result = dns_db_allrdatasets(db, node, version, now, &rds_iter);
	if (result == ISC_R_NOTFOUND) {
		*isempty = ISC_TRUE;
	} else if (result == ISC_R_SUCCESS) {
		result = dns_rdatasetiter_first(rds_iter);
		if (result == ISC_R_NOMORE) {
			*isempty = ISC_TRUE;
			result = ISC_R_SUCCESS;
		} else if (result == ISC_R_SUCCESS) {
			*isempty = ISC_FALSE;
			result = ISC_R_SUCCESS;
		} else if (result != ISC_R_SUCCESS) {
			dns_name_format(dns_fixedname_name(&fname),
					buff, DNS_NAME_FORMATSIZE);
			log_error_r("dns_rdatasetiter_first() failed during "
				    "node_isempty() for name '%s'", buff);
		}
		dns_rdatasetiter_destroy(&rds_iter);
	} else {
		dns_name_format(dns_fixedname_name(&fname),
				buff, DNS_NAME_FORMATSIZE);
		log_error_r("dns_db_allrdatasets() failed during "
			    "node_isempty() for name '%s'", buff);
	}

cleanup:
	return result;
}

/* TODO: Add 'tainted' flag to the LDAP instance if something went wrong. */
static isc_result_t
subtractrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		 dns_rdataset_t *rdataset, unsigned int options,
		 dns_rdataset_t *newrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;
	dns_fixedname_t fname;
	dns_rdatalist_t *rdlist = NULL;
	isc_boolean_t empty_node = ISC_FALSE;
	isc_result_t substract_result;
	isc_result_t result;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_fixedname_init(&fname);

	result = dns_db_subtractrdataset(ldapdb->rbtdb, node, version,
					 rdataset, options, newrdataset);
	/* DNS_R_NXRRSET mean that whole RRset was deleted. */
	if (result != ISC_R_SUCCESS && result != DNS_R_NXRRSET)
		goto cleanup;

	substract_result = result;
	/* TODO: Could it create some race-condition? What about unprocessed
	 * changes in synrepl queue? */
	if (substract_result == DNS_R_NXRRSET) {
		CHECK(node_isempty(ldapdb->rbtdb, node, version, 0,
				   &empty_node));
	}

	result = dns_rdatalist_fromrdataset(rdataset, &rdlist);
	INSIST(result == ISC_R_SUCCESS);
	CHECK(ldapdb_name_fromnode(node, dns_fixedname_name(&fname)));
	CHECK(remove_values_from_ldap(dns_fixedname_name(&fname), ldapdb->ldap_inst,
				      rdlist, empty_node));

cleanup:
	if (result == ISC_R_SUCCESS)
		result = substract_result;
	return result;
}

/* This function is usually not called for non-cache DBs, so we don't need to
 * care about performance.
 * TODO: Add 'tainted' flag to the LDAP instance if something went wrong. */
static isc_result_t
deleterdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	       dns_rdatatype_t type, dns_rdatatype_t covers)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;
	dns_fixedname_t fname;
	isc_boolean_t empty_node;
	char attr_name[LDAP_ATTR_FORMATSIZE];
	dns_rdatalist_t fake_rdlist; /* required by remove_values_from_ldap */
	isc_result_t result;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_fixedname_init(&fname);
	dns_rdatalist_init(&fake_rdlist);

	result = dns_db_deleterdataset(ldapdb->rbtdb, node, version, type,
				       covers);
	/* DNS_R_UNCHANGED mean that there was no RRset with given type. */
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/* TODO: Could it create some race-condition? What about unprocessed
	 * changes in synrepl queue? */
	CHECK(node_isempty(ldapdb->rbtdb, node, version, 0, &empty_node));
	CHECK(ldapdb_name_fromnode(node, dns_fixedname_name(&fname)));

	if (empty_node == ISC_TRUE) {
		CHECK(remove_entry_from_ldap(dns_fixedname_name(&fname),
					     ldapdb->ldap_inst));
	} else {
		CHECK(rdatatype_to_ldap_attribute(type, attr_name,
						  LDAP_ATTR_FORMATSIZE));
		CHECK(remove_attr_from_ldap(dns_fixedname_name(&fname),
					      ldapdb->ldap_inst, attr_name));
	}

cleanup:
	return result;
}

static isc_boolean_t
issecure(dns_db_t *db)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_issecure(ldapdb->rbtdb);
}

static unsigned int
nodecount(dns_db_t *db)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_nodecount(ldapdb->rbtdb);
}

/**
 * Return TRUE, because database does not need to be loaded from disk
 * or written to disk.
 *
 * !!! This could be required for optimizations (like on-disk cache).
 */
static isc_boolean_t
ispersistent(dns_db_t *db)
{
	UNUSED(db);

	return ISC_TRUE;
}

static void
overmem(dns_db_t *db, isc_boolean_t overmem)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_overmem(ldapdb->rbtdb, overmem);
}

static void
settask(dns_db_t *db, isc_task_t *task)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_settask(ldapdb->rbtdb, task);
}

#if LIBDNS_VERSION_MAJOR >= 31
static isc_result_t
getoriginnode(dns_db_t *db, dns_dbnode_t **nodep)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_getoriginnode(ldapdb->rbtdb, nodep);
}
#endif /* LIBDNS_VERSION_MAJOR >= 31 */

#if LIBDNS_VERSION_MAJOR >= 45
static void
transfernode(dns_db_t *db, dns_dbnode_t **sourcep, dns_dbnode_t **targetp)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_transfernode(ldapdb->rbtdb, sourcep, targetp);

}
#endif /* LIBDNS_VERSION_MAJOR >= 45 */

#if LIBDNS_VERSION_MAJOR >= 50
static isc_result_t
getnsec3parameters(dns_db_t *db, dns_dbversion_t *version,
			  dns_hash_t *hash, isc_uint8_t *flags,
			  isc_uint16_t *iterations,
			  unsigned char *salt, size_t *salt_length)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_getnsec3parameters(ldapdb->rbtdb, version, hash, flags,
					 iterations, salt, salt_length);

}

static isc_result_t
findnsec3node(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
	      dns_dbnode_t **nodep)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findnsec3node(ldapdb->rbtdb, name, create, nodep);
}

static isc_result_t
setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, isc_stdtime_t resign)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_setsigningtime(ldapdb->rbtdb, rdataset, resign);
}

static isc_result_t
getsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, dns_name_t *name)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_getsigningtime(ldapdb->rbtdb, rdataset, name);
}

static void
resigned(dns_db_t *db, dns_rdataset_t *rdataset,
		dns_dbversion_t *version)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_resigned(ldapdb->rbtdb, rdataset, version);
}

static isc_boolean_t
isdnssec(dns_db_t *db)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_isdnssec(ldapdb->rbtdb);
}
#endif /* LIBDNS_VERSION_MAJOR >= 50 */

#if LIBDNS_VERSION_MAJOR >= 45
static dns_stats_t *
getrrsetstats(dns_db_t *db) {
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_getrrsetstats(ldapdb->rbtdb);

}
#endif /* LIBDNS_VERSION_MAJOR >= 45 */

#if LIBDNS_VERSION_MAJOR >= 82
static isc_result_t
rpz_enabled(dns_db_t *db, dns_rpz_st_t *st)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_rpz_enabled(ldapdb->rbtdb, st);
}

static void
rpz_findips(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
		   dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *version,
		   dns_rdataset_t *ardataset, dns_rpz_st_t *st,
		   dns_name_t *query_qname)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	dns_db_rpz_findips(rpz, rpz_type, zone, ldapdb->rbtdb, version,
			   ardataset, st, query_qname);
}
#endif /* LIBDNS_VERSION_MAJOR >= 82 */

#if LIBDNS_VERSION_MAJOR >= 90
static isc_result_t
findnodeext(dns_db_t *db, dns_name_t *name,
		   isc_boolean_t create, dns_clientinfomethods_t *methods,
		   dns_clientinfo_t *clientinfo, dns_dbnode_t **nodep)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findnodeext(ldapdb->rbtdb, name, create, methods,
				  clientinfo, nodep);
}

static isc_result_t
findext(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
	       dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	       dns_dbnode_t **nodep, dns_name_t *foundname,
	       dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
	       dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	ldapdb_t *ldapdb = (ldapdb_t *) db;

	REQUIRE(VALID_LDAPDB(ldapdb));

	return dns_db_findext(ldapdb->rbtdb, name, version, type, options, now,
			      nodep, foundname, methods, clientinfo, rdataset,
			      sigrdataset);
}
#endif /* LIBDNS_VERSION_MAJOR >= 90 */

static dns_dbmethods_t ldapdb_methods = {
	attach,
	detach,
	beginload,
	endload,
	dump,
	currentversion,
	newversion,
	attachversion,
	closeversion,
	findnode,
	find,
	findzonecut,
	attachnode,
	detachnode,
	expirenode,
	printnode,
	createiterator,
	findrdataset,
	allrdatasets,
	addrdataset,
	subtractrdataset,
	deleterdataset,
	issecure,
	nodecount,
	ispersistent,
	overmem,
	settask,
#if LIBDNS_VERSION_MAJOR >= 31
	getoriginnode,
#endif /* LIBDNS_VERSION_MAJOR >= 31 */
#if LIBDNS_VERSION_MAJOR >= 45
	transfernode,
#if LIBDNS_VERSION_MAJOR >= 50
	getnsec3parameters,
	findnsec3node,
	setsigningtime,
	getsigningtime,
	resigned,
	isdnssec,
#endif /* LIBDNS_VERSION_MAJOR >= 50 */
	getrrsetstats,
#endif /* LIBDNS_VERSION_MAJOR >= 45 */
#if LIBDNS_VERSION_MAJOR >= 82
	rpz_enabled,
	rpz_findips,
#endif /* LIBDNS_VERSION_MAJOR >= 82 */
#if LIBDNS_VERSION_MAJOR >= 90
	findnodeext,
	findext
#endif /* LIBDNS_VERSION_MAJOR >= 90 */
};

isc_result_t ATTR_NONNULLS
dns_ns_buildrdata(dns_name_t *origin, dns_name_t *ns_name,
		   dns_rdataclass_t rdclass,
		   unsigned char *buffer,
		   dns_rdata_t *rdata) {
	dns_rdata_ns_t ns;
	isc_buffer_t rdatabuf;

	REQUIRE(origin != NULL);
	REQUIRE(ns_name != NULL);

	memset(buffer, 0, DNS_SOA_BUFFERSIZE);
	isc_buffer_init(&rdatabuf, buffer, DNS_SOA_BUFFERSIZE);

	ns.common.rdtype = dns_rdatatype_ns;
	ns.common.rdclass = rdclass;
	ns.mctx = NULL;
	dns_name_init(&ns.name, NULL);
	dns_name_clone(ns_name, &ns.name);

	return (dns_rdata_fromstruct(rdata, rdclass, dns_rdatatype_ns,
				      &ns, &rdatabuf));
}

/*
 * Create an SOA record for a newly-created zone
 */
static isc_result_t ATTR_NONNULLS
add_soa(isc_mem_t *mctx, dns_name_t *origin, dns_db_t *db) {
	isc_result_t result;
	dns_rdata_t rdata_soa = DNS_RDATA_INIT;
	dns_rdata_t rdata_ns = DNS_RDATA_INIT;
	unsigned char buf_soa[DNS_SOA_BUFFERSIZE];
	unsigned char buf_ns[DNS_SOA_BUFFERSIZE];
	dns_fixedname_t ns_name;
	dns_fixedname_t m_name;
	dns_dbversion_t *ver = NULL;
	dns_difftuple_t *tp_soa = NULL;
	dns_difftuple_t *tp_ns = NULL;
	dns_diff_t diff;

	dns_diff_init(mctx, &diff);
	result = dns_db_newversion(db, &ver);
	if (result != ISC_R_SUCCESS) {
		log_error_r("add_soa:dns_db_newversion");
		goto failure;
	}

	/* Build SOA record */
	dns_fixedname_init(&m_name);
	dns_name_fromstring(dns_fixedname_name(&m_name), "pspacek.brq.redhat.com.", 0, mctx);
	result = dns_soa_buildrdata(dns_fixedname_name(&m_name), dns_rootname, dns_rdataclass_in,
				    0, 0, 0, 0, 0, buf_soa, &rdata_soa);
	if (result != ISC_R_SUCCESS) {
		log_error_r("add_soa:dns_soa_buildrdata");
		goto failure;
	}

	result = dns_difftuple_create(mctx, DNS_DIFFOP_ADD, origin, 3600,
				      &rdata_soa, &tp_soa);
	if (result != ISC_R_SUCCESS) {
			log_error_r("add_soa:dns_difftuple_create");
			goto failure;
	}
	dns_diff_append(&diff, &tp_soa);

	/* Build NS record */
	dns_fixedname_init(&ns_name);
	dns_name_fromstring(dns_fixedname_name(&ns_name), "localhost.", 0, mctx);

	result = dns_ns_buildrdata(origin, dns_fixedname_name(&ns_name),
			dns_rdataclass_in,
			buf_ns,
			&rdata_ns);
	if (result != ISC_R_SUCCESS) {
			log_error_r("add_soa:dns_ns_buildrdata");
			goto failure;
	}
	result = dns_difftuple_create(mctx, DNS_DIFFOP_ADD, origin, 3600,
				      &rdata_ns, &tp_ns);
	if (result != ISC_R_SUCCESS) {
			log_error_r("add_soa:dns_difftuple_create");
			goto failure;
	}
	dns_diff_append(&diff, &tp_ns);

	result = dns_diff_apply(&diff, db, ver);
	if (result != ISC_R_SUCCESS) {
			log_error_r("add_soa:dns_difftuple_create");
			goto failure;
	}

failure:
	dns_diff_clear(&diff);
	if (ver != NULL)
		dns_db_closeversion(db, &ver, ISC_TF(result == ISC_R_SUCCESS));

	return (result);
}

static isc_result_t
ldapdb_create(isc_mem_t *mctx, dns_name_t *name, dns_dbtype_t type,
	      dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
	      void *driverarg, dns_db_t **dbp)
{
	ldapdb_t *ldapdb = NULL;
	isc_result_t result;

	UNUSED(driverarg); /* Currently we don't need any data */

	/* Database instance name. */
	REQUIRE(argc > 0);

	REQUIRE(type == dns_dbtype_zone);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(dbp != NULL && *dbp == NULL);

	CHECKED_MEM_GET_PTR(mctx, ldapdb);
	ZERO_PTR(ldapdb);

	isc_mem_attach(mctx, &ldapdb->common.mctx);

	dns_name_init(&ldapdb->common.origin, NULL);
	isc_ondestroy_init(&ldapdb->common.ondest);

	ldapdb->common.magic = DNS_DB_MAGIC;
	ldapdb->common.impmagic = LDAPDB_MAGIC;

	ldapdb->common.methods = &ldapdb_methods;
	ldapdb->common.attributes = 0;
	ldapdb->common.rdclass = rdclass;

	CHECK(dns_name_dupwithoffsets(name, mctx, &ldapdb->common.origin));

	CHECK(isc_refcount_init(&ldapdb->refs, 1));
	CHECK(manager_get_ldap_instance(argv[0], &ldapdb->ldap_inst));

	CHECK(dns_db_create(mctx, "rbt", name, dns_dbtype_zone,
			    dns_rdataclass_in, 0, NULL, &ldapdb->rbtdb));
	CHECK(add_soa(mctx, name, ldapdb->rbtdb));

	*dbp = (dns_db_t *)ldapdb;

	return ISC_R_SUCCESS;

cleanup:
	if (ldapdb != NULL) {
		if (dns_name_dynamic(&ldapdb->common.origin))
			dns_name_free(&ldapdb->common.origin, mctx);

		isc_mem_putanddetach(&ldapdb->common.mctx, ldapdb,
				     sizeof(*ldapdb));
	}

	return result;
}

static dns_dbimplementation_t *ldapdb_imp;
const char *ldapdb_impname = "dynamic-ldap";


VISIBLE isc_result_t
dynamic_driver_init(isc_mem_t *mctx, const char *name, const char * const *argv,
		    dns_dyndb_arguments_t *dyndb_args)
{
	dns_dbimplementation_t *ldapdb_imp_new = NULL;
	isc_result_t result;

	REQUIRE(name != NULL);
	REQUIRE(argv != NULL);
	REQUIRE(dyndb_args != NULL);

	log_debug(2, "registering dynamic ldap driver for %s.", name);

	/*
	 * We need to discover what rdataset methods does
	 * dns_rdatalist_tordataset use. We then make a copy for ourselves
	 * with the exception that we modify the disassociate method to free
	 * the rdlist we allocate for it in clone_rdatalist_to_rdataset().
	 */

	/* Register new DNS DB implementation. */
	result = dns_db_register(ldapdb_impname, &ldapdb_create, NULL, mctx,
				 &ldapdb_imp_new);
	if (result != ISC_R_SUCCESS && result != ISC_R_EXISTS)
		return result;
	else if (result == ISC_R_SUCCESS)
		ldapdb_imp = ldapdb_imp_new;

	/* Finally, create the instance. */
	result = manager_create_db_instance(mctx, name, argv, dyndb_args);

	return result;
}

VISIBLE void
dynamic_driver_destroy(void)
{
	/* Only unregister the implementation if it was registered by us. */
	if (ldapdb_imp != NULL)
		dns_db_unregister(&ldapdb_imp);

	destroy_manager();
}
