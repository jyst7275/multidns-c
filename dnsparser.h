//
// Created by maxxie on 16-3-7.
//

#ifndef MULTIDNS_C_DNSPARSER_H
#define MULTIDNS_C_DNSPARSER_H

#include <string.h>
#include <list>
#include <stdio.h>

#define RESPONSE_TYPE_A 1
#define RESPONSE_TYPE_NS 2
#define RESPONSE_TYPE_CNAME 5

class DnsParser{
public:
    DnsParser(char* buf, int size):buff(buf),buf_len(size){
        status = STATUS_REQUEST;
        request_domain = NULL;
        request_type = 0;
    }
    DnsParser(){}
    int parse_request(){
        request_domain = new char[64];
        request_end = parse_domain(buff,request_domain,12);
        request_type = buff[request_end + 1];
        request_end += 4;
    }
    //refresh(header and question)
    int refresh_request(char* buf){
        buff = buf;
        parse_request();
    }
    int parse_response(char* buf, int size){
        buff = buf;
        buf_len = size;
        if(request_end == 0)
            parse_request();
        int i = request_end;
// check dns result
//        for(int t = 0;t < buf_len;t++)
//            printf("%d %d\n", t, (unsigned char)buff[t]);

        while(i < buf_len) {
            if (buff[i] == -64)
                i += 2;
            else if(buff[i] > 0 && buff[i] < 64) {
                int new_i = parse_domain(buff, NULL, i, false);
                if(new_i != -1)
                    i = new_i;
            }
            else
                return -1;
            int type = buff[i + 1];
            i += 8;
            int data_length = buff[i + 1];
            i += 2;
            if(type == RESPONSE_TYPE_A)
                response_ip.push_back(parse_ip(i));
            else{
                char* tem = new char[64];
                memset(tem, 0, sizeof(char)*64);
                parse_domain(buff, tem, i, true, data_length);
                response_ip.push_back(tem);
            }
            i += data_length;
        }
        status = STATUS_RESPONSE;
    }
    char* get_domain(){
        if(request_domain == NULL)
            parse_request();
        return request_domain;
    }
    char get_type(){
        if(request_type == 0)
            parse_request();
        return request_type;
    }
    std::list<char*> *get_response_ip(){
        if(status == STATUS_RESPONSE)
            return &response_ip;
        return NULL;
    }

private:
    const static int STATUS_REQUEST = 0;
    const static int STATUS_RESPONSE = 1;

    int status;
    char* buff;
    char* request_domain;
    char request_type;
    int buf_len;
    std::list<char*> response_ip;
    int request_end = 0;
    int parse_domain(char* src, char* dst, int start, bool copy = true, int length = 10000){
        int j = 0;
        int i = start;
        if(src[i] <= 0)
            return -1;
        while(i - start < length){
            if(src[i] > 0 && src[i] < 64) {
                if (copy) {
                    memcpy(dst + j, src + i + 1, sizeof(char) * src[i]);
                    dst[j + src[i]] = '.';
                }
                j += src[i] + 1;
                i += src[i] + 1;
                if(src[i] == 0)
                    dst[j - 1] = 0;
            }
            else if(src[i] == -64){
                //always copy
                memcpy(dst + j, src + src[i+1] + 1, sizeof(char) * src[src[i+1]]);
                i += 2;
                j += src[src[i+1]];
            }
            else
                return -1;
            if (src[i] == 0)
                break;
        }
        return i + 1;
    }
    char* parse_ip(int start){
        char* ip_str = new char[16];
        memset(ip_str, 0, sizeof(char)*16);
        for(int i = start, j = 0;i < start + 3;i++){
            char* tem = new char[4];
            sprintf(tem, "%d.", (unsigned char)buff[i]);
            strcat(ip_str, tem);
            delete tem;
        }
        char* last = new char[4];
        sprintf(last, "%d", (unsigned char)buff[start + 3]);
        strcat(ip_str, last);
        delete last;
        return ip_str;
    }
/*
 * rfc1035
 * format

0            15 16            31
+------------------------------+
|     id       |      flag     |
+------------------------------+
|  question_n  |  resource_n   |
+------------------------------+
| authority_n  |    extra_n    |
+------------------------------+
|          Question            |
+------------------------------+
|            Answer            |
+------------------------------+
|          Authority           |
+------------------------------+
|            Extra             |
+------------------------------+

 * Flag Format
 1       3      1  1  1  1     3          4
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|QR|   Opcode  |AA|TC|RD|RA| (ZERO) |   RCODE   |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

 * QR=1:Rsponse, 0:Request
 * Opcode=0:Standard, 1:Reverse, 2:Status Request
 * ....

// Only Question and Header are in request record

*/

/*
 * Question Format

0            15 16            31
+------------------------------+
|         Question Name        |
+------------------------------+
|     Type     |     Class     |
+------------------------------+

 * Length of Question Name is not fixed, it start with a count(0~63) and end with a zero
 * example: www.baidu.com
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |3|w|w|w|5|b|a|i|d|u|3|c|o|m|0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 * Type List
 *   1:A
 *   2:NS
 *   5:CNAME
 *   6:SOA
 *   ......

 * Class is usually 1 -- Internet Data
*/

/*
 * Answer Record
0            15 16            31
+------------------------------+
|            Domain            |
+------------------------------+
|     Type     |     Class     |
+------------------------------+
|            Time              |
+------------------------------+
| Data  Length |               |
|            Data              |
+------------------------------+

 * Domain is of 2 bits or more
 * Domain may be replace with pointer, which is usually C00C, else it it the same as Question Name

 * Data Length is length of Data, for example
 * www.a.shifen.com:16
 * 10.0.0.1:4

*/

};

#endif //MULTIDNS_C_DNSPARSER_H
