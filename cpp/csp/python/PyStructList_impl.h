#ifndef _IN_CSP_PYTHON_PYSTRUCTLIST_HI
#define _IN_CSP_PYTHON_PYSTRUCTLIST_HI

#include <csp/engine/PartialSwitchCspType.h>
#include <csp/python/Conversions.h>
#include <csp/python/InitHelper.h>
#include <csp/python/PyCspType.h>
#include <csp/python/PyStructList.h>
#include <algorithm>

namespace csp::python
{

template<typename StorageT> inline StorageT convertValue( PyStructList<StorageT> * self, PyObject * value )
{
    return static_cast<StorageT>( fromPython<typename PyStructList<StorageT>::ElemT>( value, self -> field_type ) );
}

template<typename StorageT>
static PyObject *
PyStructList_Append( PyStructList<StorageT> * self, PyObject * args )
{
    CSP_BEGIN_METHOD;

    PyObject * value;
    if( !PyArg_ParseTuple( args, "O", &value ) )
        return NULL;
    
    if( PyList_Append( ( PyObject * ) self, value ) < 0 )
        return NULL;

    // Append the value to the vector stored in the struct field
    self -> vector.emplace_back( convertValue( self, value ) );

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Insert( PyStructList<StorageT> * self, PyObject * args )
{
    CSP_BEGIN_METHOD;

    Py_ssize_t index;
    PyObject * value;
    if( !PyArg_ParseTuple( args, "nO", &index, &value ) )
        return NULL;

    if( PyList_Insert( ( PyObject * ) self, index, value ) < 0 )
        return NULL;

    // Insert the value in the vector stored in the struct field
    // Deal with Python list indices that can be negative or point outside of the array
    // Python allows indices for insert to be past the array boundaries, in which case the element is added to the beginning or end of the array
    int sz = self -> vector.size();
    if( index < 0 )
        index += sz;
    // Insert the value in the vector stored in the struct field
    auto it = self -> vector.begin() + std::max( 0, std::min( sz, ( int ) index ) );
    self -> vector.insert( it, convertValue( self, value ) );

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Pop( PyStructList<StorageT> * self, PyObject * args )
{
    CSP_BEGIN_METHOD;

    Py_ssize_t index = -1;
    if( !PyArg_ParseTuple( args, "|n", &index) )
        return NULL;

    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "pop" ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_CallFunction( func.ptr(), "On", self, index ) );   
    if( !result.ptr() )
        return NULL;

    // Deal with Python list indices that can be negative
    int sz = self -> vector.size();
    if( index < 0 )
        index += sz;
    // Pop the value from the vector stored in the struct field
    // Python doesn't allow indices for pop to be past the array boundaries, so it is safe
    assert( index >= 0 && index < sz );
    auto it = self -> vector.begin() + ( int ) index;
    self -> vector.erase( it );

    return result.release();

    CSP_RETURN_NULL;
}

template<typename StorageT>
static PyObject *
PyStructList_Reverse( PyStructList<StorageT> * self, PyObject * Py_UNUSED( ignored ) )
{
    CSP_BEGIN_METHOD;

    if( PyList_Reverse( ( PyObject * ) self ) < 0 )
        return NULL;

    // Reverse the vector stored in the struct field
    std::reverse( self -> vector.begin(), self -> vector.end() );

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Sort( PyStructList<StorageT> * self, PyObject * args, PyObject * kwargs )
{
    CSP_BEGIN_METHOD;

    if( PyObject_Length( args ) > 0 )
    {
        PyErr_SetString( PyExc_TypeError, "sort() takes no positional arguments" );
        return NULL;
    }

    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "sort" ) );
    PyObjectPtr arguments = PyObjectPtr::own( PyTuple_Pack( 1, ( PyObject * ) self ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_Call( func.ptr(), arguments.ptr(), kwargs ) );   
    if( !result.ptr() )
        return NULL;

    // Copy sorted list into the vector stored in the struct field
    size_t sz = self -> vector.size();

    for( size_t index = 0; index < sz; ++index )
    {
        PyObject * value = PyList_GET_ITEM( ( PyObject * ) self, index );
        self -> vector[ index ] = convertValue( self, value );
    }

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Extend( PyStructList<StorageT> * self, PyObject * args )
{
    CSP_BEGIN_METHOD;

    PyObject * iterable;
    if( !PyArg_ParseTuple( args, "O", &iterable ) )
        return NULL;

    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "extend" ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_CallFunctionObjArgs( func.ptr(), self, iterable, NULL ) );
    if( !result.ptr() )
        return NULL;

    // Copy new elements of the extended list into the vector stored in the struct field
    size_t new_sz = ( size_t ) PyObject_Length( ( PyObject * ) self );
    size_t old_sz = self -> vector.size();

    // Check that all elements of a modified vector are of correct type
    std::vector<StorageT> v = self -> vector;
    v.resize( new_sz );
    for( size_t index = old_sz; index < new_sz; ++index )
    {
        PyObject * value = PyList_GET_ITEM( ( PyObject * ) self, index );
        v[ index ] = convertValue( self, value );
    }

    self -> vector.swap( v );

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Remove( PyStructList<StorageT> * self, PyObject * args )
{
    CSP_BEGIN_METHOD;
    
    PyObject * value;
    if( !PyArg_ParseTuple( args, "O", &value) )
        return NULL;

    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "remove" ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_CallFunctionObjArgs( func.ptr(), self, value, NULL ) );   
    if( !result.ptr() )
        return NULL;

    // Remove the value from the vector stored in the struct field
    // The find always succeeds, as otherwise the above Python method call would have failed
    auto it = std::find( self -> vector.begin(), self -> vector.end(), convertValue( self, value ) );
    assert( it != self -> vector.end() );
    self -> vector.erase( it );

    CSP_RETURN_NONE;
}

template<typename StorageT>
static PyObject *
PyStructList_Clear( PyStructList<StorageT> * self, PyObject * Py_UNUSED( ignored ) )
{
    CSP_BEGIN_METHOD;
    
    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "clear" ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_CallFunctionObjArgs( func.ptr(), self, NULL ) );   
    if( !result.ptr() )
        return NULL;

    // Clear the vector stored in the struct field
    self -> vector.clear();

    CSP_RETURN_NONE;
}

template<typename StorageT>
static int
py_struct_list_ass_item( PyObject * sself, Py_ssize_t index, PyObject * value )
{
    CSP_BEGIN_METHOD;
    
    PyStructList<StorageT> * self = ( PyStructList<StorageT> * ) sself;

    // Deal with Python list indices that can be negative
    int sz = self -> vector.size();
    if( index < 0 )
        index += sz;
    
    PyObjectPtr result;
    // The value is not NULL -> assign it to vector[ index ]
    if( value != NULL )
    {
        Py_INCREF( value );
        if( PyList_SetItem( ( PyObject * ) self, index, value ) < 0 )
            return -1;
    }
    // The value is NULL -> erase vector[ index ]
    else
    {
        PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "__delitem__" ) );
        PyObjectPtr arguments = PyObjectPtr::own( PyTuple_Pack( 2, ( PyObject * ) self, PyLong_FromSsize_t( index ) ) );
        result = PyObjectPtr::own( PyObject_Call( func.ptr(), arguments.ptr(), NULL ) );
        if( !result.ptr() )
            return -1;
    }

    // Set the value in the vector stored in the struct field
    // Python doesn't allow indices for set_item to be past the array boundaries, so it is safe
    // The value is not NULL -> assign it to vector[ index ]
    assert( index >= 0 && index < sz );
    if( value != NULL )
    {
        self -> vector[ index ] = convertValue( self, value );
    }
    // The value is NULL -> erase vector[ index ]
    else
    {
        auto it = self -> vector.begin() + ( int ) index;
        self -> vector.erase( it );
    }
    
    CSP_RETURN_INT;
}

template<typename StorageT>
static int
py_struct_list_ass_subscript( PyObject * sself, PyObject * item, PyObject * value )
{
    CSP_BEGIN_METHOD;
    
    PyStructList<StorageT> * self = ( PyStructList<StorageT> * ) sself;
    // The item is the individual index
    if( !PySlice_Check( item ) )
    {
        Py_ssize_t index = PyNumber_AsSsize_t( item, PyExc_IndexError );
        if( index == -1 && PyErr_Occurred() )
            return -1;

        return py_struct_list_ass_item<StorageT>( ( PyObject * ) self, index, value );
    }

    // The item is a slice
    PyObjectPtr result;
    // The value is not NULL -> assign it to vector[ slice ]
    if( value != NULL )
    {
        PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "__setitem__" ) );
        PyObjectPtr arguments = PyObjectPtr::own( PyTuple_Pack( 3, ( PyObject * ) self, item, value ) );
        result = PyObjectPtr::own( PyObject_Call( func.ptr(), arguments.ptr(), NULL ) );
    }
    // The value is NULL -> erase vector[ slice ]
    else
    {
        PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "__delitem__" ) );
        PyObjectPtr arguments = PyObjectPtr::own( PyTuple_Pack( 2, ( PyObject * ) self, item ) );
        result = PyObjectPtr::own( PyObject_Call( func.ptr(), arguments.ptr(), NULL ) );
    }
    if( !result.ptr() )
        return -1;

    // Copy modified list into the vector stored in the struct field
    size_t sz = ( size_t ) PyObject_Length( ( PyObject * ) self );

    // Check that all elements of a modified vector are of correct type
    std::vector<StorageT> v( sz ); 
    for( size_t index = 0; index < sz; ++index )
    {
        PyObject * value = PyList_GET_ITEM( ( PyObject * ) self, index );
        v[ index ] = convertValue( self, value );
    }

    self -> vector.swap( v );

    CSP_RETURN_INT;
}

template<typename StorageT>
static PyObject *
py_struct_list_inplace_concat( PyObject * sself, PyObject * other )
{    
    CSP_BEGIN_METHOD;
    
    PyStructList<StorageT> * self = ( PyStructList<StorageT> * ) sself;
    PyObjectPtr arguments = PyObjectPtr::own( PyTuple_Pack( 1, other ) );
    PyObjectPtr result = PyObjectPtr::own( PyStructList_Extend<StorageT>( self, arguments.ptr() ) );
    if( !result.ptr() )
        return NULL;
    Py_INCREF( self );
    return self;

    CSP_RETURN_NULL;
}

template<typename StorageT>
static PyObject *
py_struct_list_inplace_repeat( PyObject * sself, Py_ssize_t n )
{    
    CSP_BEGIN_METHOD;
    
    PyStructList<StorageT> * self = ( PyStructList<StorageT> * ) sself;
    
    PyObjectPtr func = PyObjectPtr::own( PyObject_GetAttrString( ( PyObject * ) &PyList_Type, "__imul__" ) );
    PyObjectPtr result = PyObjectPtr::own( PyObject_CallFunction( func.ptr(), "On", self, n ) );   
    if( !result.ptr() )
        return NULL;

    // Emulate repeating on the vector stored in the struct field
    if( n <= 0 )
        self -> vector.clear();
    else
    {
        int sz = self -> vector.size();
        self -> vector.resize( sz * n );
        for( int i = 1; i < n; ++i )
            for( int j = 0; j < sz; ++j )
                self -> vector[ i * sz + j ] = self -> vector[ j ];
    }
    
    Py_INCREF( self );
    return self;

    CSP_RETURN_NULL;
}

template<typename StorageT>
static PyMethodDef PyStructList_methods[] = {
    {"append", ( PyCFunction ) PyStructList_Append<StorageT>, METH_VARARGS, NULL},
    {"insert", ( PyCFunction ) PyStructList_Insert<StorageT>, METH_VARARGS, NULL},
    {"pop", ( PyCFunction ) PyStructList_Pop<StorageT>, METH_VARARGS, NULL},
    {"reverse", ( PyCFunction ) PyStructList_Reverse<StorageT>, METH_NOARGS, NULL},
    {"sort", ( PyCFunction ) PyStructList_Sort<StorageT>, METH_VARARGS | METH_KEYWORDS, NULL},
    {"extend", ( PyCFunction ) PyStructList_Extend<StorageT>, METH_VARARGS, NULL},
    {"remove", ( PyCFunction ) PyStructList_Remove<StorageT>, METH_VARARGS, NULL},
    {"clear", ( PyCFunction ) PyStructList_Clear<StorageT>, METH_NOARGS, NULL},
    {NULL},
};

template<typename StorageT>
static PySequenceMethods py_struct_list_as_sequence = {
    PyList_Type.tp_as_sequence -> sq_length,                                /* sq_length */
    PyList_Type.tp_as_sequence -> sq_concat,                                /* sq_concat */
    PyList_Type.tp_as_sequence -> sq_repeat,                                /* sq_repeat */
    PyList_Type.tp_as_sequence -> sq_item,                                  /* sq_item */
    0,                                          /* sq_slice */
    py_struct_list_ass_item<StorageT>,                              /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    PyList_Type.tp_as_sequence -> sq_contains,                              /* sq_contains */
    py_struct_list_inplace_concat<StorageT>,                        /* sq_inplace_concat */
    py_struct_list_inplace_repeat<StorageT>                        /* sq_inplace_repeat */
};

template<typename StorageT>
static PyMappingMethods py_struct_list_as_mapping = {
    PyList_Type.tp_as_mapping -> mp_length,
    PyList_Type.tp_as_mapping -> mp_subscript,
    py_struct_list_ass_subscript<StorageT>
};

static PyObject *
PyStructList_new( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
    // Since the PyStructList has no real meaning when created from Python, we can reconstruct the PSL's value
    // by just treating it as a list. Thus, we simply override the tp_new behaviour to return a list object here.
    // Again, since we don't have tp_init for the PSL, we need to rely on the Python list's tp_init function.

    return PyObject_Call( ( PyObject * ) &PyList_Type, args, kwds ); // Calls both tp_new and tp_init for a Python list
}

template<typename StorageT>
static int
PyStructList_tp_clear( PyStructList<StorageT> * self )
{
    Py_CLEAR( self -> pystruct );
    PyObject * pself = ( PyObject * ) self;
    Py_TYPE( pself )-> tp_base -> tp_clear( pself );
    return 0;
}

template<typename StorageT>
static int
PyStructList_traverse( PyStructList<StorageT> * self, visitproc visit, void * arg )
{
    Py_VISIT( self -> pystruct );
    PyObject * pself = ( PyObject * ) self;
    Py_TYPE( pself ) -> tp_base -> tp_traverse( pself, visit, arg );
    return 0;
}

template<typename StorageT>
static void
PyStructList_dealloc( PyStructList<StorageT> * self )
{    
    PyObject_GC_UnTrack( self );
    Py_CLEAR( self -> pystruct );
    PyObject * pself = ( PyObject * ) self;
    Py_TYPE( pself ) -> tp_base -> tp_dealloc( pself );
}

template<typename StorageT> PyTypeObject PyStructList<StorageT>::PyType = {
    .ob_base = PyVarObject_HEAD_INIT( NULL, 0 )
    .tp_name = "_cspimpl.PyStructList",
    .tp_basicsize = sizeof( PyStructList<StorageT> ),
    .tp_itemsize = 0,
    .tp_dealloc = ( destructor ) PyStructList_dealloc<StorageT>,
    .tp_as_sequence = &py_struct_list_as_sequence<StorageT>,
    .tp_as_mapping = &py_struct_list_as_mapping<StorageT>,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_LIST_SUBCLASS,
    .tp_traverse = ( traverseproc ) PyStructList_traverse<StorageT>,
    .tp_clear = ( inquiry ) PyStructList_tp_clear<StorageT>,
    .tp_methods = PyStructList_methods<StorageT>,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new   = PyStructList_new,
    .tp_free  = PyObject_GC_Del,
};

}

#endif