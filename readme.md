# Documentação — Cliente DNS (Trabalho 01)

**Disciplina:** Fundamentos de Redes de Computadores
**Trabalho:** Cliente DNS (camada de aplicação)

---

## 1. Sistema Operacional utilizado

O desenvolvimento e os testes da aplicação foram realizados em:

- **Sistema Operacional:** Linux (distribuição baseada em Ubuntu/Debian)
- **Kernel:** Linux (compatível com chamadas de sistema POSIX de sockets — `socket()`, `sendto()`, `recvfrom()`, `setsockopt()`)

> A aplicação utiliza exclusivamente headers padrão POSIX (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`), portanto deve compilar e rodar em qualquer distribuição Linux ou sistema compatível com POSIX (incluindo WSL e macOS), sem necessidade de adaptação.

## 2. Ambiente de desenvolvimento

- **Editor/IDE:** Visual Studio Code
- **Compilador:** GCC (GNU Compiler Collection)
- **Linguagem:** C (padrão C99/C11)
- **Ferramentas auxiliares de depuração/teste:** `dig`, `tcpdump`

## 3. Como construir (compilar) a aplicação

Nenhuma dependência externa é necessária além do GCC e do `make`. O código é dividido em quatro módulos (`main.c`, `dns_client.c`, `dns_encode.c` e `dns_parse.c`), compilados pelo [Makefile](Makefile):

```bash
make
```

- O Makefile usa `gcc -Wall -Wextra -std=c99`, garantindo um código livre de warnings.
- O `Makefile` gera o executável `dns_client` no diretório atual.
- Para remover os objetos e o executável: `make clean`.

Alternativamente, sem o `make`:

```bash
gcc -Wall -Wextra -std=c99 -o dns_client main.c dns_client.c dns_encode.c dns_parse.c
```

## 4. Como executar a aplicação

### Sintaxe

```bash
./dns_client <nome_dominio> <ip_servidor_dns>
```

| Parâmetro | Descrição |
|---|---|
| `nome_dominio` | Nome de domínio cujo registro MX será consultado (ex.: `unb.br`) |
| `ip_servidor_dns` | Endereço IPv4 (não hostname) do servidor DNS a ser consultado, na porta UDP 53 (ex.: `8.8.8.8`) |

### Exemplo de execução

```bash
./dns_client unb.br 8.8.8.8
```

## 5. Telas / Instruções de uso (cenários de saída)

O resultado de toda consulta (sucesso ou falha) é impresso em uma única linha na saída padrão (`stdout`), variando conforme o resultado. Apenas erros de uso da linha de comando (5.5) são enviados à saída de erro (`stderr`):

### 5.1 Resolução bem-sucedida

```
$ ./dns_client unb.br 8.8.8.8
unb.br <> unb-br.mail.protection.outlook.com
```

### 5.2 Domínio não existe (NXDOMAIN)

```
$ ./dns_client imagdaskdasdasj.br 1.1.1.1
Dominio imagdaskdasdasj.br nao encontrado
```

### 5.3 Domínio existe, mas não possui registro MX

```
$ ./dns_client fga.unb.br 8.8.8.8
Dominio fga.unb.br nao possui entrada MX
```

### 5.4 Servidor DNS não respondeu (após esgotar as tentativas)

```
$ ./dns_client unb.br 1.2.3.4
Nao foi possível coletar entrada MX para unb.br
```

Nesse cenário, o cliente aguarda até **2 segundos** por tentativa e realiza no máximo **3 tentativas** antes de reportar a falha (tempo total máximo de espera: ~6 segundos).

### 5.5 Uso incorreto (número de argumentos inválido)

Mensagem enviada ao `stderr`; o programa termina com código de saída diferente de zero. O mesmo vale para IP de servidor inválido e rótulo de domínio inválido.

```
$ ./dns_client
Uso: ./dns_client <nome_dominio> <ip_servidor_dns>
```

## 6. Detalhes de implementação relevantes

- `main.c` concentra a interface de linha de comando e a apresentação dos resultados.
- `dns_client.c` encapsula a comunicação UDP, o timeout, as tentativas e a validação do Transaction ID.
- `dns_client.h` define a interface pública da consulta DNS.
- `dns_encode.c` e `dns_parse.c` permanecem responsáveis, respectivamente, pela montagem e interpretação das mensagens DNS.

- O payload da consulta DNS é **montado manualmente**, byte a byte, sem uso de bibliotecas de resolução de nomes (não são usadas `getaddrinfo`, `gethostbyname`, `resolv.h`, etc.). Apenas a interface de sockets UDP padrão do sistema operacional é utilizada para o envio/recebimento.
- **Transaction ID:** gerado aleatoriamente a cada consulta (16 bits) e validado na resposta recebida.
- **Flags:** fixas em `0x0100` (consulta recursiva).
- **Consulta:** sempre do tipo `MX` (código 15), classe `IN` (código 1), com `QDCOUNT = 1`.
- **Interpretação da resposta:** o cliente lê o campo `RCODE` do cabeçalho para diferenciar domínio inexistente (RCODE = 3) de ausência de registro MX (RCODE = 0 e `ANCOUNT = 0`, ou resposta sem nenhum registro do tipo MX entre as *Answers*).
- **Compressão de nomes:** implementada conforme RFC 1035 (seção 4.1.4), necessária para decodificar corretamente o nome do servidor de e-mail (*exchange*) retornado no registro MX, que costuma usar ponteiros de compressão apontando para nomes já presentes no pacote.

## 7. Limitações conhecidas

- **Somente consultas MX** são suportadas (conforme escopo definido no enunciado); outros tipos de registro (A, AAAA, CNAME, TXT, etc.) não são implementados.
- **Apenas IPv4**: o endereço do servidor DNS deve ser informado como um IPv4 válido (`inet_pton` com `AF_INET`); não há suporte a IPv6 nem a hostnames como servidor.
- **Um único registro MX é exibido**: caso o domínio possua múltiplos registros MX (com diferentes prioridades), o cliente reporta apenas o primeiro encontrado na resposta, não havendo tratamento de prioridade (`preference`) nem exibição de todos os registros.
- **Sem suporte a EDNS0**: o tamanho máximo do pacote assumido é o padrão de 512 bytes; respostas maiores que isso (que exigiriam EDNS0/TCP fallback) não são tratadas.
- **Sem fallback para TCP**: caso a resposta seja truncada (`TC = 1`), o cliente não reenvia a consulta via TCP, como preveem as RFCs para respostas grandes.
- **Timeout e tentativas fixos no código**: 2 segundos por tentativa e 3 tentativas no total, sem possibilidade de configuração via linha de comando.
- **Validação de entrada limitada**: não há validação de sintaxe de nome de domínio (ex.: caracteres inválidos) além dos limites básicos de tamanho de rótulo (RFC 1035, máx. 63 bytes por rótulo).
- Não há criptografia/verificação de integridade (DNSSEC), como esperado em um cliente DNS simples/didático.

---

*Documentação elaborada como parte da entrega do Trabalho 01 da disciplina Fundamentos de Redes de Computadores.*