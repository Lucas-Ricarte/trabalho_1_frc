/*dns_types.h - Tipos, constantes e estruturas comuns do cliente dns*/

#ifndef DNS_TYPES_H
#define DNS_TYPES_H

#include <stdint.h>

/*Constantes*/
#define DNS_PORT        53
#define MAX_PACKET      512     /* payload UDP padrao (sem edns) */
#define MAX_NAME_LEN    256
#define TIMEOUT_SEC     2
#define MAX_TENTATIVAS  3
#define QTYPE_MX        15
#define QCLASS_IN       1

/*Cabecalho dns - 12 bytes*/
#pragma pack(push, 1)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;
#pragma pack(pop)

/*Resultado da consulta*/
typedef enum {
    RES_OK,
    RES_NXDOMAIN,       /*dominio nao existe*/
    RES_SEM_MX,         /*dominio existe mas nao ha registro mx*/
    RES_ERRO_SERVIDOR   /*outro rcode de erro*/
} resultado_t;

#endif /*DNS_TYPES_H*/
