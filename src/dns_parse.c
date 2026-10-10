#include <string.h>

#include "dns_parse.h"

#define RCODE_SUCESSO               0
#define RCODE_DOMINIO_INEXISTENTE   3
#define MASCARA_RCODE               0x000F
#define MASCARA_PONTEIRO            0xC0
#define MASCARA_DESLOCAMENTO        0x3F
#define MAX_ROTULO                  63
#define TAMANHO_TIPO_E_CLASSE       4
#define TAMANHO_CAMPOS_REGISTRO     10
#define TAMANHO_PREFERENCIA_MX      2

static int ler_nome(const unsigned char *pacote, int tamanho_pacote, int posicao,
                    char *nome, size_t capacidade)
{
    int posicao_apos_nome = -1;
    size_t escritos = 0;

    while (posicao < tamanho_pacote && pacote[posicao] != 0) {
        int byte_inicial = pacote[posicao];

        if ((byte_inicial & MASCARA_PONTEIRO) == MASCARA_PONTEIRO) {
            if (posicao + 1 >= tamanho_pacote) {
                return -1;
            }

            int destino = ((byte_inicial & MASCARA_DESLOCAMENTO) << 8) | pacote[posicao + 1];

            if (destino >= posicao) {
                return -1;
            }

            if (posicao_apos_nome < 0) {
                posicao_apos_nome = posicao + 2;
            }

            posicao = destino;
            continue;
        }

        size_t tamanho_rotulo = (size_t) byte_inicial;

        if (tamanho_rotulo > MAX_ROTULO
            || posicao + 1 + (int) tamanho_rotulo > tamanho_pacote
            || escritos + tamanho_rotulo + 2 > capacidade) {
            return -1;
        }

        if (escritos > 0) {
            nome[escritos++] = '.';
        }

        memcpy(nome + escritos, pacote + posicao + 1, tamanho_rotulo);
        escritos += tamanho_rotulo;
        posicao  += 1 + (int) tamanho_rotulo;
    }

    if (posicao >= tamanho_pacote) {
        return -1;
    }

    if (escritos == 0) {
        nome[escritos++] = '.';
    }

    nome[escritos] = '\0';

    if (posicao_apos_nome >= 0) {
        return posicao_apos_nome;
    }

    return posicao + 1;
}

resultado_t interpretar_resposta(const unsigned char *resposta, int tamanho,
                                 char *servidor_email, size_t capacidade)
{
    if (tamanho < TAMANHO_CABECALHO) {
        return RES_FALHA;
    }

    int rcode           = ler_16bits(resposta + 2) & MASCARA_RCODE;
    int total_perguntas = ler_16bits(resposta + 4);
    int total_respostas = ler_16bits(resposta + 6);

    if (rcode == RCODE_DOMINIO_INEXISTENTE) {
        return RES_DOMINIO_INEXISTENTE;
    }

    if (rcode != RCODE_SUCESSO) {
        return RES_FALHA;
    }

    char nome[MAX_NOME];
    int posicao = TAMANHO_CABECALHO;

    for (int i = 0; i < total_perguntas; i++) {
        posicao = ler_nome(resposta, tamanho, posicao, nome, sizeof(nome));

        if (posicao < 0) {
            return RES_FALHA;
        }

        posicao += TAMANHO_TIPO_E_CLASSE;
    }

    for (int i = 0; i < total_respostas; i++) {
        posicao = ler_nome(resposta, tamanho, posicao, nome, sizeof(nome));

        if (posicao < 0 || posicao + TAMANHO_CAMPOS_REGISTRO > tamanho) {
            return RES_FALHA;
        }

        int tipo          = ler_16bits(resposta + posicao);
        int tamanho_dados = ler_16bits(resposta + posicao + 8);
        posicao += TAMANHO_CAMPOS_REGISTRO;

        if (posicao + tamanho_dados > tamanho) {
            return RES_FALHA;
        }

        if (tipo == TIPO_MX) {
            int posicao_servidor = posicao + TAMANHO_PREFERENCIA_MX;

            if (ler_nome(resposta, tamanho, posicao_servidor, servidor_email, capacidade) < 0) {
                return RES_FALHA;
            }

            return RES_OK;
        }

        posicao += tamanho_dados;
    }

    return RES_SEM_MX;
}
