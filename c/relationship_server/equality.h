#ifndef __EQUALITY__
#define __EQUALITY__

struct eqint {
  bool operator()(const int i1, const int i2) const
  {
    return i1 == i2;
  }
};

struct eqlong {
  bool operator()(const u_int64_t u1, const u_int64_t u2) const
  {
    return u1 == u2;
  }
};

struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};

#endif
