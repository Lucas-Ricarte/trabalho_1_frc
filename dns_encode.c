/* dns_encode.c - Montagem da consulta DNS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "dns_encode.h"

/* Gera o QNAME: cada rótulo precedido do seu tamanho, terminado em 0x00. */
int encode_qname(const char *domain, unsigned char *buffer) {
    int pos = 0;
    const char *start = domain;
    const char *dot;

    while (1) {
        dot = strchr(start, '.');
        int len = dot ? (int)(dot - start) : (int)strlen(start);

        if (len <= 0 || len > 63) {
            fprintf(stderr, "Erro: rotulo invalido no dominio '%s'\n", domain);
            exit(EXIT_FAILURE);
        }

        buffer[pos++] = (unsigned char) len;
        memcpy(buffer + pos, start, len);
        pos += len;

        if (!dot) break;
        start = dot + 1;
    }
    buffer[pos++] = 0x00;
    return pos;
}

/* Monta header + question (MX, IN) e devolve o tamanho do pacote. */
int build_query(const char *domain, unsigned char *buffer, uint16_t *out_id) {
    dns_header_t *hdr = (dns_header_t *) buffer;

    uint16_t id = (uint16_t) (rand() & 0xFFFF);

    hdr->id      = htons(id);
    hdr->flags   = htons(0x0100); /* consulta recursiva */
    hdr->qdcount = htons(0x0001); /* 1 pergunta */
    hdr->ancount = 0x0000;
    hdr->nscount = 0x0000;
    hdr->arcount = 0x0000;

    *out_id = id;

    int offset = sizeof(dns_header_t);
    offset += encode_qname(domain, buffer + offset);

    uint16_t qtype  = htons(QTYPE_MX);
    uint16_t qclass = htons(QCLASS_IN);
    memcpy(buffer + offset, &qtype, 2);  offset += 2;
    memcpy(buffer + offset, &qclass, 2); offset += 2;

    return offset;
}
