#include "sys/socket.h"
#include "netinet/in.h"
#include "pthread.h"
#include "vector"
#include "list"
#include "arpa/inet.h"
#include "sys/sem.h"
#include "dnsparser.h"
#include "DnsDispatcher.h"
#include "metadata.h"
using std::vector;
pthread_t thread_pid;


struct pro_arg{
    int sockfd;
    buff *buf;
};
void* process(void *arg){
    int sockfd = ((pro_arg*)arg)->sockfd;
    buff *buf = ((pro_arg*)arg)->buf;
	buff temBuf;

	DnsParser dnsParser = DnsParser(buf->buf, buf->len);

	DnsDispatcher thisDispatcher = DnsDispatcher::get_dispatcher(dnsParser.get_domain());
	Logger::getLogger()->logRequest(buf->ipaddr, dnsParser.get_domain(), thisDispatcher.getAddressStr());

/*    sockaddr_in serverAddr = thisDispatcher.getAddress();
    buff tem ;
    int requestSocket = socket(AF_INET,SOCK_DGRAM,0);
	sendto(requestSocket,buf->buf,buf->len,0,(sockaddr *)&serverAddr,sizeof(serverAddr));
	temBuf.len = recvfrom(requestSocket,temBuf.buf,MAXLINE,0,NULL,NULL);
    printf("%d\n",temBuf.len);*/
    thisDispatcher.dispatch(buf, &temBuf, dnsParser.get_type());
	dnsParser.parse_response(temBuf.buf, temBuf.len);
    thisDispatcher.addRoute(dnsParser.get_response_ip());
    thisDispatcher.addCache(dnsParser.get_type(), &temBuf);
	Logger::getLogger()->logResponse(dnsParser.get_domain(),dnsParser.get_response_ip(), thisDispatcher.getAddressStr(), thisDispatcher.use_cahce());

	sendto(sockfd,temBuf.buf,temBuf.len,0,(sockaddr *)buf->ipaddr,sizeof(*(buf->ipaddr)));

}

class BindServer {
private:
	int sockfd;
    sockaddr_in serverAddr;
public:
	BindServer(sockaddr_in in){
        serverAddr = in;
    };
	~BindServer() {};
	int start() {
		socklen_t calen = 16;
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(bind(sockfd,(sockaddr *)&serverAddr, sizeof(serverAddr))!=0) {
            Logger::getLogger()->logInfo("Bind Port Error, Exit Now");
            return -1;
        }
        Logger::getLogger()->logInfo("Service Started");
//		bzero(&calen, sizeof(calen));
		while(1) {
			buff *buf = new buff();
			buf->len = recvfrom(sockfd, buf->buf, MAXLINE, 0, (sockaddr *) buf->ipaddr, &calen);
			if(buf->len == -1) {
				char outbuf[100];
				sprintf(outbuf, "Recvfrom error, errno:%d", errno);
				Logger::getLogger()->logInfo(outbuf);
			}
			char bb[200];
			int lenl;
/*
			buff *out = new buff();
			sockaddr_in outAddr;
			outAddr.sin_family = AF_INET;
			outAddr.sin_port = htons(53);
			inet_aton("114.114.114.114", &outAddr.sin_addr);
			int requestSocket = socket(AF_INET, SOCK_DGRAM, 0);
			sendto(requestSocket,buf->buf,(size_t)buf->len,0,(sockaddr *)&outAddr,sizeof(outAddr));
			out->len = (int)recvfrom(requestSocket,out->buf,MAXLINE,0,NULL,NULL);
			sendto(sockfd,out->buf,out->len,0,(sockaddr *)buf->ipaddr,sizeof(*(buf->ipaddr)));
*/

            pro_arg arg;
            arg.buf = buf;
            arg.sockfd = sockfd;
			pthread_t pp;
//			process(&arg);
            int err = pthread_create(&pp, NULL, process, (void*)&arg);
            if(err != 0) {
                Logger::getLogger()->logInfo("Worker Start Fail");
                return -1;
            }

		}
	}

};
