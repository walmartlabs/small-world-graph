#ifndef __CANONICALIZER_CLIENT_H__
#define __CANONICALIZER_CLIENT_H__

#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exception>

using namespace std;

class CanonicalizerClientException : exception {
  public:
    CanonicalizerClientException(const char* what) {
      what_ = what;
    }

    const char* what() {
      return what_;
    }
  protected:
    const char* what_;
};

class CanonicalMeta {
  public:
    string canonical;
    string text_rep;
    int score;
};

class CanonicalizerClient {
  public:
    static CanonicalizerClient* instance();
    void base_url(const string& url);
    void canonicals(const vector<string>& candidates, vector<CanonicalMeta>& canonicals) throw(CanonicalizerClientException);

    virtual ~CanonicalizerClient() {};

  protected:
    static CanonicalizerClient* instance_;
    string base_url_;
    CanonicalizerClient();
};


#endif
