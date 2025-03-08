/**
 * External Test for XC Object Functionality
 * 
 * This test validates the object functionality through the public API.
 * It only uses functions and types defined in libxc.h.
 */
#include "test_utils.h"
// #include <stdio.h>
// #include <string.h>

static xc_runtime_t* rt = NULL;

/* Test basic object functionality */
void test_object_basic(void) {
    rt = xc_singleton();
    test_start("Object Basic Functionality (External)");
    
    printf("Testing basic object functionality through public API...\n");
    
    // Create an object
    xc_val obj = rt->new(XC_TYPE_OBJECT);
    TEST_ASSERT(obj != NULL, "Object creation failed");
    TEST_ASSERT(rt->is(obj, XC_TYPE_OBJECT), "Object type check failed");
    
    // Create a string value
    xc_val str_key = rt->new(XC_TYPE_STRING, "name");
    xc_val str_value = rt->new(XC_TYPE_STRING, "XC Object");
    
    // Set property using dot notation
    rt->dot(obj, "name", str_value);
    
    // Get property
    xc_val result = rt->dot(obj, "name");
    TEST_ASSERT(result != NULL, "Property retrieval failed");
    TEST_ASSERT(rt->is(result, XC_TYPE_STRING), "Property type check failed");
    
    // Create a number value
    xc_val num_value = rt->new(XC_TYPE_NUMBER, 42.0);
    
    // Set another property
    rt->dot(obj, "answer", num_value);
    
    // Get the number property
    result = rt->dot(obj, "answer");
    TEST_ASSERT(result != NULL, "Number property retrieval failed");
    TEST_ASSERT(rt->is(result, XC_TYPE_NUMBER), "Number property type check failed");
    
    // Test property existence
    TEST_ASSERT(rt->dot(obj, "name") != NULL, "Property 'name' should exist");
    TEST_ASSERT(rt->dot(obj, "nonexistent") == NULL, "Property 'nonexistent' should not exist");
    
    printf("Object test completed successfully.\n");
    
    test_end("Object Basic Functionality (External)");
}

/* Test object prototype inheritance */
void test_object_prototype(void) {
    rt = xc_singleton();
    test_start("Object Prototype Inheritance (External)");
    
    printf("Testing object prototype inheritance through public API...\n");
    
    // Create a prototype object
    xc_val proto = rt->new(XC_TYPE_OBJECT);
    TEST_ASSERT(proto != NULL, "Prototype object creation failed");
    
    // Set a property on the prototype
    xc_val proto_value = rt->new(XC_TYPE_STRING, "Prototype Value");
    rt->dot(proto, "protoProperty", proto_value);
    
    // Create an object with the prototype
    xc_val obj = rt->new(XC_TYPE_OBJECT);
    TEST_ASSERT(obj != NULL, "Object creation failed");
    
    // Set the prototype
    rt->call(obj, "setPrototype", proto);
    
    // Set a property on the object
    xc_val obj_value = rt->new(XC_TYPE_STRING, "Object Value");
    rt->dot(obj, "objProperty", obj_value);
    
    // Test direct property access
    xc_val result = rt->dot(obj, "objProperty");
    TEST_ASSERT(result != NULL, "Direct property retrieval failed");
    
    // Test prototype property access
    result = rt->dot(obj, "protoProperty");
    TEST_ASSERT(result != NULL, "Prototype property retrieval failed");
    
    // Test property shadowing
    xc_val shadow_value = rt->new(XC_TYPE_STRING, "Shadow Value");
    rt->dot(obj, "protoProperty", shadow_value);
    
    result = rt->dot(obj, "protoProperty");
    TEST_ASSERT(result != NULL, "Shadowed property retrieval failed");
    
    printf("Object prototype test completed successfully.\n");
    
    test_end("Object Prototype Inheritance (External)");
} 
