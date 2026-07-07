/* Sun-$Revision: 30.9 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation "deps.hh"
# include "_deps.cpp.incl"

nmln* dependencyList::dependentsArrayS;

// Net dep-buffer nodes spliced into map dependent lists by add() since the last
// migrate (new_nmethod) or remove().  If nonzero when a new dependencyList
// starts, the previous lookup abandoned its dependency nodes in the (heap)
// dependent lists -- they must be unlinked before this list reuses the shared
// buffer slots, else add()'s top->init() turns a still-linked node into a
// self-loop and any later walk of that map's list (e.g. nmln::invalidate from
// slotsMapDeps::define during fileIn) spins forever.  g_lastDepTop is the high
// end of the previous fill so the abandoned nodes can be found.
int   g_pendingDeps = 0;
nmln* g_lastDepTop  = NULL;

# if GENERATE_DEBUGGING_AIDS
  bool dependencyList::includes(nmln* dep) {
    for (nmln* d = dependentsArrayS; d < top; d ++) {
      // check to see if already in dependents array
      if (d->next == dep || d->prev == dep) {
        return true;
      }
    }
    return false;
  }
# endif

void dependencyList::add(nmln* dep) {
  if (alreadyThere(dep)) {
    return;
  }
  if (length() >= DependentsArraySize) {
    fatal("dependency list overflow");
  }
  top->init();
  top->check_alignment(); // for Intel
  dep->add(top);
  top++;
  { extern int g_pendingDeps; extern nmln* g_lastDepTop;
    g_pendingDeps++; g_lastDepTop = top; }
}

void dependencyList::remove() {
  for (nmln* d = dependentsArrayS; d < top; d ++) {
    d->remove();
  }
  { extern int g_pendingDeps; g_pendingDeps = 0; }
}

objectLookupTarget** assignableDependencyList::dependentsArrayS;

void assignableDependencyList::add(objectLookupTarget* target) {
  for (objectLookupTarget** t = dependentsArrayS; t < top; t ++) {
    if (*t == target) {
      // already in dependency list; ignore this one
      return;
    }
  }
  if (length() >= AssignableDependentsArraySize) {
    ShouldNotReachHere(); // assignable dependency list overflow
  }
  *top++ = target;
}
