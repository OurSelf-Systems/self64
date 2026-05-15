/* Sun-$Revision: 30.8 $ */

/* Copyright 1992-2012 AUTHORS.
   See the LICENSE file for license information. */

# pragma implementation  "spaceSize.hh"

# include "_spaceSize.cpp.incl"

unsigned long MaxHeapSize=        NMethodStart     - HeapStart;
unsigned long MaxNMethodSize=     StubsStart       - NMethodStart;
unsigned long MaxStubsSize=       DepsStart        - StubsStart;
unsigned long MaxDepsSize=        ScopesStart      - DepsStart;
unsigned long MaxScopesSize=      ZoneIDStart      - ScopesStart;
unsigned long MaxZoneIDSize=      CountStubIDStart - ZoneIDStart;
unsigned long MaxCountStubIDSize= UseCountStart    - CountStubIDStart;
unsigned long MaxUseCountSize=    AddrSpaceEnd     - UseCountStart;


#define MAKE_SPACE_SLOT_TEMPLATE(s)                                         \
        spaceObj= spaceObj->copy_add_slot(new_string(STR(s)),               \
                                          obj_slotType,                     \
                                          as_smiOop(sizes->s),              \
                                          Memory->slotAnnotationObj,        \
                                          true);                            \

static oop get_space_sizes(spaceSizes *sizes) {
  oop spaceObj = create_slots((slotList*)NULL);
  APPLY_TO_SPACE_SIZES(MAKE_SPACE_SLOT_TEMPLATE);
  return spaceObj;
}

oop get_default_space_sizes_prim(oop rcvrIgnored) {
  Unused(rcvrIgnored);
  return get_space_sizes(&(Memory->default_sizes)); }


#define FILL_IN_SPACE_SLOT_TEMPLATE(s)                                  \
        p= proto->get_slot_data_address_if_present(STR(s), inObj);      \
        if (p) {                                                        \
          if (!inObj) goto unassignable;                                \
          Memory->store(p, as_smiOop(Memory->current_sizes.s));         \
        }                                                               \


oop get_current_space_sizes_prim(oop rcvrIgnored, slotsOop proto, void *FH) {
  Unused(rcvrIgnored);
  oop *p;
  bool inObj;
  APPLY_TO_SPACE_SIZES(FILL_IN_SPACE_SLOT_TEMPLATE);
  return proto; 
 unassignable:
  prim_failure(FH, UNASSIGNABLESLOTERROR);
  return NULL;
}


#define SET_FROM_DEFAULT_TEMPLATE(s) \
        s= CONC(default_ , s);

void spaceSizes::set_from_defaults()
{
  APPLY_TO_SPACE_SIZES(SET_FROM_DEFAULT_TEMPLATE);
}


#define ROUND_TO_IDEALIZED_PAGE_SIZE_TEMPLATE(s) \
        s= roundTo(s, idealized_page_size);

void spaceSizes::cleanup()
{
  APPLY_TO_SPACE_SIZES(ROUND_TO_IDEALIZED_PAGE_SIZE_TEMPLATE);
}

void spaceSizes::apply_force_frequent_scavenges_clamp()
{
  if (!ForceFrequentScavengesViaSmallNewSpace) return;
  // Cap to whatever apply_force_frequent_scavenges_post_load picked at load
  // time (either default/10 for an empty snapshot, or 110% of occupancy for
  // a heavy one). This stops Self code from growing the saved sizes beyond
  // the chosen stress target, while still honoring the per-snapshot decision
  // about whether to tenure.
  // -- claude & dmu May 2026
  const smi cap_eden = Memory->current_sizes.eden_size;
  const smi cap_surv = Memory->current_sizes.surv_size;
  if (eden_size > cap_eden) eden_size = cap_eden;
  if (surv_size > cap_surv) surv_size = cap_surv;
}
