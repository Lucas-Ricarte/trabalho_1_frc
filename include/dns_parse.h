#ifndef DNS_PARSE_H
#define DNS_PARSE_H

#include <stddef.h>

#include "dns_types.h"

resultado_t interpretar_resposta(const unsigned char *resposta, int tamanho,
                                 char *servidor_email, size_t capacidade);

#endif
