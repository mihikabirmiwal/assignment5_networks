#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lib/tdns/tdns-c.h"

/* DNS header structure */
struct dnsheader {
        uint16_t        id;         /* query identification number */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                        /* fields in third byte */
        unsigned        qr: 1;          /* response flag */
        unsigned        opcode: 4;      /* purpose of message */
        unsigned        aa: 1;          /* authoritative answer */
        unsigned        tc: 1;          /* truncated message */
        unsigned        rd: 1;          /* recursion desired */
                        /* fields in fourth byte */
        unsigned        ra: 1;          /* recursion available */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        rcode :4;       /* response code */
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ 
                        /* fields in third byte */
        unsigned        rd :1;          /* recursion desired */
        unsigned        tc :1;          /* truncated message */
        unsigned        aa :1;          /* authoritative answer */
        unsigned        opcode :4;      /* purpose of message */
        unsigned        qr :1;          /* response flag */
                        /* fields in fourth byte */
        unsigned        rcode :4;       /* response code */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ra :1;          /* recursion available */
#endif
                        /* remaining bytes */
        uint16_t        qdcount;    /* number of question records */
        uint16_t        ancount;    /* number of answer records */
        uint16_t        nscount;    /* number of authority records */
        uint16_t        arcount;    /* number of resource records */
};

/* A few macros that might be useful */
/* Feel free to add macros you want */
#define DNS_PORT 53
#define BUFFER_SIZE 2048 

int main() {
    printf("[LOCAL] in main\n");
    /* A few variable declarations that might be useful */
    /* You can add anything you want */
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    /* PART2 TODO: Implement a local iterative DNS server */
    
    /* 1. Create an **UDP** socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    /* 2. Initialize server address (INADDR_ANY, DNS_PORT) */
    /* Then bind the socket to it */
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DNS_PORT);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    /* 3. Initialize a server context using TDNSInit() */
    /* This context will be used for future TDNS library function calls */
    struct TDNSServerContext* server_context = TDNSInit();
    /* 4. Create the edu zone using TDNSCreateZone() */
    TDNSCreateZone(server_context, "edu");
    /* Add the UT nameserver ns.utexas.edu using using TDNSAddRecord() */
    TDNSAddRecord(server_context, "edu", "utexas", NULL, "ns.utexas.edu");
    printf("[LOCAL] done with first addrecord\n");
    /* Add an IP address for ns.utexas.edu domain using TDNSAddRecord() */
    TDNSAddRecord(server_context, "utexas.edu", "ns", "40.0.0.20", NULL);
    printf("[LOCAL] before while loop\n");
    /* 5. Receive a message continuously and parse it using TDNSParseMsg() */
        while (1) {
        printf("[LOCAL] inside while loop\n");
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len); //receive message from server 
        buffer[n] = '\0';
        struct TDNSParseResult* parsed = malloc(sizeof(struct TDNSParseResult));
        // 0 if query, 1 is response
        if(!TDNSParseMsg(buffer, BUFFER_SIZE, parsed)) { 
                /* 6. If it is a query for A, AAAA, NS DNS record, find the queried record using TDNSFind() */
                /* You can ignore the other types of queries */
                if (parsed->qtype == A || parsed->qtype == AAAA || parsed->qtype == NS) {
                        struct TDNSFindResult* found = malloc(sizeof(struct TDNSFindResult));
                        /* a. If the record is found and the record indicates delegation, */
                        /* send an iterative query to the corresponding nameserver */
                        /* You should store a per-query context using putAddrQID() and putNSQID() */
                        /* for future response handling */
                        printf("[LOCAL] before find\n");
                        if (TDNSFind(server_context, parsed, found)){
                                printf("[LOCAL] after find\n");
                                printf("[LOCAL] parsed nsIP: %s\n", parsed->nsIP);
                                printf("[LOCAL] parsed nsDomain: %s\n", parsed->nsDomain);
                                if (parsed->nsIP != NULL){
                                        printf("[LOCAL] found and needs delegation \n");
                                        // printf("[LOCAL] parsed nsIP: %s\n", parsed->nsIP);
                                        // printf("[LOCAL] parsed nsDomain: %s\n", parsed->nsDomain);
                                        printf("[LOCAL] adding addr and ns per-query context \n");
                                        putAddrQID(server_context, parsed->dh->id, (struct sockaddr *)&client_addr);
                                        printf("[LOCAL] finished putAddrQID \n");
                                        putNSQID(server_context, parsed->dh->id, parsed->nsIP, parsed->nsDomain);
                                        printf("[LOCAL] finished putNSQID \n");
                                        // ssize_t serialized_query_size = TDNSGetIterQuery(parsed, found->serialized);
                                        // printf("[LOCAL] finished TDNSGetIterQuery \n");
                                        // TDNSFind(server_context, parsed, found)
                                        struct sockaddr_in dest_addr;
                                        bzero((char*)&dest_addr, sizeof(dest_addr));
                                        dest_addr.sin_family = AF_INET;
                                        dest_addr.sin_port = htons(DNS_PORT);
                                        inet_pton(AF_INET, parsed->nsIP, &dest_addr.sin_addr);
                                        sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)); 
                                        // sendto(sockfd, found->serialized, found->len, 0, (struct sockaddr*)&client_addr, client_len); 
                                } else {
                                        /* b. If the record is found and the record doesn't indicate delegation, */
                                        /* send a response back */
                                        printf("[LOCAL] found and doesn't need delegation, sending back to og client \n");
                                        // delAddrQID(server_context, parsed->dh->id);
                                        // delNSQID(server_context, parsed->dh->id);
                                        sendto(sockfd, found->serialized, found->len, 0, (struct sockaddr*)&client_addr, client_len);
                                } 

                        } else {
                                printf("[LOCAL] wasn't found and is just sending back\n");
                                /* c. If the record is not found, send a response back */
                                sendto(sockfd, found->serialized, found->len, 0, (struct sockaddr*)&client_addr, client_len);
                        }
                
                }
        } else {
                if(parsed->nsIP !=NULL && parsed->nsDomain!=NULL) {
                        printf("[LOCAL] reponse: non-authoritative (referral) \n");
                        printf("[LOCAL] parsed nsIP: %s\n", parsed->nsIP);
                        printf("[LOCAL] parsed nsDomain: %s\n", parsed->nsDomain);
                        /* 7-1. If the message is a non-authoritative response */
                        /* (i.e., it contains referral to another nameserver) */
                        /* send an iterative query to the corresponding nameserver */
                        /* You can extract the query from the response using TDNSGetIterQuery() */
                        /* You should update a per-query context using putNSQID() */
                        char* serialized = malloc(BUFFER_SIZE);
                        ssize_t serialized_query_size = TDNSGetIterQuery(parsed, serialized);
                        putNSQID(server_context, parsed->dh->id, parsed->nsIP, parsed->nsDomain);
                        struct sockaddr_in dest_addr;
                        bzero((char*)&dest_addr, sizeof(dest_addr));
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(DNS_PORT);
                        inet_pton(AF_INET, parsed->nsIP, &dest_addr.sin_addr);
                        sendto(sockfd, serialized, serialized_query_size, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                } else {
                        printf("[LOCAL] reponse: authoritative (final) \n");
                        /* 7. If the message is an authoritative response (i.e., it contains an answer), */
                        /* add the NS information to the response and send it to the original client */
                        /* You can retrieve the NS and client address information for the response using */
                        /* getNSbyQID() and getAddrbyQID() */
                        const char* newIP = malloc(BUFFER_SIZE);
                        const char* newDomain = malloc(BUFFER_SIZE);
                        getNSbyQID(server_context, parsed->dh->id, &newIP, &newDomain);
                        printf("[LOCAL] finished getNSbyQID \n");
                        getAddrbyQID(server_context, parsed->dh->id, (struct sockaddr *)&client_addr);
                        printf("[LOCAL] finished getAddrbyQID \n");
                        // char* newMessage = malloc(BUFFER_SIZE);
                        /* You can add the NS information to the response using TDNSPutNStoMessage() */
                        printf("[LOCAL] putting ns to message \n");
                        printf("[LOCAL] new nsIP: %s\n", newIP);
                        printf("[LOCAL] new nsDomain: %s\n", newDomain);
                        TDNSPutNStoMessage(buffer, BUFFER_SIZE, parsed, newIP, newDomain); 
                        /* Delete a per-query context using delAddrQID() and putNSQID() */
                        printf("[LOCAL] deleting per query context \n");
                        delAddrQID(server_context, parsed->dh->id);
                        delNSQID(server_context, parsed->dh->id);
                        sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, client_len);
                }
                
        }     
}

    return 0;
}

