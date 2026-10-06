/*dns_parse.c - Parsing de respostas dns*/

#include <string.h>
#include <arpa/inet.h>

#include "dns_parse.h"

/* Decodifica um nome (com compressão) a partir de ptr; consumed recebe os bytes ocupados em ptr. */
int parse_name(const unsigned char *buffer, const unsigned char *ptr,
               char *out, int *consumed) {
    int pos = 0;
    int jumped = 0;
    int total_consumed = 0;
    int first = 1;
    int safety = 0; /* evita loop infinito em pacote malformado */
    const unsigned char *p = ptr;

    while (*p != 0x00 && safety < 128) {
        safety++;

        if ((p[0] & 0xC0) == 0xC0) {
            /* Ponteiro de compressão (bits 11) */
            if (!jumped) total_consumed += 2;
            int offset = ((p[0] & 0x3F) << 8) | p[1];
            p = buffer + offset;
            jumped = 1;
            continue;
        } else {
            int len = p[0];
            p++;
            if (!jumped) total_consumed += (len + 1);

            if (!first) out[pos++] = '.';
            first = 0;

            memcpy(out + pos, p, len);
            pos += len;
            p += len;
        }
    }

    if (!jumped) total_consumed += 1;

    out[pos] = '\0';
    *consumed = total_consumed;
    return pos;
}

/* Extrai o primeiro registro MX da resposta e classifica o resultado. */
resultado_t parse_response(const unsigned char *buffer, int recv_len,
                           char *exchange, size_t exchange_size) {
    if (recv_len < (int) sizeof(dns_header_t)) {
        return RES_ERRO_SERVIDOR;
    }

    const dns_header_t *hdr = (const dns_header_t *) buffer;
    uint16_t flags   = ntohs(hdr->flags);
    uint16_t qdcount = ntohs(hdr->qdcount);
    uint16_t ancount = ntohs(hdr->ancount);
    int rcode = flags & 0x000F;

    if (rcode == 3) {
        return RES_NXDOMAIN;
    }
    if (rcode != 0) {
        return RES_ERRO_SERVIDOR;
    }
    if (ancount == 0) {
        return RES_SEM_MX;
    }

    const unsigned char *cursor = buffer + sizeof(dns_header_t);
    char tmp_name[MAX_NAME_LEN];
    int consumed;

    /* Pula a seção Question, ecoada na resposta */
    for (int i = 0; i < qdcount; i++) {
        parse_name(buffer, cursor, tmp_name, &consumed);
        cursor += consumed;
        cursor += 4; /* QTYPE + QCLASS */
    }

    /* Procura o primeiro registro MX na seção Answer */
    for (int i = 0; i < ancount; i++) {
        parse_name(buffer, cursor, tmp_name, &consumed);
        cursor += consumed;

        uint16_t type, class_;
        uint32_t ttl;
        uint16_t rdlength;

        memcpy(&type, cursor, 2);      cursor += 2;
        memcpy(&class_, cursor, 2);    cursor += 2;
        memcpy(&ttl, cursor, 4);       cursor += 4;
        memcpy(&rdlength, cursor, 2);  cursor += 2;

        type = ntohs(type);
        rdlength = ntohs(rdlength);
        (void) class_;
        (void) ttl;

        if (type == QTYPE_MX) {
            /* RDATA do MX: preferência (2 bytes) + nome do servidor */
            const unsigned char *rdata = cursor;
            int name_consumed;
            parse_name(buffer, rdata + 2, tmp_name, &name_consumed);
            strncpy(exchange, tmp_name, exchange_size - 1);
            exchange[exchange_size - 1] = '\0';
            return RES_OK;
        }

        cursor += rdlength;
    }

    return RES_SEM_MX;
}
