# tools

Common tools for various projects. Most of them depend on libdansdl2 but are developed to fullfill very specifics needs (much too specific to get a part in the main library).

All components are linked into a single library (static or shared).

As for July 2016 these tools have been translated into english and retooled to some point (some things have been changed, all interfaces have been redone and some utilities have been deleted). As a result of this, all compatibility with the previous versions has been broken. There should still be copies in the branches "master" and "classic" of the "herramientas_proyecto" repository, but these will be developed no more.

# compatibility

As of April 2020 the dnot parser has been retired. RapidJson is introduced to take its place.
As of January 2020 compatibility with previous versions is broken, as the project has underwent a file reorganization.

# building

Must be built with a compiler that accepts the c++17 standard.

Requires gcc 7.5 or newer. With 7.5 versions requires the stdc++fs library to be added to the linker.

Requires the rapidjson library at https://github.com/Tencent/rapidjson

## License usage

Uses rapidjson, from Tencent, licensed under the MIT license.

## TODO

- Properly document the use of the more complicated tools (like the menu).
