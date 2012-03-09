#ifndef __THOMAS__WANG_HASH__
#define __THOMAS__WANG_HASH__

class ThomasWangHash {
  public:
    size_t operator()(unsigned int a) const
    {
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }
};

class ThomasWangHashPtr {
  public:
    size_t operator()(void* ptr) const
    {
      intptr_t a = (intptr_t) ptr;
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }
};

#endif
