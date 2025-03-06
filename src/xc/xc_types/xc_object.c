/*
 * xc_object.c - Object type implementation
 */

#include "../xc.h"
#include "../xc_object.h"
#include "../xc_gc.h"
#include "xc_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Initial capacity for object properties */
#define INITIAL_CAPACITY 8

/* Object property entry */
typedef struct {
    xc_object_t *key;    /* String key */
    xc_object_t *value;  /* Any value */
} xc_property_t;

/* Object structure */
typedef struct {
    xc_object_t base;          /* Must be first */
    xc_property_t *properties; /* Array of properties */
    size_t count;             /* Number of properties */
    size_t capacity;          /* Allocated capacity */
    xc_object_t *prototype;   /* Prototype object for inheritance */
} xc_object_data_t;

/* Internal helper: Find property by key */
static xc_property_t *find_property(xc_object_data_t *obj, const char *key) {
    for (size_t i = 0; i < obj->count; i++) {
        const char *prop_key = xc_string_value(NULL, obj->properties[i].key);
        if (strcmp(prop_key, key) == 0) {
            return &obj->properties[i];
        }
    }
    return NULL;
}

/* Internal helper: Ensure capacity */
static bool ensure_capacity(xc_runtime_t *rt, xc_object_data_t *obj, size_t needed) {
    if (needed <= obj->capacity) {
        return true;
    }

    size_t new_capacity = obj->capacity == 0 ? INITIAL_CAPACITY : obj->capacity * 2;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    xc_property_t *new_props = realloc(obj->properties, new_capacity * sizeof(xc_property_t));
    if (!new_props) {
        return false;
    }

    obj->properties = new_props;
    obj->capacity = new_capacity;
    return true;
}

/* Object methods */
static void object_mark(xc_runtime_t *rt, xc_object_t *obj) {
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    /* Mark all properties */
    for (size_t i = 0; i < object->count; i++) {
        if (object->properties[i].key) {
            xc_gc_mark(rt, object->properties[i].key);
        }
        if (object->properties[i].value) {
            xc_gc_mark(rt, object->properties[i].value);
        }
    }
    
    /* Mark prototype */
    if (object->prototype) {
        xc_gc_mark(rt, object->prototype);
    }
}

static void object_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    /* Free all properties */
    for (size_t i = 0; i < object->count; i++) {
        if (object->properties[i].key) {
            xc_gc_free(rt, object->properties[i].key);
        }
        if (object->properties[i].value) {
            xc_gc_free(rt, object->properties[i].value);
        }
    }
    
    /* Free properties array */
    free(object->properties);
    
    /* Free prototype reference */
    if (object->prototype) {
        xc_gc_free(rt, object->prototype);
    }
}

static bool object_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_object(rt, b)) {
        return false;
    }
    
    xc_object_data_t *obj_a = (xc_object_data_t *)a;
    xc_object_data_t *obj_b = (xc_object_data_t *)b;
    
    if (obj_a->count != obj_b->count) {
        return false;
    }
    
    /* Compare all properties */
    for (size_t i = 0; i < obj_a->count; i++) {
        xc_property_t *prop_a = &obj_a->properties[i];
        const char *key = xc_string_value(rt, prop_a->key);
        xc_property_t *prop_b = find_property(obj_b, key);
        
        if (!prop_b || !xc_equal(rt, prop_a->value, prop_b->value)) {
            return false;
        }
    }
    
    return true;
}

static int object_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_object(rt, b)) {
        return 1;  /* Objects are greater than non-objects */
    }
    
    /* Compare by number of properties */
    xc_object_data_t *obj_a = (xc_object_data_t *)a;
    xc_object_data_t *obj_b = (xc_object_data_t *)b;
    
    if (obj_a->count < obj_b->count) return -1;
    if (obj_a->count > obj_b->count) return 1;
    
    return 0;  /* Equal number of properties */
}

/* Type descriptor for object type */
static xc_type_t object_type = {
    .name = "object",
    .flags = XC_TYPE_COMPOSITE,
    .mark = object_mark,
    .free = object_free,
    .equal = object_equal,
    .compare = object_compare
};

/* Register object type */
void xc_register_object_type(xc_runtime_t *rt) {
    XC_RUNTIME_EXT(rt)->object_type = &object_type;
}

/* Create object */
xc_object_t *xc_object_create(xc_runtime_t *rt) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_object_data_t *obj = (xc_object_data_t *)xc_gc_alloc(rt, sizeof(xc_object_data_t), XC_TYPE_OBJECT);
    if (!obj) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)obj)->type = XC_RUNTIME_EXT(rt)->object_type;

    obj->properties = NULL;
    obj->count = 0;
    obj->capacity = 0;
    obj->prototype = NULL;

    return (xc_object_t *)obj;
}

/* Object operations */
xc_object_t *xc_object_get(xc_runtime_t *rt, xc_object_t *obj, const char *key) {
    assert(xc_is_object(rt, obj));
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    /* Look for property in this object */
    xc_property_t *prop = find_property(object, key);
    if (prop) {
        return prop->value;
    }
    
    /* Look in prototype chain */
    if (object->prototype) {
        return xc_object_get(rt, object->prototype, key);
    }
    
    return NULL;
}

void xc_object_set(xc_runtime_t *rt, xc_object_t *obj, const char *key, xc_object_t *value) {
    assert(xc_is_object(rt, obj));
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    /* Look for existing property */
    xc_property_t *prop = find_property(object, key);
    if (prop) {
        /* Update existing property */
        if (prop->value) {
            xc_gc_free(rt, prop->value);
        }
        prop->value = value;
        if (value) {
            xc_gc_add_ref(rt, value);
        }
        return;
    }
    
    /* Create new property */
    if (!ensure_capacity(rt, object, object->count + 1)) {
        return;
    }
    
    /* Create key string */
    xc_object_t *key_str = xc_string_create(rt, key);
    if (!key_str) {
        return;
    }
    
    /* Add new property */
    prop = &object->properties[object->count++];
    prop->key = key_str;
    prop->value = value;
    if (value) {
        xc_gc_add_ref(rt, value);
    }
}

bool xc_object_has(xc_runtime_t *rt, xc_object_t *obj, const char *key) {
    assert(xc_is_object(rt, obj));
    return find_property((xc_object_data_t *)obj, key) != NULL;
}

void xc_object_delete(xc_runtime_t *rt, xc_object_t *obj, const char *key) {
    assert(xc_is_object(rt, obj));
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    for (size_t i = 0; i < object->count; i++) {
        const char *prop_key = xc_string_value(rt, object->properties[i].key);
        if (strcmp(prop_key, key) == 0) {
            /* Release property references */
            if (object->properties[i].key) {
                xc_gc_release(rt, object->properties[i].key);
            }
            if (object->properties[i].value) {
                xc_gc_release(rt, object->properties[i].value);
            }
            
            /* Move remaining properties */
            memmove(&object->properties[i], &object->properties[i + 1],
                   (object->count - i - 1) * sizeof(xc_property_t));
            object->count--;

            /* Clear the last slot */
            if (object->count < object->capacity) {
                object->properties[object->count].key = NULL;
                object->properties[object->count].value = NULL;
            }
            return;
        }
    }
}

/* Type checking */
bool xc_is_object(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->object_type;
}

/* Prototype operations */
void xc_object_set_prototype(xc_runtime_t *rt, xc_object_t *obj, xc_object_t *proto) {
    assert(xc_is_object(rt, obj));
    xc_object_data_t *object = (xc_object_data_t *)obj;
    
    if (object->prototype) {
        xc_gc_free(rt, object->prototype);
    }
    
    object->prototype = proto;
    if (proto) {
        xc_gc_add_ref(rt, proto);
    }
}

xc_object_t *xc_object_get_prototype(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_object(rt, obj));
    xc_object_data_t *object = (xc_object_data_t *)obj;
    return object->prototype;
}