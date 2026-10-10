#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dns_client.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_dominio> <ip_servidor_dns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dominio     = argv[1];
    const char *ip_servidor = argv[2];
    char servidor_email[MAX_NOME];

    srand((unsigned int) time(NULL) ^ (unsigned int) getpid());

    switch (consultar_mx(dominio, ip_servidor, servidor_email, sizeof(servidor_email))) {
    case RES_OK:
        printf("%s <> %s\n", dominio, servidor_email);
        return EXIT_SUCCESS;

    case RES_DOMINIO_INEXISTENTE:
        printf("Dominio %s nao encontrado\n", dominio);
        return EXIT_SUCCESS;

    case RES_SEM_MX:
        printf("Dominio %s nao possui entrada MX\n", dominio);
        return EXIT_SUCCESS;

    case RES_DOMINIO_INVALIDO:
        fprintf(stderr, "Erro: nome de dominio invalido '%s'\n", dominio);
        return EXIT_FAILURE;

    case RES_IP_INVALIDO:
        fprintf(stderr, "Erro: IP de servidor invalido '%s'\n", ip_servidor);
        return EXIT_FAILURE;

    case RES_FALHA:
        break;
    }

    printf("Nao foi possível coletar entrada MX para %s\n", dominio);

    return EXIT_FAILURE;
}
