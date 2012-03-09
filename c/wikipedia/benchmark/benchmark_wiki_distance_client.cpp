#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "WikiDistanceCalculator.h"
#include "benchmark.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace boost;

int main(int argc, char** argv) {
  shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  WikiDistanceCalculatorClient client(protocol);

  bench_start("Wiki Client");

    try {
      for(int i = 0; i < 10000; i++) {
        transport->open();

        vector<WikiDistance> results;
        vector<string> tuples;
        tuples.push_back("Obama");
        tuples.push_back("General Motors");
        tuples.push_back("Bernanke");
        tuples.push_back("United States Treasury");
        tuples.push_back("Ford");
        client.distances(results,tuples);

        for(vector<WikiDistance>::const_iterator ii = results.begin(); ii != results.end(); ii++) {
          cout << (*ii).first << " :: " << (*ii).second << " :: " << (*ii).distance << endl;
        }
        transport->close();
      }

    } catch (TException &tx) {
      printf("ERROR: %s\n", tx.what());
    }

  bench_finish("Wiki Client");

}
