/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Thursday, September 18, 1997
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5G package. Source files outside the
 *              H5G package should include H5Gprivate.h instead.
 */
#ifndef H5G_PACKAGE
#error "Do not include this file outside the H5G package!"
#endif

#ifndef _H5Gpkg_H
#define _H5Gpkg_H

/* Get package's private header */
#include "H5Gprivate.h"

/* Other private headers needed by this file */
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5B2private.h"	/* v2 B-trees				*/
#include "H5HFprivate.h"	/* Fractal heaps			*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5SLprivate.h"	/* Skip lists				*/

/**************************/
/* Package Private Macros */
/**************************/

/* Standard length of fractal heap ID for link */
#define H5G_DENSE_FHEAP_ID_LEN  7

/*
 * During name lookups (see H5G_traverse()) we sometimes want information about
 * a symbolic link or a mount point.  The normal operation is to follow the
 * symbolic link or mount point and return information about its target.
 */
#define H5G_TARGET_NORMAL	0x0000
#define H5G_TARGET_SLINK	0x0001
#define H5G_TARGET_MOUNT	0x0002
#define H5G_TARGET_UDLINK	0x0004
#define H5G_CRT_INTMD_GROUP	0x0008

/****************************/
/* Package Private Typedefs */
/****************************/

/*
 * Various types of object header information can be cached in a symbol
 * table entry (it's normal home is the object header to which the entry
 * points).  This datatype determines what (if anything) is cached in the
 * symbol table entry.
 */
typedef enum H5G_cache_type_t {
    H5G_CACHED_ERROR	= -1, 	/*force enum to be signed		     */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0               */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'                       */
    H5G_CACHED_SLINK	= 2, 	/*symbolic link				     */

    H5G_NCACHED                 /*THIS MUST BE LAST                          */
} H5G_cache_type_t;

/*
 * A symbol table entry caches these parameters from object header
 * messages...  The values are entered into the symbol table when an object
 * header is created (by hand) and are extracted from the symbol table with a
 * callback function registered in H5O_init_interface().  Be sure to update
 * H5G_ent_decode(), H5G_ent_encode(), and H5G_ent_debug() as well.
 */
typedef union H5G_cache_t {
    struct {
        haddr_t btree_addr;             /*file address of symbol table B-tree*/
        haddr_t heap_addr;              /*file address of stab name heap     */
    } stab;

    struct {
	size_t	lval_offset;		/*link value offset		     */
    } slink;
} H5G_cache_t;

/*
 * A symbol table entry.  The two important fields are `name_off' and
 * `header'.  The remaining fields are used for caching information that
 * also appears in the object header to which this symbol table entry
 * points.
 */
typedef struct H5G_entry_t {
    hbool_t     dirty;                  /*entry out-of-date?                 */
    H5G_cache_type_t type;              /*type of information cached         */
    H5G_cache_t cache;                  /*cached data from object header     */
    size_t      name_off;               /*offset of name within name heap    */
    haddr_t     header;                 /*file address of object header      */
    H5F_t       *file;                  /*file to which this obj hdr belongs */
} H5G_entry_t;

/*
 * A symbol table node is a collection of symbol table entries.  It can
 * be thought of as the lowest level of the B-link tree that points to
 * a collection of symbol table entries that belong to a specific symbol
 * table or group.
 */
typedef struct H5G_node_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    unsigned nsyms;                     /*number of symbols                  */
    H5G_entry_t *entry;                 /*array of symbol table entries      */
} H5G_node_t;

/*
 * Shared information for all open group objects
 */
struct H5G_shared_t {
    int fo_count;                   /* open file object count */
    hbool_t mounted;                /* Group is mount point */
};

/*
 * A group handle passed around through layers of the library within and
 * above the H5G layer.
 */
struct H5G_t {
    H5G_shared_t *shared;               /* Shared file object data */
    H5O_loc_t oloc;                     /* Object location for group */
    H5G_name_t path;                    /* Group hierarchy path   */
};

/* Link iteration operator for internal library callbacks */
typedef herr_t (*H5G_lib_iterate_t)(const H5O_link_t *lnk, void *op_data);

/* Describe kind of callback to make for each link */
typedef struct {
    enum {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        H5G_LINK_OP_OLD,                /* "Old" application callback */
#endif /* H5_NO_DEPRECATED_SYMBOLS */
        H5G_LINK_OP_NEW                 /* "New" application callback */
    } op_type;
    union {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        H5G_iterate_t op_old;           /* "Old" application callback for each link */
#endif /* H5_NO_DEPRECATED_SYMBOLS */
        H5L_iterate_t op_new;           /* "New" application callback for each link */
    } op_func;
} H5G_link_iterate_t;

/* Data structure to hold table of links for a group */
typedef struct {
    size_t      nlinks;         /* # of links in table */
    H5O_link_t *lnks;           /* Pointer to array of links */
} H5G_link_table_t;

/*
 * Common data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 *
 * It's also used for B-tree iterators which require no additional info.
 *
 */
typedef struct H5G_bt_common_t {
    /* downward */
    const char  *name;                  /*points to temporary memory         */
    H5HL_t *heap;                       /*symbol table heap		     */
} H5G_bt_common_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the insert method for entries.
 */
typedef struct H5G_bt_ins_t {
    /* downward */
    H5G_bt_common_t common;          /* Common info for B-tree user data (must be first) */
    const H5O_link_t *lnk;              /* Link to insert into table         */
} H5G_bt_ins_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the remove method for entries.
 */
typedef struct H5G_bt_rm_t {
    /* downward */
    H5G_bt_common_t common;          /* Common info for B-tree user data (must be first) */
    H5RS_str_t *grp_full_path_r;        /* Full path of group where link is removed */
} H5G_bt_rm_t;

/* Typedef for B-tree 'find' operation */
typedef herr_t (*H5G_bt_find_op_t)(const H5G_entry_t *ent/*in*/, void *operator_data/*in,out*/);

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the 'find' method for entries.
 */
typedef struct H5G_bt_lkp_t {
    /* downward */
    H5G_bt_common_t common;          /* Common info for B-tree user data (must be first) */
    H5G_bt_find_op_t op;                /* Operator to call when correct entry is found */
    void *op_data;                      /* Data to pass to operator */

    /* upward */
} H5G_bt_lkp_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_it_t {
    /* downward */
    H5HL_t      *heap;          /*symbol table heap 			     */
    hsize_t	skip;		/*initial entries to skip		     */
    H5G_lib_iterate_t op;	/*iteration operator			     */
    void	*op_data;	/*user-defined operator data		     */

    /* upward */
    hsize_t	*final_ent;	/*final entry looked at                      */
} H5G_bt_it_it_t;

/* Data passed through B-tree iteration for copying copy symbol table content */
typedef struct H5G_bt_it_cpy_t {
    const H5O_loc_t *src_oloc;  /* Source object location */
    haddr_t     src_heap_addr;  /* Heap address of the source symbol table  */
    H5F_t       *dst_file;      /* File of destination group */
    H5O_stab_t  *dst_stab;      /* Symbol table message for destination group */
    H5O_copy_t  *cpy_info;      /* Information for copy operation */
} H5G_bt_it_cpy_t;

/* Common information for "by index" lookups in symbol tables */
typedef struct H5G_bt_it_idx_common_t {
    /* downward */
    H5F_t       *f;             /* Pointer to file that symbol table is in */
    hsize_t     idx;            /* Index of group member to be queried */
    hsize_t     num_objs;       /* The number of objects having been traversed */
    H5G_bt_find_op_t op;        /* Operator to call when correct entry is found */
} H5G_bt_it_idx_common_t;

/* Data passed through B-tree iteration for building a table of the links */
typedef struct H5G_bt_it_bt_t {
    /* downward */
    size_t alloc_nlinks;        /* Number of links allocated in table */
    H5HL_t      *heap;          /*symbol table heap 			     */

    /* upward */
    H5G_link_table_t *ltable;   /* Link table to add information to */
} H5G_bt_it_bt_t;

/* Typedefs for "new format" groups */
/* (fractal heap & v2 B-tree info) */

/* Typedef for native 'name' field index records in the v2 B-tree */
/* (Keep 'id' field first so generic record handling in callbacks works) */
typedef struct H5G_dense_bt2_name_rec_t {
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID for link */
    uint32_t hash;                      /* Hash of 'name' field value */
} H5G_dense_bt2_name_rec_t;

/* Typedef for native 'creation order' field index records in the v2 B-tree */
/* (Keep 'id' field first so generic record handling in callbacks works) */
typedef struct H5G_dense_bt2_corder_rec_t {
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID for link */
    int64_t corder;                     /* 'creation order' field value */
} H5G_dense_bt2_corder_rec_t;

/*
 * Common data exchange structure for dense link storage.  This structure is
 * passed through the v2 B-tree layer to the methods for the objects
 * to which the v2 B-tree points.
 */
typedef struct H5G_bt2_ud_common_t {
    /* downward */
    H5F_t       *f;                     /* Pointer to file that fractal heap is in */
    hid_t       dxpl_id;                /* DXPL for operation                */
    H5HF_t      *fheap;                 /* Fractal heap handle               */
    const char  *name;                  /* Name of link to compare           */
    uint32_t    name_hash;              /* Hash of name of link to compare   */
    int64_t     corder;                 /* Creation order value of link to compare   */
    H5B2_found_t found_op;              /* Callback when correct link is found */
    void        *found_op_data;         /* Callback data when correct link is found */
} H5G_bt2_ud_common_t;

/*
 * Data exchange structure for dense link storage.  This structure is
 * passed through the v2 B-tree layer when inserting links.
 */
typedef struct H5G_bt2_ud_ins_t {
    /* downward */
    H5G_bt2_ud_common_t common;         /* Common info for B-tree user data (must be first) */
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID of link to insert         */
} H5G_bt2_ud_ins_t;

/* Typedef for path traversal operations */
/* grp_loc is the location of the group in which the targeted object is located.
 * name is the last component of the object's name
 * lnk is the link between the group and the object
 * obj_loc is the target of the traversal (or NULL if the object doesn't exist)
 * operator_data is whatever udata was supplied when H5G_traverse was called
 * own_loc should be set to H5G_OWN_OBJ_LOC if this callback takes ownership of obj_loc,
 * H5G_OWN_GRP_LOC if it takes ownership of grp_loc, and H5G_OWN_NONE if obj_loc and
 * grp_loc need to be deleted.
 */
typedef herr_t (*H5G_traverse_t)(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk/*in*/, H5G_loc_t *obj_loc/*out*/, void *operator_data/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);

/* Typedef for group creation operation */
typedef struct {
    hid_t gcpl_id;              /* Group creation property list */
} H5G_obj_create_t;


/*****************************/
/* Package Private Variables */
/*****************************/

/*
 * This is the class identifier to give to the B-tree functions.
 */
H5_DLLVAR H5B_class_t H5B_SNODE[1];

/* The cache subclass */
H5_DLLVAR const H5AC_class_t H5AC_SNODE[1];

/* The v2 B-tree class for indexing 'name' field on links */
H5_DLLVAR const H5B2_class_t H5G_BT2_NAME[1];

/* The v2 B-tree class for indexing 'creation order' field on links */
H5_DLLVAR const H5B2_class_t H5G_BT2_CORDER[1];

/******************************/
/* Package Private Prototypes */
/******************************/

/*
 * General group routines
 */
H5_DLL H5G_t *H5G_create(H5F_t *file, hid_t gcpl_id, hid_t dxpl_id);
H5_DLL H5G_t *H5G_create_named(const H5G_loc_t *loc, const char *name,
    hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id);
H5_DLL H5G_t *H5G_open_name(const H5G_loc_t *loc, const char *name,
    hid_t gapl_id, hid_t dxpl_id);
H5_DLL herr_t H5G_iterate(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    const H5G_link_iterate_t *lnk_op, void *op_data, hid_t lapl_id, hid_t dxpl_id);

/*
 * Group hierarchy traversal routines
 */
H5_DLL herr_t H5G_traverse_term_interface(void);
H5_DLL herr_t H5G_traverse_special(const H5G_loc_t *grp_loc,
    const H5O_link_t *lnk, unsigned target, size_t *nlinks, hbool_t last_comp,
    H5G_loc_t *obj_loc, hid_t lapl_id, hid_t dxpl_id);
H5_DLL herr_t H5G_traverse(const H5G_loc_t *loc, const char *name,
    unsigned target, H5G_traverse_t op, void *op_data, hid_t lapl_id,
    hid_t dxpl_id);

/*
 * Utility functions
 */
H5_DLL herr_t H5G_init(void);
H5_DLL char *H5G_normalize(const char *name);
H5_DLL const char *H5G_component(const char *name, size_t *size_p);

/*
 * Functions that understand symbol tables but not names.  The
 * functions that understand names are exported to the rest of
 * the library and appear in H5Gprivate.h.
 */
H5_DLL herr_t H5G_stab_create(H5O_loc_t *grp_oloc, hid_t dxpl_id,
    const H5O_ginfo_t *ginfo, H5O_stab_t *stab);
H5_DLL herr_t H5G_stab_create_components(H5F_t *f, H5O_stab_t *stab, size_t size_hint, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_insert(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *obj_lnk, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_insert_real(H5F_t *f, H5O_stab_t *stab, const char *name,
    H5O_link_t *obj_lnk, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_delete(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab);
H5_DLL herr_t H5G_stab_iterate(const H5O_loc_t *oloc, hid_t dxpl_id, H5_iter_order_t order,
    hsize_t skip, hsize_t *last_lnk, H5G_lib_iterate_t op, void *op_data);
H5_DLL herr_t H5G_stab_count(struct H5O_loc_t *oloc, hsize_t *num_objs, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_bh_size(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab,
    H5_ih_info_t *bh_info);
H5_DLL ssize_t H5G_stab_get_name_by_idx(H5O_loc_t *oloc, H5_iter_order_t order,
    hsize_t n, char* name, size_t size, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_remove(H5O_loc_t *oloc, hid_t dxpl_id,
    H5RS_str_t *grp_full_path_r, const char *name);
H5_DLL herr_t H5G_stab_remove_by_idx(H5O_loc_t *oloc, hid_t dxpl_id,
    H5RS_str_t *grp_full_path_r, H5_iter_order_t order, hsize_t n);
H5_DLL herr_t H5G_stab_lookup(H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk, hid_t dxpl_id);
H5_DLL herr_t H5G_stab_lookup_by_idx(H5O_loc_t *grp_oloc, H5_iter_order_t order,
    hsize_t n, H5O_link_t *lnk, hid_t dxpl_id);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G_stab_get_type_by_idx(H5O_loc_t *oloc, hsize_t idx,
    hid_t dxpl_id);
#endif /* H5_NO_DEPRECATED_SYMBOLS */


/*
 * Functions that understand symbol table entries.
 */
H5_DLL herr_t H5G_ent_copy(H5G_entry_t *dst, const H5G_entry_t *src,
            H5_copy_depth_t depth);
H5_DLL herr_t H5G_ent_reset(H5G_entry_t *ent);
H5_DLL herr_t H5G_ent_decode_vec(H5F_t *f, const uint8_t **pp,
				  H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G_ent_encode_vec(H5F_t *f, uint8_t **pp,
				  const H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G_ent_convert(H5F_t *f, hid_t dxpl_id, H5HL_t *heap,
    const char *name, const H5O_link_t *lnk, H5G_entry_t *ent);
H5_DLL herr_t H5G_ent_debug(H5F_t *f, const H5G_entry_t *ent,
    FILE * stream, int indent, int fwidth, H5HL_t *heap);

/* Functions that understand symbol table nodes */
H5_DLL herr_t H5G_node_init(H5F_t *f);
H5_DLL size_t H5G_node_size_real(const H5F_t *f);
H5_DLL int H5G_node_iterate(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G_node_sumup(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G_node_by_idx(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G_node_copy(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G_node_build_table(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL herr_t H5G_node_iterate_size(H5F_t *f, hid_t dxpl_id, const void *_lt_key, haddr_t addr,
                     const void *_rt_key, void *_udata);

/* Functions that understand links in groups */
H5_DLL int H5G_link_cmp_name_inc(const void *lnk1, const void *lnk2);
H5_DLL int H5G_link_cmp_name_dec(const void *lnk1, const void *lnk2);
H5_DLL int H5G_link_cmp_corder_inc(const void *lnk1, const void *lnk2);
H5_DLL int H5G_link_cmp_corder_dec(const void *lnk1, const void *lnk2);
H5_DLL herr_t H5G_ent_to_link(H5F_t *f, H5O_link_t *lnk, const H5HL_t *heap,
    const H5G_entry_t *ent, const char *name);
H5_DLL herr_t H5G_ent_to_info(H5F_t *f, H5L_info_t *info, const H5HL_t *heap,
    const H5G_entry_t *ent);
H5_DLL herr_t H5G_link_to_info(const H5O_link_t *lnk, H5L_info_t *linfo);
H5_DLL herr_t H5G_link_to_loc(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc);
H5_DLL herr_t H5G_link_copy_file(H5F_t *dst_file, hid_t dxpl_id,
    const H5O_link_t *_src_lnk, const H5O_loc_t *src_oloc, H5O_link_t *dst_lnk,
    H5O_copy_t *cpy_info);
H5_DLL herr_t H5G_link_sort_table(H5G_link_table_t *ltable, H5_index_t idx_type,
    H5_iter_order_t order);
H5_DLL herr_t H5G_link_iterate_table(const H5G_link_table_t *ltable,
    hsize_t skip, hsize_t *last_lnk, const H5G_lib_iterate_t op, void *op_data);
H5_DLL herr_t H5G_link_release_table(H5G_link_table_t *ltable);
H5_DLL herr_t H5G_link_name_replace(H5F_t *file, hid_t dxpl_id,
    H5RS_str_t *grp_full_path_r, const H5O_link_t *lnk);

/* Functions that understand "compact" link storage */
H5_DLL herr_t H5G_compact_insert(const H5O_loc_t *grp_oloc, H5O_link_t *obj_lnk,
    hid_t dxpl_id);
H5_DLL ssize_t H5G_compact_get_name_by_idx(H5O_loc_t *oloc, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t idx, char *name, size_t size);
H5_DLL herr_t H5G_compact_remove(const H5O_loc_t *oloc, hid_t dxpl_id,
    H5RS_str_t *grp_full_path_r, const char *name);
H5_DLL herr_t H5G_compact_remove_by_idx(const H5O_loc_t *oloc, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5RS_str_t *grp_full_path_r, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n);
H5_DLL herr_t H5G_compact_iterate(const H5O_loc_t *oloc, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t skip, hsize_t *last_lnk, H5G_lib_iterate_t op, void *op_data);
H5_DLL htri_t H5G_compact_lookup(H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk, hid_t dxpl_id);
H5_DLL herr_t H5G_compact_lookup_by_idx(H5O_loc_t *oloc, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t n, H5O_link_t *lnk);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G_compact_get_type_by_idx(H5O_loc_t *oloc, hid_t dxpl_id,
    const H5O_linfo_t *linfo, hsize_t idx);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Functions that understand "dense" link storage */
H5_DLL herr_t H5G_dense_build_table(H5F_t *f, hid_t dxpl_id, const H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, H5G_link_table_t *ltable);
H5_DLL herr_t H5G_dense_create(H5F_t *f, hid_t dxpl_id, H5O_linfo_t *linfo);
H5_DLL herr_t H5G_dense_insert(H5F_t *f, hid_t dxpl_id,
    const H5O_linfo_t *linfo, const H5O_link_t *lnk);
H5_DLL htri_t H5G_dense_lookup(H5F_t *f, hid_t dxpl_id,
    const H5O_linfo_t *linfo, const char *name, H5O_link_t *lnk);
H5_DLL herr_t H5G_dense_lookup_by_idx(H5F_t *f, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t n, H5O_link_t *lnk);
H5_DLL herr_t H5G_dense_iterate(H5F_t *f, hid_t dxpl_id, const H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    H5G_lib_iterate_t op, void *op_data);
H5_DLL ssize_t H5G_dense_get_name_by_idx(H5F_t  *f, hid_t dxpl_id,
    H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    char *name, size_t size);
H5_DLL herr_t H5G_dense_remove(H5F_t *f, hid_t dxpl_id, const H5O_linfo_t *linfo,
    H5RS_str_t *grp_full_path_r, const char *name);
H5_DLL herr_t H5G_dense_remove_by_idx(H5F_t *f, hid_t dxpl_id,
    const H5O_linfo_t *linfo, H5RS_str_t *grp_full_path_r, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n);
H5_DLL herr_t H5G_dense_delete(H5F_t *f, hid_t dxpl_id, H5O_linfo_t *linfo,
    hbool_t adj_link);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G_dense_get_type_by_idx(H5F_t  *f, hid_t dxpl_id,
    H5O_linfo_t *linfo, hsize_t idx);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Functions that understand group objects */
H5_DLL herr_t H5G_obj_create(H5F_t *f, hid_t dxpl_id, const H5O_ginfo_t *ginfo,
    const H5O_linfo_t *linfo, hid_t gcpl_id, H5O_loc_t *oloc/*out*/);
H5_DLL htri_t H5G_obj_get_linfo(const H5O_loc_t *grp_oloc, H5O_linfo_t *linfo,
    hid_t dxpl_id);
H5_DLL herr_t H5G_obj_insert(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *obj_lnk, hbool_t adj_link, hid_t dxpl_id);
H5_DLL herr_t H5G_obj_iterate(const H5O_loc_t *grp_oloc,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    H5G_lib_iterate_t op, void *op_data, hid_t dxpl_id);
H5_DLL herr_t H5G_obj_info(H5O_loc_t *oloc, H5G_info_t *grp_info, hid_t dxpl_id);
H5_DLL ssize_t H5G_obj_get_name_by_idx(H5O_loc_t *oloc, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, char* name, size_t size, hid_t dxpl_id);
H5_DLL herr_t H5G_obj_remove(H5O_loc_t *oloc, H5RS_str_t *grp_full_path_r,
    const char *name, hid_t dxpl_id);
H5_DLL herr_t H5G_obj_remove_by_idx(H5O_loc_t *grp_oloc, H5RS_str_t *grp_full_path_r,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t dxpl_id);
H5_DLL htri_t H5G_obj_lookup(H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk, hid_t dxpl_id);
H5_DLL herr_t H5G_obj_lookup_by_idx(H5O_loc_t *grp_oloc, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5O_link_t *lnk, hid_t dxpl_id);

/*
 * These functions operate on group hierarchy names.
 */
H5_DLL herr_t H5G_name_init(H5G_name_t *name, const char *path);
H5_DLL herr_t H5G_name_set(H5G_name_t *loc, H5G_name_t *obj, const char *name);
H5_DLL H5RS_str_t *H5G_build_fullpath_refstr_str(H5RS_str_t *path_r, const char *name);

/*
 * These functions operate on group "locations"
 */
H5_DLL herr_t H5G_loc_root(H5F_t *f, H5G_loc_t *loc);
H5_DLL herr_t H5G_loc_copy(H5G_loc_t *dst, const H5G_loc_t *src, H5_copy_depth_t depth);
H5_DLL herr_t H5G_loc_insert(H5G_loc_t *grp_loc, const char *name,
    H5G_loc_t *obj_loc, hid_t dxpl_id);

/* Testing functions */
#ifdef H5G_TESTING
H5_DLL htri_t H5G_is_empty_test(hid_t gid);
H5_DLL htri_t H5G_has_links_test(hid_t gid, unsigned *nmsgs);
H5_DLL htri_t H5G_has_stab_test(hid_t gid);
H5_DLL htri_t H5G_is_new_dense_test(hid_t gid);
H5_DLL herr_t H5G_new_dense_info_test(hid_t gid, hsize_t *name_count, hsize_t *corder_count);
H5_DLL herr_t H5G_lheap_size_test(hid_t gid, size_t *lheap_size);
H5_DLL herr_t H5G_user_path_test(hid_t obj_id, char *user_path, size_t *user_path_len, unsigned *user_path_hidden);
#endif /* H5G_TESTING */

#endif /* _H5Gpkg_H */

