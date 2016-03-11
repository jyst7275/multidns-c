//
// Created by maxxie on 16-3-8.
//

#ifndef MULTIDNS_C_DNSDISPATCHER_H
#define MULTIDNS_C_DNSDISPATCHER_H

#include <netinet/in.h>
#include "sys/socket.h"
#include <fstream>
#include <iostream>
#include "list"
#include "json/json.h"
#include <boost/regex.hpp>
#include <arpa/inet.h>
#include <sys/time.h>
#include "unistd.h"
#include "time.h"
#include "map"
#include "string"
#include "metadata.h"
#include "Logger.h"


class Cache{
public:
    buff* buf;
    long int timestamp;
    Cache(){}
    Cache(buff* bu){
        buf = new buff;
        memcpy(buf->buf, bu->buf, bu->len * sizeof(char));
        buf->len = bu->len;
        struct timeval time_val;
        struct timezone zone_val;
        gettimeofday(&time_val,&zone_val);
        timestamp = time_val.tv_sec;
    }
    bool isOutDated(long time_se){
        struct timeval timeval1;
        struct timezone timezone1;
        gettimeofday(&timeval1, &timezone1);
        return (timeval1.tv_sec - timestamp) > time_se;
    }
};

class DnsDispatcher{
private:
    static sockaddr_in defa;
    static std::list<std::pair<boost::regex, DnsDispatcher> > dispatcher;
    static std::map<std::string, Cache> cache;
    static long av_time;
    static sockaddr_in bind;
    bool redirect;
    std::string bash;
    sockaddr_in address;
    std::string hos;
    bool use_cache;
public:

    DnsDispatcher(bool re, std::string s, sockaddr_in addr):redirect(re),bash(s),address(addr){}
    ~DnsDispatcher(){}
    static int init(char* config){

        Json::Reader reader;
        std::ifstream is;
        std::string ex = "|";
        is.open(config);
        Json::Value root;
        if(!reader.parse(is, root, false)) {
            Logger::getLogger()->logInfo("Json Parse Error, Please Check!");
            return -1;
        }
        bzero(&defa, sizeof(defa));
        defa.sin_family = AF_INET;
        defa.sin_port = htons(53);
        const char* defa_str = root["default"].asCString();
        if(inet_aton(defa_str, &defa.sin_addr) == 0){
            Logger::getLogger()->logInfo("Default Server Address Error, Please Check!");
            return -1;
        }
        av_time = root["cache_t"].asInt();

        bzero(&bind,sizeof(bind));
        bind.sin_family = AF_INET;

        inet_aton(root["bind"].asCString(), &bind.sin_addr);
//        bind.sin_addr.s_addr = htonl(INADDR_ANY);
        bind.sin_port = htons(root["port"].asInt());
        char outbuf[40];
        sprintf(outbuf, "Starting Server at %s:%d", root["bind"].asCString(), root["port"].asInt());
        Logger::getLogger()->logInfo(outbuf);


        for (Json::Value v: root["sections"]){
            std::string tem = "";
            bool redirect = v["redirect"].asBool();
            std::string bash;
            const char *server_str = v["server"].asCString();
            sockaddr_in serverAddr;
            bzero(&serverAddr,sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(53);
            if(inet_aton(server_str, &serverAddr.sin_addr) == 0){
                Logger::getLogger()->logInfo("Server Address Error, Please Check!");
                return -1;
            };
            if(redirect)
                bash = v["bash"].asString();
            else
                bash = "";
            for(Json::Value dom: v["domains"]){
                tem += dom.asString();
                tem += "|";
            }
//            printf("%s\n", tem.substr(0, tem.length() - 1).c_str());
            boost::regex ex(tem.c_str());
            std::pair<boost::regex, DnsDispatcher> infopair(ex,DnsDispatcher(redirect,bash,serverAddr));
            dispatcher.push_back(infopair);
        }
        return 0;
    }
    static DnsDispatcher get_dispatcher(char* host){
        for(std::pair<boost::regex, DnsDispatcher> p : dispatcher){
            if(boost::regex_match(host, p.first)) {
                DnsDispatcher pat = p.second;
                pat.setHost(host);
                return pat;
            }
        }
        DnsDispatcher pat = DnsDispatcher(false, "", defa);
        pat.setHost(host);
        return pat;
    }
    static sockaddr_in get_bind(){return bind;}

    bool use_cahce(){return use_cache;}
    void addCache(char type, buff* buf){
        if(!use_cache)
            cache[hos + "," + type] = Cache(buf);
    }
    void setHost(char* host){
        this->hos = host;
    }
    sockaddr_in getAddress(){
        return address;
    }
    char* getAddressStr(){
        char* str = new char[20];
        strcpy(str, inet_ntoa(address.sin_addr));
        return str;
    }
    int addRoute(std::list<char*> *l){
        if(!redirect || use_cache)
            return 1;
        char* exe = new char[100];
        for(char* c : *l){
            sprintf(exe, bash.c_str(), c);
            system(exe);
            memset(exe, 0,sizeof(char)*100);
        }
    }
    void dispatch(buff *in, buff *out, char type){
        use_cache = false;
        std::string search = hos + "," + type;
        if(cache.find(search)==cache.end()){
            sockaddr_in serverAddr = getAddress();
            int requestSocket = socket(AF_INET,SOCK_DGRAM,0);
            sendto(requestSocket,in->buf,(size_t)in->len,0,(sockaddr *)&serverAddr,sizeof(serverAddr));
            out->len = (int)recvfrom(requestSocket,out->buf,MAXLINE,0,NULL,NULL);
            return;
        }
        Cache ca = cache[search];
        if(ca.isOutDated(av_time)){
            sockaddr_in serverAddr = getAddress();
            int requestSocket = socket(AF_INET,SOCK_DGRAM,0);
            sendto(requestSocket,in->buf,(size_t)in->len,0,(sockaddr *)&serverAddr,sizeof(serverAddr));
            out->len = (int)recvfrom(requestSocket,out->buf,MAXLINE,0,NULL,NULL);
            return;
        }
        ca.buf->duplicate_id(in);
        out->buf = ca.buf->buf;
        out->len = ca.buf->len;
        use_cache = true;
    }
};

sockaddr_in DnsDispatcher::defa;
std::list<std::pair<boost::regex, DnsDispatcher> > DnsDispatcher::dispatcher;
std::map<std::string, Cache> DnsDispatcher::cache;
long DnsDispatcher::av_time = 0;
sockaddr_in DnsDispatcher::bind;
#endif //MULTIDNS_C_DNSDISPATCHER_H
