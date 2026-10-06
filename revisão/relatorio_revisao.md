# Relatório de revisão - Cliente DNS

## 1. Objetivo

O projeto implementa um cliente DNS em C para consultar registros do tipo MX
(Mail Exchanger). O programa recebe o domínio e o endereço IPv4 do servidor DNS
pela linha de comando:

```text
./dns_client <nome_dominio> <ip_servidor_dns>
```

A aplicação monta manualmente a mensagem DNS, envia a consulta por UDP para a
porta 53, aguarda a resposta e extrai o nome do servidor de e-mail.

## 2. Organização do projeto

| Arquivo | Responsabilidade |
| --- | --- |
| `main.c` | Entrada do programa, argumentos e apresentação dos resultados |
| `dns_client.c` | Socket UDP, envio, recebimento, timeout e tentativas |
| `dns_client.h` | Interface pública da comunicação DNS |
| `dns_encode.c` | Codificação do domínio e montagem da consulta DNS |
| `dns_parse.c` | Interpretação do cabeçalho e dos registros da resposta |
| `dns_types.h` | Constantes, estrutura do cabeçalho e resultados |
| `dns_encode.h` | Interface do módulo de codificação |
| `dns_parse.h` | Interface do módulo de parsing |
| `Makefile` | Compilação e limpeza do projeto |

A separação organiza o código por responsabilidade: comunicação, codificação e
interpretação da mensagem DNS ficam em módulos distintos.

## 3. Fundamentos de Redes de Computadores

### 3.1 Modelo cliente-servidor

O programa é o cliente. Ele envia uma requisição para um servidor DNS externo,
como `8.8.8.8` ou `1.1.1.1`, e aguarda a resposta. O projeto não implementa
um servidor DNS próprio. A interface em `main.c` delega a consulta para
`dns_client.c`, que encapsula a comunicação UDP.

### 3.2 Sockets e UDP

O socket é criado com IPv4 e UDP:

```c
int sock = socket(AF_INET, SOCK_DGRAM, 0);
if (sock < 0) {
    perror("socket");
    return EXIT_FAILURE;
}
```

`AF_INET` representa IPv4 e `SOCK_DGRAM` representa comunicação por
Datagramas UDP. O destino usa a porta padrão do DNS:

```c
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(DNS_PORT);

if (inet_pton(AF_INET, ip_servidor, &server_addr.sin_addr) != 1) {
    fprintf(stderr, "Erro: IP de servidor invalido '%s'\n", ip_servidor);
    close(sock);
    return EXIT_FAILURE;
}
```

`htons()` converte a porta para a ordem de bytes da rede e `inet_pton()` converte
o IPv4 textual para o formato binário usado pelo socket.

### 3.3 Timeout e retransmissão

Como o UDP não garante entrega, o cliente configura um timeout de dois segundos:

```c
struct timeval tv;
tv.tv_sec = TIMEOUT_SEC;
tv.tv_usec = 0;
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

A consulta é enviada no máximo três vezes:

```c
for (tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++) {
    ssize_t sent = sendto(sock, query, query_len, 0,
                           (struct sockaddr *) &server_addr,
                           sizeof(server_addr));

    recv_len = recvfrom(sock, response, sizeof(response), 0,
                        (struct sockaddr *) &from_addr, &from_len);

    if (recv_len < 0) {
        continue;
    }
}
```

Esse mecanismo trata perdas de datagramas e atende ao requisito de três
tentativas com espera de dois segundos.

### 3.4 Transaction ID

Cada consulta recebe um identificador aleatório de 16 bits:

```c
uint16_t id = (uint16_t) (rand() & 0xFFFF);
hdr->id = htons(id);
*out_id = id;
```

A resposta somente é aceita se possuir o mesmo identificador:

```c
dns_header_t *resp_hdr = (dns_header_t *) response;
if (ntohs(resp_hdr->id) != query_id) {
    recv_len = -1;
    continue;
}
```

Isso associa a resposta à consulta correta e evita aceitar pacotes de outra
transação DNS.

## 4. Montagem do payload DNS

### 4.1 Cabeçalho

O cabeçalho DNS possui 12 bytes. O projeto define os campos exigidos pelo
enunciado:

```c
hdr->flags   = htons(0x0100); /* consulta recursiva */
hdr->qdcount = htons(0x0001); /* uma pergunta */
hdr->ancount = 0x0000;
hdr->nscount = 0x0000;
hdr->arcount = 0x0000;
```

Assim, a consulta solicita recursão, possui uma pergunta e não possui registros
Answer, Authority ou Additional no pacote enviado.

### 4.2 Codificação do domínio

O nome DNS é codificado em rótulos. Cada rótulo começa com seu tamanho e o
nome termina com `0x00`:

```c
dot = strchr(start, '.');
int len = dot ? (int)(dot - start) : (int)strlen(start);

buffer[pos++] = (unsigned char) len;
memcpy(buffer + pos, start, len);
pos += len;
```

Por exemplo, `unb.br` é representado como:

```text
03 'u' 'n' 'b' 02 'b' 'r' 00
```

O tipo da consulta é MX e a classe é IN:

```c
uint16_t qtype  = htons(QTYPE_MX);
uint16_t qclass = htons(QCLASS_IN);
memcpy(buffer + offset, &qtype, 2);
offset += 2;
memcpy(buffer + offset, &qclass, 2);
offset += 2;
```

Esse formato segue a estrutura de mensagens DNS definida pelas RFCs 1034 e
1035.

## 5. Interpretação da resposta

O parser lê os campos do cabeçalho e verifica o `RCODE`:

```c
uint16_t flags = ntohs(hdr->flags);
uint16_t ancount = ntohs(hdr->ancount);
int rcode = flags & 0x000F;

if (rcode == 3) {
    return RES_NXDOMAIN;
}
if (rcode != 0) {
    return RES_ERRO_SERVIDOR;
}
if (ancount == 0) {
    return RES_SEM_MX;
}
```

O programa diferencia domínio inexistente, erro do servidor e domínio sem
registro MX. Quando encontra um registro MX, extrai o nome do servidor de
correio presente no campo RDATA.

## 6. Compressão de nomes DNS

As respostas DNS normalmente usam compressão de nomes. O parser identifica
ponteiros pelos dois bits mais significativos:

```c
if ((p[0] & 0xC0) == 0xC0) {
    int offset = ((p[0] & 0x3F) << 8) | p[1];
    p = buffer + offset;
    jumped = 1;
    continue;
}
```

Esse comportamento permite interpretar nomes que apontam para outras posições
do mesmo pacote, conforme a RFC 1035.

## 7. Validações realizadas

- Compilação com `gcc -Wall -Wextra -std=c99` sem warnings.
- Consulta válida para `unb.br`.
- Consulta válida para `gmail.com`.
- Tratamento de domínio inexistente com resposta `NXDOMAIN`.
- Rejeição de endereço IPv4 inválido.
- Mensagem de uso quando os argumentos estão incorretos.
- Limpeza dos artefatos com `make clean`.

Comandos atuais de compilação e execução:

```bash
make
./dns_client unb.br 8.8.8.8
make clean
```

## 8. Pontos positivos

- O payload da consulta DNS é montado manualmente.
- A comunicação usa sockets UDP, conforme solicitado.
- A porta padrão 53 é utilizada.
- O Transaction ID é gerado e conferido.
- O timeout e as três tentativas estão implementados.
- O tipo MX e a classe IN são configurados corretamente.
- O parser trata `NXDOMAIN` e ausência de registro MX.
- A compressão de nomes DNS é suportada.
- O código está dividido em módulos `.c` e `.h`.
- O projeto usa conversões de ordem de bytes com `htons()` e `ntohs()`.
- O `Makefile` automatiza compilação e limpeza.

## 9. Melhorias recomendadas

### 9.1 Validar tamanhos de buffers

`encode_qname()` e `build_query()` recebem ponteiros, mas não recebem a
capacidade dos buffers. O ideal é adicionar um parâmetro `size_t` e impedir
escritas quando não houver espaço suficiente.

### 9.2 Evitar `exit()` dentro de módulos

Atualmente, um domínio inválido encerra o processo dentro de `encode_qname()`:

```c
if (len <= 0 || len > 63) {
    fprintf(stderr, "Erro: rotulo invalido no dominio '%s'\n", domain);
    exit(EXIT_FAILURE);
}
```

Uma interface mais encapsulada retornaria um código de erro e deixaria `main()`
decidir como informar o problema.

### 9.3 Tornar funções internas privadas

Se `parse_name()` não for necessária fora de `dns_parse.c`, ela pode ser
marcada como `static` e removida de `dns_parse.h`. Isso reduz a interface
pública do módulo.

### 9.4 Validar respostas malformadas

O parser deve conferir os limites antes de acessar cada campo, validar
`RDLENGTH` e impedir que ponteiros de compressão apontem para fora do pacote.

### 9.5 Tratar respostas truncadas

O cliente pode verificar a flag `TC` e implementar fallback para TCP ou emitir
uma mensagem específica quando a resposta UDP estiver truncada.

### 9.6 Corrigir a documentação existente

O README principal ainda deve ser atualizado para usar o `Makefile`, pois a
compilação atual depende de `clienteDNS.c`, `dns_encode.c` e `dns_parse.c`.
Também deve informar que executáveis e arquivos objeto não devem ser enviados
no ZIP da entrega. A documentação foi atualizada para usar o `Makefile` e para
descrever a separação entre `main.c` e `dns_client.c`.

### 9.7 Tratar o registro MX raiz

Quando o servidor MX retornado for `.`, o resultado atualmente pode aparecer
com o nome vazio. O parser poderia representar explicitamente esse caso como
`.`.

## 10. Conclusão

O projeto implementa o núcleo solicitado no Trabalho 01: um cliente DNS em C
que monta manualmente uma consulta MX, utiliza UDP, trata timeout e interpreta
respostas DNS. A arquitetura está adequada ao escopo acadêmico e apresenta
separação razoável entre comunicação, codificação e parsing.

As melhorias prioritárias são atualizar o README, adicionar validações de
limites nos buffers e tornar o parser mais resistente a pacotes malformados.
