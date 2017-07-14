CREATE EXTENSION jsonb_plpython2u CASCADE;

-- test jsonb -> python dict
CREATE FUNCTION test1(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, dict)
plpy.info(sorted(val.items()))
return len(val)
$$;

SELECT test1('{"a":1, "c":"NULL"}'::jsonb);

-- test jsonb -> python dict
-- complex dict with dicts as value
CREATE FUNCTION test1complex(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, dict)
assert(val == {"d":{"d": 1}})
return len(val)
$$;

SELECT test1complex('{"d":{"d": 1}}'::jsonb);


-- test jsonb[] -> python dict
-- dict with array as value
CREATE FUNCTION test1arr(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, dict)
assert(val == {"d": [12,1]})
return len(val)
$$;

SELECT test1arr('{"d":[12,1]}'::jsonb);

-- test jsonb[] -> python list
-- simple list
CREATE FUNCTION test2arr(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, list)
assert(val == [12,1])
return len(val)
$$;

SELECT test2arr('[12,1]'::jsonb);

-- test jsonb[] -> python list
-- array of dicts
CREATE FUNCTION test3arr(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, list)
assert(val == [{"a":1,"b":2},{"c":3,"d":4}])
return len(val)
$$;

SELECT test3arr('[{"a":1,"b":2},{"c":3,"d":4}]'::jsonb);

-- test jsonb int -> python int
CREATE FUNCTION test1int(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert(val == [1])
return len(val)
$$;

SELECT test1int('1'::jsonb);

-- test jsonb string -> python string
CREATE FUNCTION test1string(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert(val == ["a"])
return len(val)
$$;

SELECT test1string('"a"'::jsonb);


DROP EXTENSION jsonb_plpython2u CASCADE;
