#ifndef DNS_TYPES_H
#define DNS_TYPES_H

#include <stdint.h>

#define MAX_PACOTE          512
#define MAX_NOME            256
#define TAMANHO_CABECALHO   12
#define TIPO_MX             15

typedef enum {
    RES_OK,
    RES_DOMINIO_INEXISTENTE,
    RES_SEM_MX,
    RES_DOMINIO_INVALIDO,
    RES_IP_INVALIDO,
    RES_FALHA
} resultado_t;

static inline uint16_t ler_16bits(const unsigned char *origem)
{
    return (uint16_t) ((origem[0] << 8) | origem[1]);
}

#endif
