#include <string.h>

#include "dns_encode.h"

#define MAX_ROTULO                  63
#define MAX_DOMINIO                 253
#define FLAGS_CONSULTA_RECURSIVA    0x0100
#define UMA_PERGUNTA                0x0001
#define NENHUM_REGISTRO             0x0000
#define CLASSE_IN                   1

static unsigned char *escrever_16bits(unsigned char *cursor, uint16_t valor)
{
    cursor[0] = (unsigned char) (valor >> 8);
    cursor[1] = (unsigned char) (valor & 0xFF);

    return cursor + 2;
}

static unsigned char *codificar_nome(const char *dominio, unsigned char *cursor)
{
    while (*dominio != '\0') {
        size_t tamanho_rotulo = strcspn(dominio, ".");

        if (tamanho_rotulo == 0 || tamanho_rotulo > MAX_ROTULO) {
            return NULL;
        }

        *cursor++ = (unsigned char) tamanho_rotulo;
        memcpy(cursor, dominio, tamanho_rotulo);
        cursor += tamanho_rotulo;

        dominio += tamanho_rotulo;

        if (*dominio == '.') {
            dominio++;
        }
    }

    *cursor++ = 0;

    return cursor;
}

int montar_consulta(const char *dominio, uint16_t id, unsigned char *consulta)
{
    size_t tamanho_dominio = strlen(dominio);

    if (tamanho_dominio == 0 || tamanho_dominio > MAX_DOMINIO) {
        return -1;
    }

    unsigned char *cursor = consulta;

    cursor = escrever_16bits(cursor, id);
    cursor = escrever_16bits(cursor, FLAGS_CONSULTA_RECURSIVA);
    cursor = escrever_16bits(cursor, UMA_PERGUNTA);
    cursor = escrever_16bits(cursor, NENHUM_REGISTRO);
    cursor = escrever_16bits(cursor, NENHUM_REGISTRO);
    cursor = escrever_16bits(cursor, NENHUM_REGISTRO);

    cursor = codificar_nome(dominio, cursor);

    if (cursor == NULL) {
        return -1;
    }

    cursor = escrever_16bits(cursor, TIPO_MX);
    cursor = escrever_16bits(cursor, CLASSE_IN);

    return (int) (cursor - consulta);
}
