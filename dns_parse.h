/*dns_parse.h - Parsing de respostas dns*/

#ifndef DNS_PARSE_H
#define DNS_PARSE_H

#include <stddef.h>
#include "dns_types.h"

/* Decodifica um nome (com compressão) a partir de ptr; consumed recebe os bytes ocupados em ptr. */
int parse_name(const unsigned char *buffer, const unsigned char *ptr,
               char *out, int *consumed);

/* Extrai o primeiro registro MX da resposta e classifica o resultado. */
resultado_t parse_response(const unsigned char *buffer, int recv_len,
                           char *exchange, size_t exchange_size);

#endif /* DNS_PARSE_H */
