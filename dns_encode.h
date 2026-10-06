/*
 * dns_encode.h - Codificacao de nomes e construcao de consultas DNS
 */

#ifndef DNS_ENCODE_H
#define DNS_ENCODE_H

#include "dns_types.h"

/* Codifica o domínio no formato QNAME; retorna os bytes escritos. */
int encode_qname(const char *domain, unsigned char *buffer);

/* Monta a consulta MX/IN; retorna o tamanho do pacote e o ID em out_id. */
int build_query(const char *domain, unsigned char *buffer, uint16_t *out_id);

#endif /* DNS_ENCODE_H */
