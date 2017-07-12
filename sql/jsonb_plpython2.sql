CREATE EXTENSION jsonb_plpython2u CASCADE;

-- test jsonb -> python
CREATE FUNCTION test1(val jsonb) RETURNS int
LANGUAGE plpython2u
TRANSFORM FOR TYPE jsonb
AS $$
assert isinstance(val, dict)
plpy.info(sorted(val.items()))
return len(val)
$$;

SELECT test1('{"a":1, "c":"NULL"}'::jsonb);

DROP EXTENSION jsonb_plpython2u CASCADE;
