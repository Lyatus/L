#include "Context.h"

using namespace L;
using namespace Script;

Context::Context() {
  _frames.push(0); // Start current frame
  _frames.push(0); // Start next frame (no variable)
  variable(symbol("do")) = (Native)[](Context& c,const Array<Var>& a)->Var {
    for(int i(1); i<a.size()-1; i++)
      c.execute(a[i]);
    return c.execute(a.back());
  };
  variable(symbol("while")) = (Native)[](Context& c,const Array<Var>& a)->Var {
    Var wtr;
    while(c.execute(a[1]).get<bool>())
      wtr = c.execute(a[2]);
    return wtr;
  };
  variable(symbol("if")) = (Native)[](Context& c,const Array<Var>& a)->Var {
    if(a.size()>2) {
      if(c.execute(a[1]).get<bool>())
        return c.execute(a[2]);
      else if(a.size()>3)
        return c.execute(a[3]);
    }
    return 0;
  };
  variable(symbol("set")) = (Native)[](Context& c,const Array<Var>& a)->Var {
    Var target(c.execute(a[1]));
    if(a.size()==3) {
      if(target.is<Symbol>())
        return c.variable(target.as<Symbol>()) = c.execute(a[2]);
      else if(target.is<Var*>())
        return *target.as<Var*>() = c.execute(a[2]);
    }
    return 0;
  };
  variable(symbol("typename")) = (Function)[](Context& c,int params)->Var {
    return c.parameter(0).type()->name;
  };
  variable(symbol("=")) = (Function)[](Context& c,int params)->Var {
    if(params==2)
      return c.parameter(0)==c.parameter(1);
    else return false;
  };
  variable(symbol("+")) = (Function)[](Context& c,int params)->Var {
    int wtr(0);
    for(int i(0); i<params; i++)
      if(c.parameter(i).is<int>())
        wtr += c.parameter(i).as<int>();
    return wtr;
  };
  variable(symbol("*")) = (Function)[](Context& c,int params)->Var {
    int wtr(1);
    for(int i(0); i<params; i++)
      if(c.parameter(i).is<int>())
        wtr *= c.parameter(i).as<int>();
    return wtr;
  };
  variable(symbol("/")) = (Function)[](Context& c,int params)->Var {
    if(params==2 && c.parameter(0).is<int>() && c.parameter(1).is<int>()) {
      return c.parameter(0).as<int>()/c.parameter(1).as<int>();
    } else return 0;
  };
  variable(symbol("%")) = (Function)[](Context& c,int params)->Var {
    if(params==2 && c.parameter(0).is<int>() && c.parameter(1).is<int>()) {
      return c.parameter(0).as<int>()%c.parameter(1).as<int>();
    } else return 0;
  };
  variable(symbol("print")) = (Function)[](Context& c,int params)->Var {
    for(int i(0); i<params; i++)
      out << c.parameter(i);
    return 0;
  };
}
void Context::read(Stream& stream) {
  Script::Lexer lexer(stream);
  Var v;
  lexer.nextToken();
  while(!stream.end()) {
    read(v,lexer);
    execute(v);
  }
}
void Context::read(Var& v, Lexer& lexer) {
  if(lexer.acceptToken("(")) { // It's a list of expressions
    v = Array<Var>();
    int i(0);
    while(!lexer.acceptToken(")"))
      read(v[i++],lexer);
  } else if(lexer.acceptToken("'")) {
    v = Quote();
    read(v.as<Quote>().var,lexer);
  } else if(lexer.acceptToken("!")) {
    read(v,lexer);
    v = execute(v);
  } else {
    const char* token(lexer.token());
    if(lexer.literal()) v = token; // Character string
    else if(lexer.isToken("true")) v = true;
    else if(lexer.isToken("false")) v = false;
    else if(token[strspn(token,"-0123456789")]=='\0') v = atoi(token); // Integer
    else if(token[strspn(token,"-0123456789.")]=='\0') v = (float)atof(token); // Float
    else v = symbol(token);
    lexer.nextToken();
  }
}

Symbol Context::symbol(const char* str) {
  // 32-bit FNV-1a algorithm
  uint32 wtr(2166136261);
  while(*str) {
    wtr ^= *str;
    wtr *= 16777619;
    str++;
  }
  return {wtr};
}
Var& Context::variable(Symbol sym) {
  // Search the known variable
  for(int i(nextFrame()-1); i>=0; i--)
    if(_stack[i].key().id==sym.id)
      return _stack[i].value();
  pushVariable(sym);
  return _stack.back().value();
}
void Context::pushVariable(Symbol sym, const Var& v) {
  _stack.push(keyValue(sym,v));
  _frames.back()++; // Added variable to current frame, must push next frame
}
Var Context::execute(const Var& code) {
  if(code.is<Array<Var> >()) { // Function call
    const Array<Var>& array(code.as<Array<Var> >()); // Get reference of array value
    const Var& handle(execute(array[0])); // Execute first child of array to get function handle
    if(handle.is<Native>())
      return handle.as<Native>()(*this,array);
    else if(handle.is<Function>() || handle.is<Array<Var> >()) {
      Var wtr;
      _frames.push(nextFrame()); // Add new frame
      for(int i(1); i<array.size(); i++) { // For all parameters
        Symbol sym = {0};
        if(handle.is<Array<Var> >() && handle.as<Array<Var> >()[0].as<Array<Var> >().size()>=i) {
          Var symVar(execute(handle.as<Array<Var> >()[0].as<Array<Var> >()[i-1]));
          if(symVar.is<Symbol>())
            sym = symVar.as<Symbol>();
        }
        pushVariable(sym,execute(array[i])); // Compute parameter values
      }
      //out << _stack << '\n';
      if(handle.is<Array<Var> >())
        wtr = execute(handle.as<Array<Var> >()[1]);
      else if(handle.is<Function>()) // It's a function pointer
        wtr = handle.as<Function>()(*this,array.size()-1); // Call function
      _frames.pop(); // Remove current frame
      _stack.size(nextFrame()); // Remove elements from previous frame
      return wtr;
    }
  } else if(code.is<Symbol>()) return variable(code.as<Symbol>()); // Evaluate symbol
  else if(code.is<Quote>()) return code.as<Quote>().var; // Return raw data
  return code;
}
