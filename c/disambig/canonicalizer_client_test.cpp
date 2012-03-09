#include "canonicalizer_client.h"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
  CanonicalizerClient* cc = CanonicalizerClient::instance();
  cc->base_url("http://localhost:5000");
  vector<string> candidates;
  /*
  candidates.push_back("China");
  candidates.push_back("Beijing");
  candidates.push_back("Hu Jintao");
  */

  candidates.push_back("The Philippines");
  candidates.push_back("Filipino");//|Pilipinas|Southeast Asia|Pacific Ocean|Luzon Strait|South China Sea|Vietnam|Sulu Sea|Celebes Sea|Indonesia|Philippine Sea|Pacific Ring of Fire|Visayas|Negritos|Austronesian|Malay|Hindu|Islamic|Chinese|Ferdinand Magellan|Spanish|Manila–Acapulco|Christianity|Philippine Revolution|Spanish-American War|Philippine-American War|United States|Japanese|World War II
  vector<CanonicalMeta> result;
  cc->canonicals(candidates,result);

  printf("%d canonicals found\n",result.size());

  for(vector<CanonicalMeta>::const_iterator ii = result.begin(); ii != result.end(); ii++) {
    CanonicalMeta meta = *ii;
    cout << "Result:" << meta.canonical << ":" << meta.text_rep << ":" << meta.score << endl;
  }
}
