# B+

An OpenGL, SDL rendering framework for testing/playing with the tile-set generator in [WFC++](github.com/heyx3/WFCpp). Became a big-enough effort to split into its own repo for better organization.

# Dependencies

Dependencies are already in the repo, for a 64-bit Windows build (including the build products from WFC++ itself).

# Build

The code itself is designed to be platform-agnostic, but I'm a Windows developer and both WFC++ and B+ are Visual Studio 2017 projects, using C++17. I haven't actually tested this code on any other platforms.


# License

Copyright William Manning 2019. Released under the Lesser GNU Public License (LGPL).

https://www.gnu.org/licenses/lgpl-3.0.html

In a nutshell, as long as the software you release isn't just a modified version of this source code, you're probably fine. For example, you could release a closed-source commercial product that links to this engine through a DLL.