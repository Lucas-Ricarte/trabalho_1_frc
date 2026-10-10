#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "dns_client.h"
#include "dns_encode.h"
#include "dns_parse.h"

#define PORTA_DNS           53
#define TEMPO_ESPERA_MS     2000
#define MAX_TENTATIVAS      3

static long long milissegundos_atuais(void)
{
    struct timeval agora;
    gettimeofday(&agora, NULL);

    return agora.tv_sec * 1000LL + agora.tv_usec / 1000;
}

static int aguardar_resposta(int sock, uint16_t id, unsigned char *resposta)
{
    long long prazo = milissegundos_atuais() + TEMPO_ESPERA_MS;
    struct pollfd evento = { .fd = sock, .events = POLLIN };
    long long restante;

    while ((restante = prazo - milissegundos_atuais()) > 0) {
        if (poll(&evento, 1, (int) restante) <= 0) {
            return -1;
        }

        ssize_t recebidos = recv(sock, resposta, MAX_PACOTE, 0);

        if (recebidos < 0) {
            return -1;
        }

        if (recebidos >= TAMANHO_CABECALHO && ler_16bits(resposta) == id) {
            return (int) recebidos;
        }
    }

    return -1;
}

resultado_t consultar_mx(const char *dominio, const char *ip_servidor,
                         char *servidor_email, size_t capacidade)
{
    struct sockaddr_in servidor = { .sin_family = AF_INET, .sin_port = htons(PORTA_DNS) };

    if (inet_pton(AF_INET, ip_servidor, &servidor.sin_addr) != 1) {
        return RES_IP_INVALIDO;
    }

    unsigned char consulta[MAX_PACOTE];
    uint16_t id = (uint16_t) rand();
    int tamanho_consulta = montar_consulta(dominio, id, consulta);

    if (tamanho_consulta < 0) {
        return RES_DOMINIO_INVALIDO;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return RES_FALHA;
    }

    unsigned char resposta[MAX_PACOTE];
    int tamanho_resposta = -1;

    for (int tentativa = 0; tentativa < MAX_TENTATIVAS && tamanho_resposta < 0; tentativa++) {
        if (sendto(sock, consulta, tamanho_consulta, 0,
                   (struct sockaddr *) &servidor, sizeof(servidor)) < 0) {
            perror("sendto");
            break;
        }

        tamanho_resposta = aguardar_resposta(sock, id, resposta);
    }

    close(sock);

    if (tamanho_resposta < 0) {
        return RES_FALHA;
    }

    return interpretar_resposta(resposta, tamanho_resposta, servidor_email, capacidade);
}
