/* Copyright 2024-2026 Russell Allen.
   See the LICENSE file for license information. */

# pragma implementation "interpreterPICTable.hh"
# include "_interpreterPICTable.cpp.incl"
# include <cstdint>


# if TARGET_IS_64BIT

InterpreterPICTable* interpreter_pic_table = NULL;


InterpreterPICTable::InterpreterPICTable() {
  memset(buckets, 0, sizeof(buckets));
  _count = 0;
  pending_free = NULL;
}


InterpreterPICTable::~InterpreterPICTable() {
  flush_all();
}


int32 InterpreterPICTable::hash(oop method) {
  // Use oop address shifted right to remove tag bits.
  // Addresses move during GC, so we rebuild after GC.
  uintptr_t h = (uintptr_t)method;
  h = (h >> 3) ^ (h >> 17);
  return (int32)(h % TABLE_SIZE);
}


InterpreterPICData* InterpreterPICTable::lookup(oop method) {
  int32 idx = hash(method);
  for (InterpreterPICData* d = buckets[idx]; d; d = d->next) {
    if (d->method == method)
      return d;
  }
  return NULL;
}


InterpreterPICData* InterpreterPICTable::lookup_or_create(
    oop method, int32 num_codes, u_char* codes) {

  InterpreterPICData* d = lookup(method);
  if (d) return d;

  // Count send sites
  int32 num_sends = 0;
  for (int32 i = 0; i < num_codes; i++) {
    int op = getOp(codes[i]);
    if (op == SEND_CODE || op == IMPLICIT_SEND_CODE)
      num_sends++;
  }
  if (num_sends == 0)
    return NULL;

  // Allocate new entry
  d = (InterpreterPICData*)malloc(sizeof(InterpreterPICData));
  d->method   = method;
  d->num_pics = num_sends;
  d->invocation_count = 0;
  d->tier_up_at = 0;
  d->map_len  = num_codes;

  d->pc_to_pic = (int16_t*)malloc(num_codes * sizeof(int16_t));
  for (int32 i = 0; i < num_codes; i++)
    d->pc_to_pic[i] = -1;

  if (num_sends > INT16_MAX)
    num_sends = INT16_MAX;  // cap to avoid int16_t overflow; excess sends won't be cached

  d->pics = (InterpreterPIC*)malloc(num_sends * sizeof(InterpreterPIC));
  memset(d->pics, 0, num_sends * sizeof(InterpreterPIC));

  // Fill pc_to_pic map
  int32 pic_idx = 0;
  for (int32 i = 0; i < num_codes; i++) {
    int op = getOp(codes[i]);
    if (op == SEND_CODE || op == IMPLICIT_SEND_CODE) {
      if (pic_idx < num_sends)
        d->pc_to_pic[i] = (int16_t)pic_idx;
      pic_idx++;
    }
  }

  // Insert at head of bucket
  int32 idx = hash(method);
  d->next = buckets[idx];
  buckets[idx] = d;
  _count++;

  return d;
}


void InterpreterPICTable::invalidate_all() {
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      for (int32 j = 0; j < d->num_pics; j++) {
        d->pics[j].count = 0;
        memset(d->pics[j].hitCount, 0, sizeof(d->pics[j].hitCount));
      }
    }
  }
}


void InterpreterPICTable::invalidate_entries_caching(oop method) {
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      for (int32 j = 0; j < d->num_pics; j++) {
        InterpreterPIC& pic = d->pics[j];
        // compact the entries, dropping those whose cached result is `method`
        int32 dst = 0;
        for (int32 k = 0; k < pic.count; k++) {
          if (pic.entries[k].cachedMethod == method) continue;
          if (dst != k) {
            pic.entries[dst]    = pic.entries[k];
            pic.hitCount[dst]   = pic.hitCount[k];
            pic.resultType[dst] = pic.resultType[k];
            pic.slotOffset[dst] = pic.slotOffset[k];
            pic.adepsCount[dst] = pic.adepsCount[k];
            for (int32 a = 0; a < pic.adepsCount[k]; a++) {
              pic.adepsHolder[dst][a] = pic.adepsHolder[k][a];
              pic.adepsOffset[dst][a] = pic.adepsOffset[k][a];
              pic.adepsValue [dst][a] = pic.adepsValue [k][a];
            }
          }
          dst++;
        }
        if (dst != pic.count) {
          pic.count = (int8_t)dst;
          pic.next  = 0;  // only read once the PIC refills to PIC_SIZE
        }
      }
    }
  }
}


void InterpreterPICTable::flush_all() {
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    InterpreterPICData* d = buckets[i];
    while (d) {
      InterpreterPICData* next = d->next;
      free(d->pc_to_pic);
      free(d->pics);
      free(d);
      d = next;
    }
    buckets[i] = NULL;
  }
  _count = 0;
  drain_pending_free();
}


void InterpreterPICTable::rebuild_hash() {
  // Collect all entries, then re-insert.
  // Needed because oop addresses change during GC.
  InterpreterPICData* all = NULL;
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    InterpreterPICData* d = buckets[i];
    while (d) {
      InterpreterPICData* next = d->next;
      d->next = all;
      all = d;
      d = next;
    }
    buckets[i] = NULL;
  }
  while (all) {
    InterpreterPICData* next = all->next;
    int32 idx = hash(all->method);
    all->next = buckets[idx];
    buckets[idx] = all;
    all = next;
  }
}


// Update all oop pointers after scavenging (new-space objects may have moved)
void InterpreterPICTable::scavenge_contents() {
  bool need_rehash = false;
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      oop old_method = d->method;
      d->method = d->method->scavenge();
      if (d->method != old_method)
        need_rehash = true;

      for (int32 j = 0; j < d->num_pics; j++) {
        InterpreterPIC& pic = d->pics[j];
        for (int32 k = 0; k < pic.count; k++) {
          *(oop*)&pic.entries[k].cachedMap =
              oop(pic.entries[k].cachedMap)->scavenge();
          if (pic.entries[k].cachedMethod)
            pic.entries[k].cachedMethod =
                pic.entries[k].cachedMethod->scavenge();
          if (pic.entries[k].cachedHolder)
            pic.entries[k].cachedHolder =
                pic.entries[k].cachedHolder->scavenge();
          for (int32 a = 0; a < pic.adepsCount[k]; a++) {
            pic.adepsHolder[k][a] = pic.adepsHolder[k][a]->scavenge();
            pic.adepsValue [k][a] = pic.adepsValue [k][a]->scavenge();
          }
        }
      }
    }
  }
  if (need_rehash)
    rebuild_hash();
}


// Mark oop pointers during mark-sweep GC.
//
// d->method is a WEAK key — deliberately NOT marked here, so the PIC cache
// cannot keep an otherwise-dead method (and all its literals) alive.  Entries
// whose method is unreachable from a real root are dropped in gc_weak_finalize()
// after the full strong closure.  The cached send-site data (maps/methods/
// holders) IS strong-marked: those oops are non-leaf and need transitive marking
// during the strong phase, and surviving entries need them to stay valid.
void InterpreterPICTable::gc_mark_contents() {
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      for (int32 j = 0; j < d->num_pics; j++) {
        InterpreterPIC& pic = d->pics[j];
        for (int32 k = 0; k < pic.count; k++) {
          *(oop*)&pic.entries[k].cachedMap =
              oop(pic.entries[k].cachedMap)->gc_mark();
          if (pic.entries[k].cachedMethod)
            pic.entries[k].cachedMethod =
                pic.entries[k].cachedMethod->gc_mark();
          if (pic.entries[k].cachedHolder)
            pic.entries[k].cachedHolder =
                pic.entries[k].cachedHolder->gc_mark();
          for (int32 a = 0; a < pic.adepsCount[k]; a++) {
            pic.adepsHolder[k][a] = pic.adepsHolder[k][a]->gc_mark();
            pic.adepsValue [k][a] = pic.adepsValue [k][a]->gc_mark();
          }
        }
      }
    }
  }
}


// Weak-key finalization — must run AFTER the full strong-mark closure
// (universe::garbage_collect calls this right after object_table->gc_mark_rest).
// An entry whose method was not marked by a real root is unreachable except
// through this cache.  We UNLINK it from the buckets and park it on pending_free
// (freed later by drain_pending_free) — freeing here is unsafe (an interpreter
// may hold raw _pics/_pc_to_pic pointers into the entry, and freeing during GC
// risks the heap).  A running method has an active interpreter whose
// method_object is marked by processes->gc_mark_contents(), so it is never
// evicted while live.  Surviving entries' method pointers are forwarded.
void InterpreterPICTable::gc_weak_finalize() {
  bool need_rehash = false;
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    InterpreterPICData** pp = &buckets[i];
    while (*pp) {
      InterpreterPICData* d = *pp;
      if (!memOop(d->method)->is_gc_marked()) {
        // method is dead — unlink and park for later free (do NOT free now)
        *pp = d->next;
        d->next = pending_free;
        pending_free = d;
        _count--;
      } else {
        oop old_method = d->method;
        d->method = d->method->gc_mark();   // already marked; just forwards addr
        if (d->method != old_method)
          need_rehash = true;
        pp = &d->next;
      }
    }
  }
  if (need_rehash)
    rebuild_hash();
}


// Free entries parked by gc_weak_finalize().  Called from a non-GC point (the
// top of interpreter::attach_pics): any interpreter currently on the stack has a
// live (marked) method, so its entry was never parked — nothing references a
// parked entry's raw _pics/_pc_to_pic arrays.  d->method here is a dangling oop
// (its method was swept) but we never dereference it.
void InterpreterPICTable::drain_pending_free() {
  while (pending_free) {
    InterpreterPICData* d = pending_free;
    pending_free = d->next;
    free(d->pc_to_pic);
    free(d->pics);
    free(d);
  }
}


// Unmark all oop pointers after mark-sweep GC
void InterpreterPICTable::gc_unmark_contents() {
  bool need_rehash = false;
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      oop old_method = d->method;
      d->method = d->method->gc_unmark();
      if (d->method != old_method)
        need_rehash = true;

      for (int32 j = 0; j < d->num_pics; j++) {
        InterpreterPIC& pic = d->pics[j];
        for (int32 k = 0; k < pic.count; k++) {
          *(oop*)&pic.entries[k].cachedMap =
              oop(pic.entries[k].cachedMap)->gc_unmark();
          if (pic.entries[k].cachedMethod)
            pic.entries[k].cachedMethod =
                pic.entries[k].cachedMethod->gc_unmark();
          if (pic.entries[k].cachedHolder)
            pic.entries[k].cachedHolder =
                pic.entries[k].cachedHolder->gc_unmark();
          for (int32 a = 0; a < pic.adepsCount[k]; a++) {
            pic.adepsHolder[k][a] = pic.adepsHolder[k][a]->gc_unmark();
            pic.adepsValue [k][a] = pic.adepsValue [k][a]->gc_unmark();
          }
        }
      }
    }
  }
  if (need_rehash)
    rebuild_hash();
}


// Update any oop that matches 'from' to 'to' (used during programming)
void InterpreterPICTable::switch_pointers(oop from, oop to) {
  bool need_rehash = false;
  for (int32 i = 0; i < TABLE_SIZE; i++) {
    for (InterpreterPICData* d = buckets[i]; d; d = d->next) {
      if (d->method == from) {
        d->method = to;
        need_rehash = true;
      }
      for (int32 j = 0; j < d->num_pics; j++) {
        InterpreterPIC& pic = d->pics[j];
        for (int32 k = 0; k < pic.count; k++) {
          if (oop(pic.entries[k].cachedMap) == from)
            *(oop*)&pic.entries[k].cachedMap = to;
          if (pic.entries[k].cachedMethod && pic.entries[k].cachedMethod == from)
            pic.entries[k].cachedMethod = to;
          if (pic.entries[k].cachedHolder && pic.entries[k].cachedHolder == from)
            pic.entries[k].cachedHolder = to;
          for (int32 a = 0; a < pic.adepsCount[k]; a++) {
            if (pic.adepsHolder[k][a] == from) pic.adepsHolder[k][a] = to;
            if (pic.adepsValue [k][a] == from) pic.adepsValue [k][a] = to;
          }
        }
      }
    }
  }
  if (need_rehash)
    rebuild_hash();
}

# endif // TARGET_IS_64BIT
