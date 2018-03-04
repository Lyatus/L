#pragma once

#include "script.h"
#include "../container/Array.h"
#include "../dynamic/Variable.h"
#include "../container/Ref.h"
#include "../container/Table.h"
#include "../text/Symbol.h"

namespace L {
  namespace Script {
    class Context {
    private:
      static Table<Symbol, Var> _globals;
      static Table<const TypeDescription*, Var> _typeTables;
      Array<Var> _stack, _selves;
      Array<uint32_t> _frames;
      Var _self;

    public:
      Context();

      inline Var& currentSelf() { L_ASSERT(!_selves.empty()); return _selves.back(); }
      inline Table<Var, Var>& selfTable() { return *_self.as<Ref<Table<Var, Var>>>(); }
      inline uint32_t currentFrame() const { L_ASSERT(!_frames.empty()); return _frames.back(); }
      inline uint32_t localCount() const { return uint32_t(_stack.size())-currentFrame(); }
      inline Var& local(uint32_t i) { return _stack[i+currentFrame()]; }
      inline Var& returnValue() { return local(uint32_t(-1)); }
      bool tryExecuteMethod(const Symbol&, std::initializer_list<Var> = {});
      Var executeInside(const Var& code);

      static inline Var& global(Symbol s) { return _globals[s]; }
      static Ref<Table<Var, Var>> typeTable(const TypeDescription*);
      static inline Var& typeValue(const TypeDescription* td, const Var& k) { return (*typeTable(td))[k]; }

    private:
      Var executeReturn(const Var& code); // Copies and return result of execution
      Var& executeRef(const Var& code); // Pushes result of exection on stack then returns ref to it
      void discardExecute(const Var& code); // Replaces top of the stack with result of execution
      void executeDiscard(const Var& code); // Discards result of execution
      void execute(const Var& code, Var* selfOut = nullptr); // Pushes result of execution on stack
      Var* reference(const Var& code, Var* selfOut = nullptr);
    };
  }
  inline Stream& operator<<(Stream& s, const Script::RawSymbol& v) { return s << '\'' << v.sym; }
}


