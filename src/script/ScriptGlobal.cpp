#include "ScriptGlobal.h"

#include "../container/Pool.h"
#include "../container/Table.h"
#include "../parallelism/Lock.h"

using namespace L;

static Pool<ScriptGlobal::Slot> slots;
static Table<Symbol, ScriptGlobal::Slot*> slots_by_name;
static Lock slots_lock;

ScriptGlobal::ScriptGlobal(const Symbol& name) {
  L_SCOPED_LOCK(slots_lock);
  if(Slot** found = slots_by_name.find(name)) {
    _slot = *found;
  } else {
    slots_by_name[name] = _slot = slots.construct();
    _slot->name = name;
  }
}

Array<Symbol> ScriptGlobal::get_all_names() {
  L_SCOPED_LOCK(slots_lock);
  Array<Symbol> names;
  for(const auto& slot : slots_by_name) {
    names.push(slot.key());
  }
  return names;
}

Stream& L::operator<=(Stream& s, const ScriptGlobal& v) {
  return s <= v._slot->name;
}
Stream& L::operator>=(Stream& s, ScriptGlobal& v) {
  Symbol name;
  s >= name;
  v = ScriptGlobal(name);
  return s;
}
