/*
 * dns_parse.h - Parsing de respostas DNS
 */

#ifndef DNS_PARSE_H
#define DNS_PARSE_H

#include <stddef.h>
#include "dns_types.h"

/* Decodifica um nome de dominio a partir da posicao "ptr" dentro do
 * pacote "buffer" (necessario ter o inicio do pacote para resolver
 * ponteiros de compressao - RFC 1035, secao 4.1.4).
 *
 * out        : buffer de saida onde o nome decodificado eh escrito
 * consumed   : quantidade de bytes consumidos a partir de ptr NA
 *              sequencia original (sem contar bytes apos um "salto"
 *              de ponteiro) - usado para avancar o cursor de leitura
 *              do pacote no chamador.
 * Retorna a quantidade de caracteres escritos em out. */
int parse_name(const unsigned char *buffer, const unsigned char *ptr,
               char *out, int *consumed);

/* Percorre a resposta DNS recebida, procurando o primeiro registro MX.
 * Preenche "exchange" com o nome do servidor de e-mail encontrado.    */
resultado_t parse_response(const unsigned char *buffer, int recv_len,
                           char *exchange, size_t exchange_size);

#endif /* DNS_PARSE_H */
