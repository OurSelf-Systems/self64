/* Copyright 2024-2026 Russell Allen.
   See the LICENSE file for license information. */

# ifdef INTERFACE_PRAGMAS
  # pragma interface
# endif


// Heap-allocated persistent PIC table for interpreter send-site caching.
// Maps method oops to per-method PIC data that persists across invocations.
// GC-integrated: scavenge_contents, gc_mark_contents, gc_unmark_contents,
// switch_pointers are called from universe during GC.

# if TARGET_IS_64BIT

struct InterpreterPICData {
  oop               method;     // key (method oop) — for GC traversal
  int32             num_pics;   // number of send sites in method
  int32             invocation_count; // activations; the SIC trigger input
  int32             map_len;    // length of pc_to_pic array
  InterpreterPIC*   pics;       // malloc'd array of PICs
  int16_t*          pc_to_pic;  // malloc'd PC→PIC index map
  InterpreterPICData* next;     // chain link for hash collision
};


class InterpreterPICTable : public CHeapObj {
  static const int TABLE_SIZE = 4099;
  InterpreterPICData* buckets[TABLE_SIZE];
  int32 _count;

  // Entries evicted by gc_weak_finalize() are unlinked from the buckets and
  // parked here (linked via ->next) rather than freed during GC.  They are
  // off the buckets, so no scavenge/mark/switch walk touches their (now dead)
  // method; drain_pending_free() frees them at a safe, non-GC point.
  InterpreterPICData* pending_free;

  int32 hash(oop method);
  void  rebuild_hash();

 public:
  InterpreterPICTable();
  ~InterpreterPICTable();

  InterpreterPICData* lookup(oop method);
  InterpreterPICData* lookup_or_create(oop method, int32 num_codes,
                                       u_char* codes);

  void invalidate_all();
  void flush_all();

  // GC integration
  void scavenge_contents();
  void gc_mark_contents();
  // Weak-key finalization: run AFTER the full strong-mark closure (mirrors the
  // string table). Unlinks entries whose method is otherwise unreachable so the
  // PIC cache stops keeping transient methods (and their literals) alive. The
  // method oop is a WEAK key.
  void gc_weak_finalize();
  // Free entries parked by gc_weak_finalize(). Must be called at a non-GC point
  // where no interpreter references a parked entry's _pics/_pc_to_pic.
  void drain_pending_free();
  void gc_unmark_contents();
  void switch_pointers(oop from, oop to);
};

extern InterpreterPICTable* interpreter_pic_table;

# endif // TARGET_IS_64BIT
