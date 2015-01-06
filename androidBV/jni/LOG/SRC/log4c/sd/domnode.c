static const char version[] = "$Id$";

/* 
 * Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifdef HAVE_CONFIG_H
#include "log4c_config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "malloc.h"
#include "error.h"
#include "domnode.h"
#include "list.h"

/* TODO: generic format support */
#include "domnode-xml.h"

/******************************************************************************/
extern sd_domnode_t* __sd_domnode_new(const char* a_name, const char* a_value,
				      int is_elem)
{
    sd_domnode_t* this;
    
    this = sd_calloc(1, sizeof(*this));
    
    this->name     = a_name    ? sd_strdup(a_name) : 0;
    this->value    = a_value   ? sd_strdup(a_value): 0;
    this->children = is_elem ? sd_list_new(10) : 0;
    this->attrs    = is_elem ? sd_list_new(10) : 0;
    
    return this;
}

/******************************************************************************/
extern sd_domnode_t* sd_domnode_new(const char* a_name, const char* a_value)
{
    return __sd_domnode_new(a_name, a_value, 1);
}

/******************************************************************************/
static int foreach_delete(sd_domnode_t* a_node, void* unused)
{
    sd_domnode_delete(a_node);
    return 0;
}

/******************************************************************************/
static void domnode_clear(sd_domnode_t* this)
{
    free((void*) this->name);
    free((void*) this->value);
    
    sd_list_foreach(this->children, (sd_list_func_t) foreach_delete, 0);
    sd_list_delete(this->children);
    
    sd_list_foreach(this->attrs, (sd_list_func_t) foreach_delete, 0);
    sd_list_delete(this->attrs);
}

/******************************************************************************/
extern void sd_domnode_delete(sd_domnode_t* this)
{
    if (!this)
	return;
    
    domnode_clear(this);
    free(this);
}

/******************************************************************************/
static void domnode_update(sd_domnode_t* this, sd_domnode_t* a_node)
{
    domnode_clear(this);
    this->name	= a_node->name;
    this->value	= a_node->value;
    this->children = a_node->children;
    this->attrs	= a_node->attrs;
    
    /* Destroy this now empty node ! */
    free(a_node);
}

/******************************************************************************/
extern int sd_domnode_fread(sd_domnode_t* this, FILE* a_stream)
{
    int ret;
    sd_domnode_t* node;
    
    /* TODO: generic format support */
    if (! (ret = __sd_domnode_xml_fread(&node, a_stream)))
	domnode_update(this, node);
    
    return ret ? -1 : 0;
}

/******************************************************************************/
extern int sd_domnode_read(sd_domnode_t* this, const char* a_buffer,
			   size_t a_size)
{
    int ret;
    sd_domnode_t* node;
    
    /* TODO: generic format support */
    if (! (ret = __sd_domnode_xml_read(&node, a_buffer, a_size)))
	domnode_update(this, node);
    
    return ret ? -1 : 0;
}

/******************************************************************************/
extern int sd_domnode_write(sd_domnode_t* this, char** a_buffer,
			    size_t* a_size)
{
    /* TODO: generic format support */
    return __sd_domnode_xml_write(this, a_buffer, a_size);
}

/******************************************************************************/
extern int sd_domnode_fwrite(const sd_domnode_t* this, FILE* a_stream)
{
    /* TODO: generic format support */
    return __sd_domnode_xml_fwrite(this, a_stream);
}

/******************************************************************************/
extern int sd_domnode_load(sd_domnode_t* this, const char* a_filename)
{
    FILE* fp;
    int   ret = 0;

    if ( (fp = fopen(a_filename, "r")) == 0)
	return -1;
    
    ret = sd_domnode_fread(this, fp);

    fclose(fp);
    return ret;
}

/******************************************************************************/
extern int sd_domnode_store(const sd_domnode_t* this, const char* afilename)
{
    FILE* fp;
    int   ret = 0;

    if ( (fp = fopen(afilename, "w")) == 0)
	return -1;
    
    ret = sd_domnode_fwrite(this, fp);

    fclose(fp);
    return ret;
}

/******************************************************************************/
extern sd_domnode_t* sd_domnode_search(const sd_domnode_t* this,
				       const char* a_name)
{
    sd_list_iter_t* i;

    if(!this)
	{
		return 0;
	}

    if(this->name && strcmp(this->name, a_name) == 0)
    {
    	return this;
    }

    for (i = sd_list_begin(this->children); i != sd_list_end(this->children); 
	 i = sd_list_iter_next(i)) {
	sd_domnode_t* node = i->data;
	
	if (strcmp(node->name, a_name) == 0)
	    return node;
    }

    for (i = sd_list_begin(this->attrs); i != sd_list_end(this->attrs); 
	 i = sd_list_iter_next(i)) {
	sd_domnode_t* node = i->data;
	
	if (strcmp(node->name, a_name) == 0)
	    return node;
    }

    for (i = sd_list_begin(this->children); i != sd_list_end(this->children); 
	 i = sd_list_iter_next(i)) {
	sd_domnode_t* node = i->data;
	
	if ((node = sd_domnode_search(node, a_name)) != 0)
	    return node;
    }

    return 0;
}

/******************************************************************************/
extern sd_domnode_t* sd_domnode_attrs_put(sd_domnode_t* this,
					  sd_domnode_t* a_attr)
{
    sd_list_iter_t* i;
    
    if (!this || !this->attrs || !a_attr || !a_attr->value)
	return 0;
    
    if ((i = sd_list_lookadd(this->attrs, a_attr)) == sd_list_end(this->attrs))
	return 0;
    
    return i->data;
}

/******************************************************************************/
extern sd_domnode_t* 
sd_domnode_attrs_get(const sd_domnode_t* this, const char* a_name)
{
    sd_list_iter_t* i;

    if (!this || !this->attrs || !a_name || !*a_name)
	return 0;

    for (i = sd_list_begin(this->attrs); i != sd_list_end(this->attrs); 
	 i = sd_list_iter_next(i)) {
	sd_domnode_t* node = i->data;
	
	if (strcmp(node->name, a_name) == 0)
	    return node;
    }

    return 0;
}

/******************************************************************************/
extern sd_domnode_t* 
sd_domnode_attrs_remove(sd_domnode_t* this, const char* a_name)
{
    sd_list_iter_t* i;

    if (!this || !this->attrs || !a_name || !*a_name)
	return 0;

    for (i = sd_list_begin(this->attrs); i != sd_list_end(this->attrs);
	 i = sd_list_iter_next(i)) {
	sd_domnode_t* node = i->data;
	
	if (strcmp(node->name, a_name) == 0) {
	    sd_list_iter_del(i);
	    return node;
	}
    }

    return 0;
}

extern sd_domnode_t* sd_domnode_first_child(const sd_domnode_t* this)
{
    if(this && this->children != 0 ){
    	if(sd_list_begin(this->children) == 0)
    	{
    		return 0;
    	}
       return sd_list_begin(this->children)->data;
    }
    return 0;
}

extern sd_domnode_t* sd_domnode_next_sibling(const sd_domnode_t* child)
{
    sd_list_iter_t* i;
    if(child){
        for (i = sd_list_begin(child); i != sd_list_end(child);
             i = sd_list_iter_next(i)) {
                sd_domnode_t* node = i->data;
                if(node == child){
                    return sd_list_iter_next(i)->data;
                }
        }
    }
    return 0;
}

extern sd_domnode_t* sd_domnode_last_child(const sd_domnode_t* this)
{
	if(!this)
	{
		return 0;
	}
    if(this->children != 0 ){
    	if(sd_list_rbegin(this->children) == 0)
    	{
    		return 0;
    	}
       return sd_list_rbegin(this->children)->__prev->data;
    }
    return 0;
}

extern sd_domnode_t* sd_domnode_next_child(const sd_domnode_t* parent, const sd_domnode_t* child)
{
    sd_list_iter_t* i;
    if(parent){
        for (i = sd_list_begin(parent->children); i != sd_list_end(parent->children);
             i = sd_list_iter_next(i)) {
                sd_domnode_t* node = i->data;
                if(0 == child){
                    return node;
                }
                if(node == child && sd_list_iter_next(i) != 0){
                    return sd_list_iter_next(i)->data;
                }
        }
    }
    return 0;
}

extern int sd_domnode_add_child_to_child(const sd_domnode_t* holder, sd_domnode_t* src)
{
	sd_domnode_t *new_child;
	sd_domnode_t *new_attr;
	sd_domnode_t *new_data;
	sd_list_iter_t* i;

	new_child = sd_domnode_new(src->name,NULL);
	for (i = sd_list_begin(src->attrs); i != sd_list_end(src->attrs);
			 i = sd_list_iter_next(i))
	{
		new_data = i->data;
		new_attr = __sd_domnode_new(new_data->name,new_data->value,0);
		sd_list_add_last(new_child->attrs,new_attr);
	}
	sd_list_add(holder->children,new_child);
}

extern sd_domnode_t* sd_domnode_search_attrs_value(const sd_domnode_t* this,
				       const char* a_name,const char* a_value)
{
    sd_list_iter_t* i;


}

extern int sd_domnode_del_child_from_child(const sd_domnode_t* holder, sd_domnode_t* src,const char* a_name)
{
	sd_domnode_t *del_child;
	sd_domnode_t *del_data;
	sd_domnode_t *child;
	sd_domnode_t *next_child;
	sd_domnode_t *data;
	sd_list_iter_t* i;
	sd_list_iter_t* j;

	i = sd_list_begin(src->attrs);
	del_data = i->data;
	child = sd_domnode_first_child(holder);
	while(child)
	{
		next_child = sd_domnode_next_child(holder, child);
		j = sd_list_begin(child->attrs);
		data = j->data;
		if(strcmp(data->value,del_data->value) == 0)
		{
			domnode_clear(child);
			sd_list_del(holder->children,child);
		}
		child = next_child;
	}

	return 0;
}
