## Boostrap the database

You can prepare your Underpass installation with data for a specific country.

Go to the `utils` directory and run the boostrap script:

```
./bootstrap.sh -r south-america -c uruguay
```

Use `-p <PORT>` and `-u <USERNAME>` for the database.

For example, if you installed Underpass using Docker:

Regions (-r) are:

    africa
    asia
    australia-oceania
    central-america
    europe
    north-america
    south-america

Countries (-c) is the name of the country inside the region.

Data is downloaded from GeoFabrik, if you are not sure of what name you need to use, please check there.

For advanced users, check the [boostrap script documentation](/underpass/Dev/bootstrapsh).
