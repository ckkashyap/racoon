#ifndef _KERNEL_DS_LIST_H
#define _KERNEL_DS_LIST_H


#define NEXT_LINEAR( OBA, type )					\
	(( OBA->list.next == NULL ) ? NULL :	(OBA->list.next->object) )
		

#define PREV_LINEAR( OBA, type )					\
	(( OBA->list.prev == NULL ) ? NULL : (OBA->list.prev->object))


#define LINKED_LIST			struct ds_list list

#define LIST_INIT( robj )	list_init( &(robj->list), robj )	

struct ds_list	
{										
	void* object;
	struct ds_list* prev;					
	struct ds_list* next;					
};							


static inline void list_init( struct ds_list *l, void *object )
{
	l->object = object;
	l->prev = NULL;
	l->next = NULL;
}


static inline void list_remove( struct ds_list *l )
{
	if ( l->prev != NULL )
			 l->prev->next = l->next;
	if ( l->next != NULL )
			  l->next->prev = l->prev;

	list_init( l, l->object );
}


static inline void list_insertBefore( struct ds_list *position, struct ds_list* l )
{
	l->prev = position->prev;
	l->next = position;
	position->prev = l;

	if ( l->prev != NULL ) l->prev->next = l;
}

static inline void list_insertAfter( struct ds_list *position, struct ds_list* l )
{
	l->prev = position;
	l->next = position->next;
	position->next = l;

	if ( l->next != NULL ) l->next->prev = l;
}



#endif

