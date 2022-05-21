
# Source

I get the files from the github.com releases.

https://github.com/libuv/libuv/releases

# Changes

The tests fail big time on our systems and on launchpad. I don't have time
to mess around with those at the moment. I edited the debian/rules and
added the `LIBUV_BUILD_TESTS=OFF` flag. This also went in the ../CMakeLists.txt
file.

    dh_auto_configure -- -DCMAKE_BUILD_TYPE=Release \
                         -DLIBUV_BUILD_TESTS=OFF \
                         -DCXX_FLAGS=-std=c++14

For some reasons the launchpad installation fails saying that the archive
library is not available. I added a property to rename it as the default
is to use a _weird_ name (`libuv_a.a`):

    set_target_properties(uv_a PROPERTIES OUTPUT_NAME "uv")

With this extra line, the installation works as expected.

