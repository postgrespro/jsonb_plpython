#include "postgres.h"

#include "fmgr.h"
#include "plpython.h"
#include "plpy_typeio.h"
#include "hstore.h"
#include "jsonb.h"
#include "fmgrprotos.h"

PG_MODULE_MAGIC;

extern void _PG_init(void);

/* Linkage to functions in plpython module */
typedef char *(*PLyObject_AsString_t) (PyObject *plrv);
static PLyObject_AsString_t PLyObject_AsString_p;
#if PY_MAJOR_VERSION >= 3
typedef PyObject *(*PLyUnicode_FromStringAndSize_t) (const char *s, Py_ssize_t size);
static PLyUnicode_FromStringAndSize_t PLyUnicode_FromStringAndSize_p;
#endif

/* Linkage to functions in hstore module */
typedef HStore *(*hstoreUpgrade_t) (Datum orig);
static hstoreUpgrade_t hstoreUpgrade_p;
typedef int (*hstoreUniquePairs_t) (Pairs *a, int32 l, int32 *buflen);
static hstoreUniquePairs_t hstoreUniquePairs_p;
typedef HStore *(*hstorePairs_t) (Pairs *pairs, int32 pcount, int32 buflen);
static hstorePairs_t hstorePairs_p;
typedef size_t (*hstoreCheckKeyLen_t) (size_t len);
static hstoreCheckKeyLen_t hstoreCheckKeyLen_p;
typedef size_t (*hstoreCheckValLen_t) (size_t len);
static hstoreCheckValLen_t hstoreCheckValLen_p;

/* Linkage to functions in jsonb module */

/*
 * Module initialize function: fetch function pointers for cross-module calls.
 */
void
_PG_init(void)
{
	/* Asserts verify that typedefs above match original declarations */
	AssertVariableIsOfType(&PLyObject_AsString, PLyObject_AsString_t);
	PLyObject_AsString_p = (PLyObject_AsString_t)
		load_external_function("$libdir/" PLPYTHON_LIBNAME, "PLyObject_AsString",
							   true, NULL);
#if PY_MAJOR_VERSION >= 3
	AssertVariableIsOfType(&PLyUnicode_FromStringAndSize, PLyUnicode_FromStringAndSize_t);
	PLyUnicode_FromStringAndSize_p = (PLyUnicode_FromStringAndSize_t)
		load_external_function("$libdir/" PLPYTHON_LIBNAME, "PLyUnicode_FromStringAndSize",
							   true, NULL);
#endif
	AssertVariableIsOfType(&hstoreUpgrade, hstoreUpgrade_t);
	hstoreUpgrade_p = (hstoreUpgrade_t)
		load_external_function("$libdir/hstore", "hstoreUpgrade",
							   true, NULL);
	AssertVariableIsOfType(&hstoreUniquePairs, hstoreUniquePairs_t);
	hstoreUniquePairs_p = (hstoreUniquePairs_t)
		load_external_function("$libdir/hstore", "hstoreUniquePairs",
							   true, NULL);
	AssertVariableIsOfType(&hstorePairs, hstorePairs_t);
	hstorePairs_p = (hstorePairs_t)
		load_external_function("$libdir/hstore", "hstorePairs",
							   true, NULL);
	AssertVariableIsOfType(&hstoreCheckKeyLen, hstoreCheckKeyLen_t);
	hstoreCheckKeyLen_p = (hstoreCheckKeyLen_t)
		load_external_function("$libdir/hstore", "hstoreCheckKeyLen",
							   true, NULL);
	AssertVariableIsOfType(&hstoreCheckValLen, hstoreCheckValLen_t);
	hstoreCheckValLen_p = (hstoreCheckValLen_t)
		load_external_function("$libdir/hstore", "hstoreCheckValLen",
							   true, NULL);
}


/* These defines must be after the module init function */
#define PLyObject_AsString PLyObject_AsString_p
#define PLyUnicode_FromStringAndSize PLyUnicode_FromStringAndSize_p
#define hstoreUpgrade hstoreUpgrade_p
#define hstoreUniquePairs hstoreUniquePairs_p
#define hstorePairs hstorePairs_p
#define hstoreCheckKeyLen hstoreCheckKeyLen_p
#define hstoreCheckValLen hstoreCheckValLen_p

PyObject *PyObject_FromJsonb(JsonbContainer *jsonb, PyObject *decimal_constructor);

PyObject *PyObject_FromJsonbValue(JsonbValue jsonbValue, PyObject *decimal_constructor){
	PyObject *result;
	char *str;
	switch (jsonbValue.type){
		case jbvNull:
			result = Py_None;
			break;
		case jbvBinary:
			result = PyObject_FromJsonb(jsonbValue.val.binary.data, decimal_constructor);
			break;
		case jbvNumeric:
			str = DatumGetCString(
					DirectFunctionCall1(numeric_out, jsonbValue.val.numeric)
					);
			result = PyObject_CallFunction(
					decimal_constructor, "s", str
					);
			break;
		case jbvString:
			result = PyString_FromStringAndSize(
					jsonbValue.val.string.val,
					jsonbValue.val.string.len
					);
			break;
		case jbvBool:
			result = jsonbValue.val.boolean ? Py_True: Py_False;
			break;
		case jbvArray:
			result = PyString_FromStringAndSize("ValArr",6);
			break;
	}
	return (result);
}

PyObject *PyObject_FromJsonb(JsonbContainer *jsonb, PyObject *decimal_constructor){
	PyObject   *object = Py_None;
	JsonbIterator	*it;
	JsonbIteratorToken r;
	JsonbValue v;

				object = PyDict_New();
	it = JsonbIteratorInit(jsonb);

	while ((r = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
	{
		PyObject   *key = Py_None;
		PyObject *value = Py_None;

		switch (r){
			case (WJB_KEY):
				key = PyString_FromStringAndSize(
						v.val.string.val,
						v.val.string.len
						);

				r = JsonbIteratorNext(&it, &v, true);
				value = PyObject_FromJsonbValue(v,decimal_constructor);
				PyDict_SetItem(object, key, value);
				Py_XDECREF(value);
				Py_XDECREF(key);
				break;
			case (WJB_BEGIN_ARRAY):
				object = PyList_New(0);
				while (
						((r = JsonbIteratorNext(&it, &v, true)) == WJB_ELEM)
						&&(r!=WJB_DONE)
						)
					PyList_Append(object, PyObject_FromJsonbValue(v, decimal_constructor));
				return (object);
				break;
			default:
				break;
		}
	}
	return (object);
}

PG_FUNCTION_INFO_V1(jsonb_to_plpython);

Datum
jsonb_to_plpython(PG_FUNCTION_ARGS)
{
	Jsonb	   *in = PG_GETARG_JSONB(0);

	PyObject   *dict;
	//TODO make next 2 variables GLOBAL!!!!
	PyObject *decimal_module;
	PyObject *decimal_constructor;
	decimal_module = PyImport_ImportModule("cdecimal");
	if (!decimal_module){
		PyErr_Clear();
		decimal_module = PyImport_ImportModule("decimal");
	}
	decimal_constructor = PyObject_GetAttrString(decimal_module, "Decimal");

	dict = PyObject_FromJsonb(&in->root, decimal_constructor);

	//TODO free allocated memory

	return PointerGetDatum(dict);
}


PG_FUNCTION_INFO_V1(plpython_to_jsonb);

Datum
plpython_to_jsonb(PG_FUNCTION_ARGS)
{
	PyObject   *obj;
	volatile PyObject *items_v = NULL;
	int32		pcount;
	JsonbValue	   *out;
	JsonbParseState *jsonb_state = NULL;

	obj = (PyObject *) PG_GETARG_POINTER(0);
	/*
	if (!PySequence_check(obj))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("not a Python sequence")));
	*/
	if(PyMapping_Check(obj)){
		pcount = PyMapping_Size(obj);
		items_v = PyMapping_Items(obj);

		PG_TRY();
		{
			int32		i;
			PyObject   *items = (PyObject *) items_v;
			JsonbValue	   jbvValue;
			JsonbValue	   jbvKey;

			pushJsonbValue(&jsonb_state,WJB_BEGIN_OBJECT, NULL);

			for (i = 0; i < pcount; i++)
			{
				PyObject   *tuple;
				PyObject   *key;
				PyObject   *value;

				tuple = PyList_GetItem(items, i);
				key = PyTuple_GetItem(tuple, 0);
				value = PyTuple_GetItem(tuple, 1);

				if (key == Py_None){
					jbvKey.type = jbvString;
					jbvKey.val.string.len = 0;
					jbvKey.val.string.val = "";
				}
				else{
					jbvKey.type = jbvString;
					jbvKey.val.string.val = PLyObject_AsString(key);
					jbvKey.val.string.len = strlen(jbvKey.val.string.val);
				}
				pushJsonbValue(&jsonb_state,WJB_KEY,&jbvKey);
				if (value == Py_None){
					jbvValue.type = jbvString;
					jbvValue.val.string.val = "";
					jbvValue.val.string.len = 0;
				}
				else{
					jbvValue.type = jbvString;
					//TODO AsString - rewrite
					jbvValue.val.string.val = PLyObject_AsString(value);
					jbvValue.val.string.len = strlen(jbvValue.val.string.val);
				}
				pushJsonbValue(&jsonb_state,WJB_VALUE,&jbvValue);
			}

			out = pushJsonbValue(&jsonb_state,WJB_END_OBJECT, NULL);
		}
		PG_CATCH();
		{
			Py_DECREF(items_v);
			PG_RE_THROW();
		}
		PG_END_TRY();
	}
	else
		if(PySequence_Check(obj)){
			JsonbValue	   jbvElem;

			pcount = PySequence_Size(obj);

			PG_TRY();
			{
				int32		i;

				pushJsonbValue(&jsonb_state,WJB_BEGIN_ARRAY, NULL);

				for (i = 0; i < pcount; i++)
				{
					PyObject   *value;

					value = PySequence_GetItem(obj, i);

					if (value == Py_None){
						jbvElem.type = jbvString;
						jbvElem.val.string.len = 0;
						jbvElem.val.string.val = "";
					}
					else{
						jbvElem.type = jbvString;
						jbvElem.val.string.val = PLyObject_AsString(value);
						jbvElem.val.string.len = strlen(jbvElem.val.string.val);
					}
					pushJsonbValue(&jsonb_state,WJB_ELEM,&jbvElem);
				}
				out = pushJsonbValue(&jsonb_state,WJB_END_ARRAY, NULL);
			}
			PG_CATCH();
			{
				PG_RE_THROW();
			}
			PG_END_TRY();
		}
	PG_RETURN_POINTER(JsonbValueToJsonb(out));
}
