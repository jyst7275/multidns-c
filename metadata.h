//
// Created by maxxie on 16-3-10.
//

#ifndef MULTIDNS_C_METADATA_H
#define MULTIDNS_C_METADATA_H

#include <netinet/in.h>
#define MAXLINE 200
class buff{
public:
	char *buf;
	int len;
	sockaddr_in *ipaddr;
	buff(){
		buf = new char[MAXLINE];
		ipaddr = new sockaddr_in();
		bzero(buf, sizeof(char)*MAXLINE);
		bzero(ipaddr, sizeof(*ipaddr));
	}
	void duplicate_id(buff* to){
		buf[0] = to->buf[0];
		buf[1] = to->buf[1];
	}
};
#endif //MULTIDNS_C_METADATA_H
