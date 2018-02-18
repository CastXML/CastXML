CastXML
*******

Introduction
============

CastXML is a C-family abstract syntax tree XML output tool.

This project is maintained by `Kitware`_ in support of `ITK`_,
the Insight Segmentation and Registration Toolkit.

.. _`Kitware`: http://www.kitware.com
.. _`ITK`: http://www.itk.org

Manual
======

See the `castxml(1)`_ manual page for instructions to run the tool.

.. _`castxml(1)`: doc/manual/castxml.1.rst

License
=======

CastXML is licensed under the `Apache License, Version 2.0`_.
See the `<LICENSE>`__ and `<NOTICE>`__ files for details.

.. _`Apache License, Version 2.0`: http://www.apache.org/licenses/LICENSE-2.0

Community
=========

Subscribe and post to the `CastXML Mailing List`_ for discussion of CastXML.

.. _`CastXML Mailing List`: http://public.kitware.com/mailman/listinfo/castxml

Build
=====

To build CastXML from source, first obtain the prerequisites:

* A C++ compiler supporting the ``c++11`` standard language level.

* `CMake`_ cross-platform build system generator.

* `LLVM/Clang`_ compiler SDK install tree built using the C++ compiler.
  This version of CastXML has been tested with LLVM/Clang

  - SVN revision ``319768`` (trunk)
  - Release ``5.0``
  - Release ``4.0``
  - Release ``3.9``
  - Release ``3.8``
  - Release ``3.7``
  - Release ``3.6``

* Optionally, the `Sphinx`_ documentation generator to build documentation.

Run CMake on the CastXML source tree to generate a build tree using
a C++ compiler compatible with that used to build the LLVM/Clang SDK.
CMake options include:

``Clang_DIR``
  Location of the LLVM/Clang SDK.  Set to ``<prefix>/lib/cmake/clang``,
  where ``<prefix>`` is the top of the LLVM/Clang SDK install tree.
  Alternatively, ``LLVM_DIR`` may be set to ``<prefix>/lib/cmake/llvm``.

``SPHINX_EXECUTABLE``
  Location of the ``sphinx-build`` executable.
  Required only if building documentation.

``SPHINX_HTML``
  Build documentation in ``html`` format.

``SPHINX_MAN``
  Build documentation in ``man`` format.

Run the corresponding native build tool (e.g. ``make``) in the CastXML
build tree, and optionally build the ``install`` target.  The ``castxml``
command-line tool may be used either from the build tree or the install tree.
The install tree is relocatable.

.. _`CMake`: http://www.cmake.org/
.. _`LLVM/Clang`: http://clang.llvm.org/
.. _`Sphinx`: http://sphinx-doc.org/
