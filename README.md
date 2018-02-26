Virtual Texture Tile Creator
============================

I wrote this program for use within my master's thesis research.
Specifically my intended use was to create virtual texture tiles for Crytek's Sponza scene, which I rendered within a path tracer.
Feel free to use this work for reference. It is distributed as open source under the MIT License.
My master's thesis is called Virtual Textures for Path Tracing and can be requested as a PDF on my website [hanscronau.com].

~Hans Cronau


Installation
------------

This software uses CMake and requires compiled versions of Boost.filesystem and DevIL.
Make sure to have all installed and CMake pointed to the correct libraries.


### Having issues with Boost.filesystem?
I found that it was a bit of a paint getting Boost to compile, finding it with this project's CMake, and liking it on my 64bit Windows 10 system.
Here's how I got it to work:

- Download and install the latest CMake.
- Download the latest Boost and extract to C:/Program Files/boost/boost_x_xx_x
- Find a way to talk to your Visual Studio Command Prompt (cl). See tips below.
- Compile Boost.filesystem. See tips below.

Tips on talking to your Visual Studio Command Prompt:

  - Take a look at [Boost, Getting Started, Or Build From The Command Prompt].
  - I looked for a corresponding shortcut in my Start menu and opened it as administrator (RMB).
  - Another way is to open any command prompt and (cmd) as administrator and run C:\Program Files (x86)\Microsoft Visual Studio #\VC\vcvarsall.bat.

Tips on compilation:

- Take a look at [Prepare to use a Boost Library Binary].
  - Sadly [5.1 Simplified Build from Source] is a bit too simple.
  - Based on [5.2.4 Invoke b2] and [Invocation] I composed the following commands matching my setup in CMake.
    They are to be executed from the command prompt opened above, at C:/Program Files/boost/boost_x_xx_x:
      ```shell
      bootstrap
      .\b2 filesystem variant=debug,release link=static threading=multi address-model=64 toolset=msvc-12.0 stage
      ```
- Setup the CMakeLists.txt to include and link Boost.
  (I've already done this for this project's CMakeLists.txt. You don't need to do it again.)
  - Find instructions at https://cmake.org/cmake/help/v3.0/module/FindBoost.html.
  - ProTip: Ctrl+F for "filesystem".
- Throughout the above use a single platform architecture ((native) 32bit, (native) 64bit) between CMake, Boost, and DevIL.


[hanscronau.com]: https://hanscronau.com

[Boost, Getting Started, Or Build From The Command Prompt]: http://www.boost.org/doc/libs/1_64_0/more/getting_started/windows.html#or-build-from-the-command-prompt

[Prepare to use a Boost Library Binary]: http://www.boost.org/doc/libs/1_64_0/more/getting_started/windows.html#prepare-to-use-a-boost-library-binary

[5.1 Simplified Build from Source]: http://www.boost.org/doc/libs/1_64_0/more/getting_started/windows.html#simplified-build-from-source

[5.2.4 Invoke b2]: http://www.boost.org/doc/libs/1_64_0/more/getting_started/windows.html#invoke-b2

[Invocation]: http://www.boost.org/build/doc/html/bbv2/overview/invocation.html
