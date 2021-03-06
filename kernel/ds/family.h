#ifndef _KERNEL_DS_FAMILY_H
#define _KERNEL_DS_FAMILY_H

#include <debug/assert.h>

#define NEXT_SIBLING( robj, type )							\
	(( robj->family_tree.sibling_next == NULL ) ? NULL :	\
			(type)robj->family_tree.sibling_next->object)	
		

#define PREV_SIBLING( robj, type )							\
	(( robj->family_tree.sibling_prev == NULL ) ? NULL :	\
			(type)robj->family_tree.sibling_prev->object)	

#define PARENT( robj, type )								\
	(( robj->family_tree.parent == NULL ) ? NULL :			\
			(type)robj->family_tree.parent->object)		


#define CHILD( robj, type )								\
	(( robj->family_tree.child == NULL ) ? NULL :		\
			(type)robj->family_tree.child->object)	


#define FAMILY_TREE		struct ds_family_tree family_tree

#define FAMILY_INIT( robj )		family_init( &(robj->family_tree), robj )	

struct ds_family_tree
{										
	void* object;
	struct ds_family_tree* parent;						
	struct ds_family_tree* child;							
	struct ds_family_tree* sibling_prev;					
	struct ds_family_tree* sibling_next;					
};							


static inline void family_init( struct ds_family_tree *fam, void *object )
{
	fam->object = object;
	fam->parent = NULL;
	fam->child = NULL;
	fam->sibling_prev = NULL;
	fam->sibling_next = NULL;
}


/**  
 *
 * \note Does not forget the children. Children are still attached.
 * 
 */
static inline void family_remove( struct ds_family_tree *fam )
{
	if ( fam->parent != NULL )
	{
	     if ( fam->parent->child == fam )
			  fam->parent->child = fam->sibling_next;
	}

    if ( fam->sibling_prev != NULL )
			 fam->sibling_prev->sibling_next = fam->sibling_next;
    if ( fam->sibling_next != NULL )
			 fam->sibling_next->sibling_prev = fam->sibling_prev;
	
	fam->parent = NULL;
	fam->sibling_next = NULL;
	fam->sibling_prev = NULL;
}


static inline void family_add_child( struct ds_family_tree *parent, 
								struct ds_family_tree* fam )
{
	if ( parent->child != NULL )  
	{
		parent->child->sibling_prev = fam;
		fam->sibling_next = parent->child;
	}

	parent->child = fam;
	fam->parent = parent;
}

static inline void family_add_sibling( struct ds_family_tree *bro, 
										struct ds_family_tree* fam )
{
	if ( bro->sibling_next != NULL ) bro->sibling_next->sibling_prev = fam;
	fam->sibling_next = bro->sibling_next;
	fam->sibling_prev = bro;
	bro->sibling_next = fam;

	fam->parent = bro->parent;
}


/** All the children from the deadParent will become the children of 
 * the stepParent.
 */
static inline void family_adopt( struct ds_family_tree *stepParent, 
								 struct ds_family_tree *deadParent )
{
	struct ds_family_tree *tmp;
	struct ds_family_tree *next;

	tmp = deadParent->child;
	while ( tmp != NULL )
	{
		next = tmp->sibling_next;
			
			tmp->parent = stepParent;
			tmp->sibling_prev = NULL;
			tmp->sibling_next = stepParent->child;

			if ( stepParent->child != NULL )
					stepParent->child->sibling_prev = tmp;

			stepParent->child = tmp;

		tmp = next;
	}

	deadParent->child = NULL;
}



#endif

