/* Relationship Client Implementation */

#include "relationship_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>

/* For inet_ntoa. */
#include <arpa/inet.h>

/* Required by event.h. */
#include <sys/time.h>

#include <iostream>

#include <google/dense_hash_set>
#include "equality.h"
#include "thomas_wang_hash.h"

using namespace std;
using google::dense_hash_set;

typedef dense_hash_set<uint32_t,ThomasWangHash, eqint> dup_set_t;

int 
RelationshipClient::net_connect()
{
  int soc;
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 

  // Create socket 
  soc = socket(AF_INET, SOCK_STREAM, 0); 
  if (soc < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(port_); 
  addr.sin_addr.s_addr = inet_addr(host_.c_str()); 

  // Set non-blocking 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // Trying to connect with timeout 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        do { 
           tv.tv_sec = 1; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(soc, &myset); 
           res = select(soc+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              exit(0); 
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg &= (~O_NONBLOCK); 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  return soc;
}

static bool buffered_read( int fd, char * buf, int len)
{
	int res, err;
	for ( ;; )
	{
		res = recv ( fd, buf, len, 0 );

		if ( res<0 )
		{
			err = errno;
			if ( err==EINTR || err==EWOULDBLOCK ) // FIXME! remove non-blocking mode here; add timeout
				continue;
      cout << "Error " << strerror(errno) << endl;
			return false;
		}

		len -= res;
		buf += res;

		if ( len==0 )
			return true;

		if ( res==0 )
		{
      cout << "Incomplete Read" << endl;
      return false;
		}
	}
}

static bool buffered_write(int fd, char* buf, int len)
{
	int res, err;
	for ( ;; )
	{
		res = send ( fd, buf, len, 0 );

		if ( res<0 )
		{
			err = errno;
      /*
			if ( err==EINTR || err==EWOULDBLOCK ) // FIXME! remove non-blocking mode here; add timeout
				continue;
      */
      cout << "Error " << strerror(errno) << endl;
			return false;
		}

		len -= res;
		buf += res;

		if ( len==0 )
			return true;

		if ( res==0 )
		{
      cout << "Incomplete Write" << endl;
      return false;
		}
	}

}

struct int_array*
RelationshipClient::sites_close_to(u_int32_t site_id)
{
  struct int_array* results;
  bool rc;
  int fd = net_connect();
  char header[8];

  char* send_message = (char*) malloc(12);
  *(int*)(send_message) = htonl(4);
  *(int*)(send_message + 4) = htonl(2);
  *(int*)(send_message + 8) = htonl(site_id);

  rc = buffered_write(fd,send_message,12);
  if (!rc) {
    cout << "Send Error " << strerror(errno) << endl;
  }

  free(send_message);

  results = (struct int_array*) calloc(sizeof(struct int_array),1);

  rc = buffered_read(fd,header,8);
  if (!rc) {
    return results;
  }


  u_int32_t payload_size = ntohl(*((int*) (header)));
  if (!payload_size) {
    return results;
  }

  char* recv_payload = (char*) malloc(payload_size);
  rc = buffered_read( fd, recv_payload, payload_size);
  if (!rc) {
    free(recv_payload);
    return results;
  }

  results->array = (u_int32_t*) malloc(payload_size);
  results->length = payload_size / sizeof(u_int32_t);
  for(size_t i=0; i < payload_size / 4; i++) {
    results->array[i] = ntohl(*(((int*) recv_payload) + i));
  }

  free(recv_payload);
  return results;
}

vector<Relationship> 
RelationshipClient::distances(const vector<int>& ids)
{
  vector<Relationship> rels;
  bool rc;
  int fd = net_connect();
  char header[8];

  dup_set_t dups(ids.size());
  dups.set_empty_key(0);
  for(size_t i=0; i < ids.size(); i++) {
    dups.insert(ids[i]);
  }
  uint32_t send_payload_size = dups.size() * 4;
  char* send_message = (char*) malloc(send_payload_size + 8);
  *(int*)(send_message) = htonl(send_payload_size);
  *(int*)(send_message + 4) = htonl(1);
  char* send_payload = send_message + 8;
  int counter = 0;
  for(dup_set_t::const_iterator ii = dups.begin(); ii != dups.end(); ii++) {
    *(int*)(send_payload + (counter * 4)) = htonl(*ii);
  }

  rc = buffered_write(fd,send_message,send_payload_size + 8);
  if (!rc) {
    cout << "Send Error " << strerror(errno) << endl;
  }
  free(send_message);

  rc = buffered_read(fd,header,8);
  if (!rc) {
    return rels;
  }

  u_int32_t payload_size = ntohl(*((int*) (header)));
  cout << "Received Payload:" << payload_size << endl;
  if (payload_size > 0) {
    char* recv_payload = (char *) malloc(payload_size);
    rc = buffered_read( fd, recv_payload, payload_size);
    int* recv_payload_ints = (int*) recv_payload;
    if (!rc) {
      free(recv_payload);
      return rels;
    }

    for(size_t i = 0; i < payload_size / 4; i += 3) {
      Relationship rel;
      rel.site_id =  ntohl(*(recv_payload_ints + i));
      rel.other_site_id = ntohl(*(recv_payload_ints + (i + 1)));
      rel.distance = ntohl(*(recv_payload_ints + (i + 2)));
      rels.push_back(rel);
    }
  }
  return rels;
}

vector<Relationship> 
RelationshipClient::distances(const int_array_t* ids)
{
  vector<Relationship> rels;
  bool rc;
  char header[8];

  if (ids->length == 0 || ids->length == 1)
    return rels;

  int fd = net_connect();

  dup_set_t dups(ids->length);
  dups.set_empty_key(0);
  for(size_t i=0; i < ids->length; i++) {
    dups.insert(ids->array[i]);
  }
  uint32_t send_payload_size = dups.size() * 4;
  char* send_message = (char*) malloc(send_payload_size + 8);
  *(int*)(send_message) = htonl(send_payload_size);
  *(int*)(send_message + 4) = htonl(1);
  char* send_payload = send_message + 8;
  int counter = 0;
  for(dup_set_t::const_iterator ii = dups.begin(); ii != dups.end(); ii++) {
    *(int*)(send_payload + (counter * 4)) = htonl(*ii);
    counter++;
  }

  rc = buffered_write(fd,send_message,send_payload_size + 8);
  if (!rc) {
    cout << "Send Error " << strerror(errno) << endl;
    free(send_message);
    return rels;
  }
  free(send_message);

  rc = buffered_read(fd,header,8);
  if (!rc) {
    return rels;
  }

  u_int32_t payload_size = ntohl(*((int*) (header)));
  if (payload_size > 0) {
    char* recv_payload = (char *) malloc(payload_size);
    rc = buffered_read( fd, recv_payload, payload_size);
    int* recv_payload_ints = (int*) recv_payload;
    if (!rc) {
      free(recv_payload);
      return rels;
    }

    for(size_t i = 0; i < payload_size / 4; i += 3) {
      Relationship rel;
      rel.site_id =  ntohl(*(recv_payload_ints + i));
      rel.other_site_id = ntohl(*(recv_payload_ints + (i + 1)));
      rel.distance = ntohl(*(recv_payload_ints + (i + 2)));
      rels.push_back(rel);
    }
    free(recv_payload);
  }
  close(fd);
  return rels;
}
