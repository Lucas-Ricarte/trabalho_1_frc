/*
 * dns_types.h - Tipos, constantes e estruturas comuns do cliente DNS
 * ------------------------------------------------------------------
 * Compartilhado por todos os modulos do cliente DNS (trabalho 01).
 */

#ifndef DNS_TYPES_H
#define DNS_TYPES_H

#include <stdint.h>

/* ---- Constantes -------------------------------------------------- */
#define DNS_PORT        53
#define MAX_PACKET      512     /* payload UDP padrao (sem EDNS) */
#define MAX_NAME_LEN    256
#define TIMEOUT_SEC     2
#define MAX_TENTATIVAS  3
#define QTYPE_MX        15
#define QCLASS_IN       1

/* ---- Cabecalho DNS (12 bytes fixos) ------------------------------ */
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

/* ---- Resultado da consulta --------------------------------------- */
typedef enum {
    RES_OK,
    RES_NXDOMAIN,       /* dominio nao existe (RCODE = 3) */
    RES_SEM_MX,         /* dominio existe mas nao ha registro MX */
    RES_ERRO_SERVIDOR   /* outro RCODE de erro */
} resultado_t;

#endif /* DNS_TYPES_H */
