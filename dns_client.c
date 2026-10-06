/* Comunicacao com servidor DNS por UDP. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "dns_client.h"
#include "dns_encode.h"
#include "dns_parse.h"

/* Consulta um registro MX e escreve o exchange no buffer de saida. */
resultado_t dns_query(const char *domain, const char *server_ip,
                      char *exchange, size_t exchange_size) {
    /* Inicializa o gerador usado pelo Transaction ID da consulta. */
    srand((unsigned int) time(NULL) ^ (unsigned int) getpid());

    /* Monta o pacote DNS antes de abrir a comunicacao UDP. */
    unsigned char query[MAX_PACKET];
    uint16_t query_id;
    int query_len = build_query(domain, query, &query_id);

    /* Cria o socket UDP IPv4. */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return RES_ERRO_SERVIDOR;
    }

    /* Configura o tempo maximo de espera por cada resposta. */
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        close(sock);
        return RES_ERRO_SERVIDOR;
    }

    /* Configura o endereco IPv4 e a porta padrao do servico DNS. */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Erro: IP de servidor invalido '%s'\n", server_ip);
        close(sock);
        return RES_ERRO_SERVIDOR;
    }

    unsigned char response[MAX_PACKET];
    int recv_len = -1;

    /* Envia novamente a consulta quando ocorre timeout ou resposta invalida. */
    for (int tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++) {
        ssize_t sent = sendto(sock, query, query_len, 0,
                               (struct sockaddr *) &server_addr,
                               sizeof(server_addr));
        if (sent < 0) {
            perror("sendto");
            close(sock);
            return RES_ERRO_SERVIDOR;
        }

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        recv_len = recvfrom(sock, response, sizeof(response), 0,
                            (struct sockaddr *) &from_addr, &from_len);

        if (recv_len < 0) {
            /* Timeout ou falha no recebimento: realiza nova tentativa. */
            continue;
        }

        if (recv_len < (int) sizeof(dns_header_t)) {
            /* Resposta menor que o cabecalho DNS nao pode ser interpretada. */
            recv_len = -1;
            continue;
        }

        dns_header_t *response_header = (dns_header_t *) response;
        if (ntohs(response_header->id) != query_id) {
            /* Ignora pacote pertencente a outra transacao DNS. */
            recv_len = -1;
            continue;
        }

        break;
    }

    close(sock);

    if (recv_len < 0) {
        return RES_ERRO_SERVIDOR;
    }

    /* Interpreta a resposta e extrai o primeiro registro MX encontrado. */
    return parse_response(response, recv_len, exchange, exchange_size);
}