#include "Type.h"

#include "../macros.h"
#include "../text/String.h"
#include "../math/Vector.h"
#include "../text/Symbol.h"
#include "../time/Time.h"

using namespace L;

Table<Symbol, const TypeDescription*> L::types;

Cast TypeDescription::cast(const TypeDescription* target) const {
  auto found = casts.find((intptr_t)target);
  return found ? *found : nullptr;
}

void L::TypeInit() {
  // Operators
  Type<uint8_t>::canall<>();
  Type<int8_t>::canall<>();
  Type<uint16_t>::canall<>();
  Type<int16_t>::canall<>();
  Type<uint32_t>::canall<>();
  Type<int32_t>::canall<>();
  Type<uint64_t>::canall<>();
  Type<int64_t>::canall<>();
  Type<float>::canmath<>();
  Type<float>::cancmp<>();
  Type<double>::canmath<>();
  Type<double>::cancmp<>();
  Type<long double>::canmath<>();
  Type<long double>::cancmp<>();
  Type<String>::canadd<>();
  Type<String>::cancmp<>();
  Type<Symbol>::cancmp<>();
  Type<Time>::canmath<>();
  Type<Time>::cancmp<>();
  Type<Vector3f>::canmath();

  // Casts
  Type<int>::addcast<bool>([](void* dst, const void* src) {new(dst)bool((*(int*)src)!=0); });
  Type<int>::addcast<float>();
  Type<int>::addcast<String>([](void* dst, const void* src) {new(dst)String(ntos(*(int*)src)); });
  Type<unsigned int>::addcast<float>();
  Type<float>::addcast<String>([](void* dst, const void* src) {new(dst)String(ntos(*(float*)src)); });
  Type<String>::addcast<bool>([](void* dst, const void* src) {new(dst)bool(!((String*)src)->empty()); });
}
