/* Interface de linha de comando do cliente DNS. */

#include <stdio.h>
#include <stdlib.h>

#include "dns_client.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_dominio> <ip_servidor_dns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dominio = argv[1];
    char exchange[MAX_NAME_LEN];
    resultado_t resultado = dns_query(dominio, argv[2], exchange, sizeof(exchange));

    switch (resultado) {
        case RES_OK:
            printf("%s <> %s\n", dominio, exchange);
            break;
        case RES_NXDOMAIN:
            printf("Dominio %s nao encontrado\n", dominio);
            break;
        case RES_SEM_MX:
            printf("Dominio %s nao possui entrada MX\n", dominio);
            break;
        case RES_ERRO_SERVIDOR:
        default:
            printf("Nao foi possível coletar entrada MX para %s\n", dominio);
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}