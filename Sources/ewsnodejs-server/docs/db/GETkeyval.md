
# [Open Speech Platform - Server API](../api.md)

## GET /db/:table/:keyname

Get all keys, or the corresponding value for a key in the given table

### Return type

If a key was specified:

    Value as blob type.

If no key was specified:

    a json list of keys

### Responses

Code | Meaning
--- | ---
200 | Success
404 | table not found
500 | key not found


### Examples

```python
URL = "http://localhost:5000/api/db/test_table/myKeyname"
res = requests.get(URL)
assert res.text == "value_of_mykeyname"
```

```python
URL = "http://localhost:5000/api/db/test_table"
res = requests.get(URL)
# res.json() is list of keys
```