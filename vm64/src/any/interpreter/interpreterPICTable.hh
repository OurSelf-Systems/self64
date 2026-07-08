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
  int32             invocation_count; // activations + taken back edges; the
                                      // SIC tier-up trigger input
  int32             tier_up_at; // 0: armed -- fires at the threshold, and
                                // stays armed after a success so other
                                // receiver maps get their own compiles
                                // (probeCache dedupes); >0: failure backoff,
                                // retry once the count reaches it; -1: off
                                // (dead-home block, or gave up)
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
  bool verify();
  // Drop just the entries whose cached result is `method` (used after a
  // tier-up compile so those sites re-look-up and route to the new nmethod,
  // leaving every other method's type feedback intact).
  void invalidate_entries_caching(oop method);
  // Re-arm every tier-up trigger: backoff (>0) and off (-1) alike memoize
  // compile outcomes and home/promotion decisions made against code-cache
  // contents, so a whole-cache flush invalidates their premise (a promoted
  // home's -1 would otherwise pin it interpreted forever once its nmethod is
  // gone).  Still-valid -1s get re-derived on the next hot crossing;
  // invocation counts and type feedback survive.
  void reset_tier_up_backoff();
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
