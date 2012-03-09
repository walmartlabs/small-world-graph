#ifndef __EQUALITY__
#define __EQUALITY__

#include <string>
#include <string.h>
#include <stdint.h>
using namespace std;

struct eqptr
{
  bool operator()(const void* s1, void* s2) const
  {
    return (s1 == s2);
  }
};

struct eqint {
  bool operator()(const int i1, const int i2) const
  {
    return i1 == i2;
  }
};

struct eqlong {
  bool operator()(const uint64_t u1, const uint64_t u2) const
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

struct eqcasestr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcasecmp(s1, s2) == 0);
  }
};

struct eqstring
{
  bool operator()(const string& s1, const string& s2) const
  {
    return (s1 == s2);
  }
};

#endif
