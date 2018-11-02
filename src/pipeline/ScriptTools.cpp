#include "ScriptTools.h"

#include "../container/Table.h"
#include "../dev/profiling.h"

using namespace L;

static bool is_jump_inst(ScriptInstruction inst) {
  return inst.opcode==IterEndJump || inst.opcode==Jump || inst.opcode==CondJump || inst.opcode==CondNotJump;
}
static void remove_instruction(Script& script, uintptr_t index) {
  for(uintptr_t i(0); i<script.bytecode.size(); i++) {
    ScriptInstruction& inst(script.bytecode[i]);
    // Shift every jump that should be affected
    if(is_jump_inst(inst)) {
      uintptr_t ji(i+intptr_t(inst.bc));
      if(i<index && index<=ji) {
        inst.bc--;
      } else if(ji<=index && index<i) {
        inst.bc++;
      }
    }
    // Shift functions that should be affected
    else if(inst.opcode==LoadFun) {
      if(index<inst.bc) {
        inst.bc--;
      }
    }
  }
  script.bytecode.erase(index);
}
void ScriptTools::optimize(Script& script) {
  L_SCOPE_MARKER("Script optimization");
  { // Replace LoadConst by shortcuts when available
    for(ScriptInstruction& inst : script.bytecode) {
      if(inst.opcode==LoadConst) {
        const Var& const_value(script.constants[inst.b]);
        if(const_value.is<bool>()) {
          inst.opcode = LoadBool;
          inst.b = const_value.as<bool>();
        } else if(const_value.is<float>()) {
          const float float_value(const_value.as<float>());
          if(float_value==float(int16_t(float_value))) {
            inst.opcode = LoadInt;
            inst.bc = int16_t(float_value);
          }
        }
      }
    }
  }

  { // Remove null jumps
    for(uintptr_t i(0); i<script.bytecode.size(); i++) {
      if(is_jump_inst(script.bytecode[i]) && script.bytecode[i].bc==0) {
        remove_instruction(script, i);
        i--;
      }
    }
  }

  { // Remove unused constants
    // Gather all used constants
    Table<uintptr_t, bool> used_constants;
    for(uintptr_t i(0); i<script.bytecode.size(); i++) {
      if(script.bytecode[i].opcode==LoadConst) {
        used_constants[script.bytecode[i].b] = true;
      }
    }

    const size_t before_const_count(script.constants.size());
    uintptr_t after_i(0);
    for(uintptr_t before_i(0); before_i<before_const_count; before_i++) {
      if(!used_constants.find(before_i)) { // This constant is unused
        // Remove it from constant array
        script.constants.erase(after_i);

        // Walk through code to update constant indices
        for(uintptr_t j(0); j<script.bytecode.size(); j++) {
          ScriptInstruction& inst(script.bytecode[j]);
          if(inst.opcode==LoadConst && inst.b > after_i) {
            inst.b--;
          }
        }
      } else {
        after_i++;
      }
    }
  }
}

void ScriptTools::print_disassembly(const Script& script, Stream& s) {
  uintptr_t addr(0);
  for(const ScriptInstruction& i : script.bytecode) {
    s << ntos<10>(addr, 4) << ' ';
    switch(i.opcode) {
      case CopyLocal:    s << "CopyLocal:    " << i.a << " := " << i.b << "\n"; break;
      case LoadConst:    s << "LoadConst:    " << i.a << " := " << script.constants[i.b] << "\n"; break;
      case LoadBool:     s << "LoadBool:     " << i.a << " := " << (i.b!=0) << "\n"; break;
      case LoadInt:      s << "LoadInt:      " << i.a << " := " << i.bc << "\n"; break;
      case LoadGlobal:   s << "LoadGlobal:   " << i.a << " := " << script.globals[i.b].name() << "\n"; break;
      case StoreGlobal:  s << "StoreGlobal:  " << script.globals[i.a].name() << " := " << i.b << "\n"; break;
      case LoadFun:      s << "LoadFun:      " << i.a << " := " << i.bc << "\n"; break;

      case MakeObject:   s << "MakeObject:   " << i.a << " := {}\n"; break;
      case GetItem:      s << "GetItem:      " << i.c << " := " << i.a << "[" << i.b << "]\n"; break;
      case SetItem:      s << "SetItem:      " << i.a << "[" << i.b << "] := " << i.c << "\n"; break;

      case MakeIterator: s << "MakeIterator: " << i.a << " := it(" << i.b << ")\n"; break;
      case Iterate:      s << "Iterate:      (" << i.a << "," << i.b << ") := iter(" << i.c << ")\n"; break;
      case IterEndJump:  s << "IterEndJump:  iter_end(" << i.a << ") => " << i.bc << "\n"; break;

      case Jump:         s << "Jump:         " << i.bc << "\n"; break;
      case CondJump:     s << "CondJump:     " << i.a << " => " << i.bc << "\n"; break;
      case CondNotJump:  s << "CondNotJump:  !" << i.a << " => " << i.bc << "\n"; break;

      case Add:          s << "Add:          " << i.a << " += " << i.b << "\n"; break;
      case Sub:          s << "Sub:          " << i.a << " -= " << i.b << "\n"; break;
      case Mul:          s << "Mul:          " << i.a << " *= " << i.b << "\n"; break;
      case Div:          s << "Div:          " << i.a << " /= " << i.b << "\n"; break;
      case Mod:          s << "Mod:          " << i.a << " %= " << i.b << "\n"; break;
      case Inv:          s << "Inv:          " << i.a << " := inv(" << i.a << ")\n"; break;
      case Not:          s << "Not:          " << i.a << " := !" << i.a << "\n"; break;

      case LessThan:     s << "LessThan:     " << i.a << " := " << i.b << " < " << i.c << "\n"; break;
      case LessEqual:    s << "LessEqual:    " << i.a << " := " << i.b << " <= " << i.c << "\n"; break;
      case Equal:        s << "Equal:        " << i.a << " := " << i.b << " == " << i.c << "\n"; break;

      case Call:         s << "Call:         " << i.a << " (" << i.b << " parameters)\n"; break;
      case Return:       s << "Return\n"; break;
      default: error("Unknown script instruction to print"); break;
    }
    addr++;
  }
}