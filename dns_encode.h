/*
 * dns_encode.h - Codificacao de nomes e construcao de consultas DNS
 */

#ifndef DNS_ENCODE_H
#define DNS_ENCODE_H

#include "dns_types.h"

/* Codifica um nome de dominio no formato de "labels" do DNS.
 * Retorna a quantidade de bytes escritos em buffer.
 * Ex: "unb.br" -> 03 'u''n''b' 02 'b''r' 00 */
int encode_qname(const char *domain, unsigned char *buffer);

/* Monta o pacote de consulta DNS completo (header + question) para o
 * dominio informado, tipo MX, classe IN. Retorna o tamanho total do
 * pacote em bytes e devolve o Transaction ID gerado via out_id. */
int build_query(const char *domain, unsigned char *buffer, uint16_t *out_id);

#endif /* DNS_ENCODE_H */
