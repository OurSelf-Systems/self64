.. _creating-primitives:

Creating Primitives
===================

Self primitives are methods implemented in C++ inside the Virtual Machine. This
page describes how to create new primitives. For how to *use* primitives from
Self code, see :ref:`primitives`.

There are three mechanisms for creating primitives, each suited to different
use cases:

.. list-table::
   :widths: 20 40 40
   :header-rows: 1

   * - Mechanism
     - Best for
     - Effort
   * - Inline primitives
     - Performance-critical operations (arithmetic, cloning, comparison)
     - Low — write one C++ function, add one table entry
   * - primitiveMaker glue
     - Wrapping C libraries (Unix, Quartz, X11, math)
     - Medium — write a template, run the generator
   * - Proxy primitives
     - Wrapping C structs as Self objects
     - Medium — a specialisation of primitiveMaker


Inline Primitives
-----------------

Inline primitives are C++ functions registered directly in the VM's primitive
table. They are the simplest mechanism and give full control over the
implementation.

Implementing the function
^^^^^^^^^^^^^^^^^^^^^^^^^

Create a C function with ``extern "C"`` linkage. The receiver is the first
argument. All arguments and the return value are ``oop`` (or a tagged subtype
like ``smiOop``). On failure, return an error oop.

Example — integer doubling primitive::

    // In vm64/src/any/prims/miscPrims.cpp (or a new file)

    extern "C" oop smi_double_prim(smiOop rcvr) {
      if (!oop(rcvr)->is_smi())
        return ErrorCodes::vmString_prim_error("badTypeError");

      smi a = smi(rcvr);
      smi result;
      if (__builtin_add_overflow(a, a, &result))
        return ErrorCodes::vmString_prim_error("overflowError");
      return oop(result);
    }

Registering in the primitive table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add an entry to one of the ``fntable`` arrays in
``vm64/src/any/prims/prim.cpp``::

    {
    "IntDouble", fntype(&smi_double_prim),
     IntArithmeticPrimitive, IntegerPrimType,
     NOSIDEEFFECTS,
     "Returns the receiver multiplied by two. May fail with overflow."
    },

Each entry has six fields:

1. **Name** — the primitive selector without the leading underscore (e.g.,
   ``"IntDouble"`` is invoked as ``_IntDouble`` from Self).

2. **Function pointer** — wrapped in ``fntype()``.

3. **primType** — tells the compiler what optimisations are safe:

   - ``ExternalPrimitive`` — general purpose (most primitives)
   - ``IntArithmeticPrimitive``, ``IntComparisonPrimitive`` — integer ops
   - ``FloatArithmeticPrimitive``, ``FloatComparisonPrimitive`` — float ops
   - ``ClonePrimitive`` — cloning
   - ``AtPrimitive``, ``AtPutPrimitive``, ``SizePrimitive`` — vector ops

4. **primExprType** — the return type hint:

   - ``IntegerPrimType``, ``FloatPrimType``, ``StringPrimType``
   - ``ReceiverPrimType`` — returns receiver
   - ``UnknownPrimType`` — no type information

5. **Side-effect flags** — a macro encoding six booleans:

   - ``NOSIDEEFFECTS`` — safe to reorder, constant-fold
   - ``SIDEEFFECTS`` — has side effects, cannot be moved
   - ``SIDEEFFECTS_CANABORT`` — may abort the process
   - ``SIDEEFFECTS_WALKSTACK`` — may walk the stack

6. **Documentation string** — shown by ``_Describe``.

Using it from Self
^^^^^^^^^^^^^^^^^^

After recompiling the VM::

    > 21 _IntDouble
    42


Failure handling
^^^^^^^^^^^^^^^^

Primitives can fail. The caller can supply a failure handler by appending
``IfFail:`` to the selector::

    > 'hello' _IntDoubleIfFail: [|:e| e printLine. -1]
    badTypeError
    -1

To support ``IfFail:``, your primitive can accept an optional failure handle
parameter. See ``glueDefs.hh`` for the ``passFailHandle`` mechanism.


The primitiveMaker System
-------------------------

The primitiveMaker is a Self program that reads a template file and generates
three outputs:

- A ``.primMaker.hh`` file — C++ macro defining primitive table entries
- A ``_glue.cpp`` file — C++ wrapper functions
- A ``_wrappers.self`` file — Self methods that call the primitives

This is the standard way to wrap C libraries for use from Self.

Template file format
^^^^^^^^^^^^^^^^^^^^

A template file is a Self script that invokes the primitiveMaker::

    primitiveMaker reader copy staticLinking create: 'mylib' From: '

      macroName: mylib

    traits: traits myObject
      visibility: publicSlot

      -- Each line defines one primitive:
      -- receiver args = resultType operation cFunction [flags]

      float sin = float call sin
      float raisedTo: float = float call pow
      void doSomething: string = int call my_c_func passFailHandle

    '

Template syntax
^^^^^^^^^^^^^^^

**Metadata directives** (before the first ``traits:`` line):

- ``macroName: name`` — base name for the generated C++ macro
- ``glueLibraryName: file.o`` — object file name for linking

**Trait target** — where the Self wrapper methods are added:

- ``traits: traits myObject`` — subsequent primitives go on this object
- You can have multiple ``traits:`` lines to target different objects

**Visibility:**

- ``visibility: publicSlot`` — wrappers are public
- ``visibility: privateSlot`` — wrappers are private

**Primitive definition lines:**

Each line has the form::

    receiver [arg1 [arg2 ...]] = resultType operation cName [flags]

Where:

- **receiver** and **args** are type conversion specs (see below)
- **resultType** is the conversion for the return value
- **operation** is one of: ``call``, ``get``, ``getMember``, ``set``,
  ``setMember``, ``new``, ``delete``
- **cName** is the C function or variable name
- **flags** (optional): ``cannotFail``, ``canAWS``, ``passFailHandle``

Type conversions
^^^^^^^^^^^^^^^^

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Type spec
     - Description
   * - ``oop``
     - Any Self object (no conversion)
   * - ``smi``
     - Small integer
   * - ``int``, ``unsigned_int``
     - C int (converted from/to smi)
   * - ``float``, ``double``
     - Floating point (converted from/to floatOop)
   * - ``bool``
     - Boolean (converted from/to true/false objects)
   * - ``string``
     - C string (from Self string or byte vector)
   * - ``bv_len char*``
     - Byte vector with length (two C args: pointer + length)
   * - ``void``
     - No value (for receiver: discard it; for result: return receiver)
   * - ``proxy TypeName SealName``
     - C pointer wrapped as a sealed proxy object

Example: wrapping a C library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Suppose you have a C library with::

    int widget_create(const char* name);
    void widget_destroy(int handle);
    float widget_get_width(int handle);

Create ``objects/glue/widgetTemplates.self``::

    primitiveMaker reader copy staticLinking create: 'widget' From: '

      macroName: widget

    traits: globals widgets
      visibility: publicSlot

      void create: string = int call widget_create passFailHandle
      int destroy = void call widget_destroy
      int getWidth = float call widget_get_width

    '

Running the generator
^^^^^^^^^^^^^^^^^^^^^

In a running Self VM::

    'objects/glue/widgetTemplates.self' _RunScript

Or equivalently, file in the template::

    bootstrap read: 'widgetTemplates' From: 'objects/glue'

This writes three files into the ``objects/glue/`` directory:

- ``widget.primMaker.hh`` — primitive table entries
- ``widget_glue.cpp`` — C++ glue functions
- ``widget_wrappers.self`` — Self wrapper methods

Build integration
^^^^^^^^^^^^^^^^^

1. Add ``widget_glue.cpp`` to ``vm64/src/CMakeLists.txt``
2. Add ``widget.primMaker.hh`` includes to the build (see existing
   ``prim_table_*.hh`` files for the pattern)
3. Add dependencies to ``vm64/build_support/includeDB.in``
4. Recompile the VM
5. File in the wrappers: ``'objects/glue/widget_wrappers.self' _RunScript``


Proxy Primitives
----------------

Proxies wrap C structs or pointers as Self objects with type-safe "seals".
They are a specialisation of the primitiveMaker system.

What is a proxy?
^^^^^^^^^^^^^^^^

A Self proxy object contains either:

- A **live pointer** to a C-allocated structure (dangerous — the C code
  owns the memory)
- A **dead copy** of the structure data (safe — Self owns a copy)

Each proxy carries a **seal** — a unique identifier that prevents type
confusion. Sending a proxy to a primitive that expects a different seal
will fail with ``badTypeSealError``.

Defining proxy types in templates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example: wrapping the Unix ``stat`` structure::

    primitiveMaker reader copy staticLinking create: 'stat' From: '

      macroName: stat

    traits: traits unixStatStructure
      visibility: publicSlot

      -- Reading struct fields (getMember):
      stat_type st_dev   = short          getMember st_dev
      stat_type st_mode  = unsigned_short getMember st_mode
      stat_type st_size  = long           getMember st_size

      -- Deleting the proxy:
      stat_type delete   = void           delete

    traits: globals os
      visibility: publicSlot

      -- Creating a proxy (call + deadCopy):
      void stat: string = stat_type {statStructure deadCopy} call stat_wrapper \
                          passFailHandle

    '

Key concepts:

- ``stat_type`` is both the C type name and the proxy type — the
  primitiveMaker creates a seal ``stat_type_seal`` automatically
- ``getMember field`` reads a struct field
- ``setMember field`` writes a struct field
- ``{statStructure deadCopy}`` means the result is a dead (copied) proxy
  of the given Self prototype
- ``delete`` frees the C-allocated memory
- ``new`` allocates a new C struct

The C wrapper function (``stat_wrapper`` in this example) must be
implemented manually in the ``_glue.cpp`` file, calling the C library
and returning a pointer to the struct.

Live vs dead proxies
^^^^^^^^^^^^^^^^^^^^

**Live proxies** hold a pointer to C-owned memory. They are dangerous
because the C code may free the memory at any time. Use live proxies
only for short-lived references.

**Dead proxies** hold a copy of the struct data inside the Self heap.
They are safe and survive garbage collection. Use ``{prototype deadCopy}``
in the template to create dead proxies.

Seals and type safety
^^^^^^^^^^^^^^^^^^^^^

Every proxy type has a unique seal. When a primitive receives a proxy
argument, the glue code checks that the proxy's seal matches the
expected type. If not, the primitive fails with ``badTypeSealError``.

This prevents accidentally passing a file descriptor proxy to a
function expecting a window handle proxy, even though both are
C pointers internally.


Reference
---------

Existing template files
^^^^^^^^^^^^^^^^^^^^^^^

The ``objects/glue/`` directory contains template files for the
standard library bindings:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Template
     - Description
   * - ``transcendentalTemplates.self``
     - Math functions (sin, cos, sqrt, pow, etc.)
   * - ``statTemplates.self``
     - Unix stat() struct access
   * - ``unixPrimTemplates.self``
     - Unix system calls (file I/O, processes, signals)
   * - ``quartzTemplates.self``
     - macOS Quartz/CoreGraphics windowing and drawing
   * - ``xlibTemplates.self``
     - X11 windowing (Linux)
   * - ``xftTemplates.self``
     - Xft font rendering (Linux)
   * - ``termcapTemplates.self``
     - Terminal capabilities

These serve as good examples for writing new templates.

Primitive failure error codes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See :ref:`primitives` for a complete list of error code strings
(``badTypeError``, ``overflowError``, ``badTypeSealError``, etc.).
