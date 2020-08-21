#pragma once

#include <new>
#include "../hash.h"
#include "../system/Memory.h"
#include "../stream/Stream.h"
#include "../stream/serial_bin.h"
#include "../stream/serial_text.h"

namespace L {
  template <class K, class V>
  class Table {
  public:
    class Slot {
    private:
      uint32_t _hash;
      K _key;
      V _value;
    public:
      inline Slot(uint32_t h, const K& key) : _hash(h), _key(key) {}
      inline bool empty() const { return !hash(); }
      inline uint32_t hash() const { return _hash; }
      inline const K& key() const { return _key; }
      inline const V& value() const { return _value; }
      inline V& value() { return _value; }
    };
  protected:
    Slot* _slots;
    size_t _size, _count;
    void grow() {
      if(_slots) {
        const size_t oldsize = _size;
        Slot* oldslots = _slots;
        _slots = Memory::alloc_type_zero<Slot>(_size *= 2);
        for(uintptr_t i(0); i < oldsize; i++) {
          if(!oldslots[i].empty()) {
            memcpy((void*)(find_slot_or_create(oldslots[i].hash())), oldslots + i, sizeof(Slot));
          }
        }
        Memory::free(oldslots, oldsize * sizeof(Slot));
      } else {
        _slots = Memory::alloc_type_zero<Slot>(_size = 4);
      }
    }
  public:
    class Iterator {
    private:
      Slot* _slot;
    public:
      inline Iterator(Slot* slot) : _slot(slot) {}
      inline Iterator& operator++() { if(_slot->hash()) _slot++; return *this; }
      inline bool operator!=(const Iterator& other) { while(_slot < other._slot && !_slot->hash()) _slot++; return _slot != other._slot; }
      inline Slot* operator->() const { return _slot; }
      inline Slot& operator*() const { return *(operator->()); }
    };
    constexpr Table() : _slots(nullptr), _size(0), _count(0) {}
    Table(const Table& other) : _slots(other._slots ? Memory::alloc_type_zero<Slot>(other._size) : nullptr), _size(other._size), _count(other._count) {
      for(uintptr_t i(0); i < _size; i++) {
        if(!other._slots[i].empty()) {
          new(_slots + i)Slot(other._slots[i]);
        }
      }
    }
    inline Table& operator=(const Table& other) {
      this->~Table();
      new(this)Table(other);
      return *this;
    }
    ~Table() {
      if(_slots) {
        clear();
        Memory::free(_slots, _size * sizeof(Slot));
      }
    }
    inline size_t size() const { return _size; }
    inline size_t count() const { return _count; }
    inline Iterator begin() const {
      if(_count) {
        for(uintptr_t i(0); i < _size; i++) {
          if(_slots[i].hash()) {
            return Iterator(_slots + i);
          }
        }
      }
      return Iterator(nullptr);
    }
    inline Iterator end() const { return (_count) ? _slots + _size : Iterator(nullptr); }
    V& operator[](const K& k) {
      const uint32_t h = hash(k);
      Slot* slot = find_slot_or_create(h);
      if(slot->empty()) {
        new(slot)Slot(h, k);
        _count++;
      }
      return slot->value();
    }
    inline uintptr_t index_for(uint32_t h) const { return uintptr_t(h * (float(_size) / UINT32_MAX)); }
    Slot* find_slot_or_create(uint32_t h) {
      if(_count * 10 >= _size * 8) {
        grow();
      }
      while(true) {
        const uintptr_t i = index_for(h);
        for(uintptr_t j = 0; j < _size; j++) {
          const uintptr_t k = (i + j) % _size;
          if(_slots[k].empty() || _slots[k].hash() == h) {
            return _slots + k;
          }
        }
        grow();
      }
    }
    Slot* find_slot(const K& key) const {
      const uint32_t h = hash(key);
      const uintptr_t i = index_for(h);
      for(uintptr_t j = 0; j < _size; j++) {
        const uintptr_t k = (i + j) % _size;
        if(_slots[k].hash() == h) {
          return _slots + k;
        } else if(_slots[k].empty()) {
          return nullptr;
        }
      }
      return nullptr;
    }
    V* find(const K& key) const {
      Slot* slot = find_slot(key);
      return slot && !slot->empty() ? &slot->value() : nullptr;
    }
    V get(const K& key, const V& default_value) const {
      if(const V* value = find(key)) {
        return *value;
      } else {
        return default_value;
      }
    }
    void remove(const K& key) {
      Slot* slot = find_slot(key);
      if(slot && !slot->empty()) {
        slot->~Slot();
        while(true) {
          const uintptr_t slot_index = slot - _slots;
          const uintptr_t next_slot_index = (slot_index + 1) % _size;
          Slot* next_slot = _slots + next_slot_index;
          if(!next_slot->empty() && index_for(next_slot->hash()) != next_slot_index) {
            memcpy((void*)slot, next_slot, sizeof(*slot));
            slot = next_slot;
          } else {
            break;
          }
        }
        memset((void*)slot, 0, sizeof(*slot));
        _count--;
      }
    }
    void clear() {
      if(_count) {
        for(uintptr_t i(0); i < _size; i++) {
          if(!_slots[i].empty()) {
            _slots[i].~Slot();
          }
        }
      }
      _count = 0;
    }

    friend Stream& operator<=(Stream& s, const Table& v) { s <= v.count(); for(const auto& e : v) s <= e.key() <= e.value(); return s; }
    friend Stream& operator>=(Stream& s, Table& t) { size_t size; s >= size; while(size--) { K k; V v; s >= k >= v; t[k] = v; } return s; }
  };
  template <class K, class V> inline Stream& operator<<(Stream& s, const Table<K, V>& v) {
    s << '{';
    bool first = true;
    for(auto&& e : v) {
      if(first) {
        first = false;
      } else {
        s << ',';
      }
      s << e.key() << ':' << e.value();
    }
    s << '}';
    return s;
  }
  template <class K, class V> Stream& operator<(Stream& s, const Table<K, V>& v) {
    s < v.count();
    for(const auto& e : v) {
      s < e.key() < e.value();
    }
    return s;
  }
  template <class K, class V> Stream& operator>(Stream& s, Table<K, V>& v) {
    size_t count;
    s > count;
    for(size_t i(0); i < count; i++) {
      K key;
      V value;
      s > key > value;
      v[key] = value;
    }
    return s;
  }
}
