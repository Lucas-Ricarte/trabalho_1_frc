# Checklist de requisitos — Trabalho 01 (Cliente DNS)

Conferência do projeto contra o enunciado `trabalho_01_2026.02.pdf` (Prof. Tiago Alves).

Legenda: `[x]` atendido · `[ ]` pendente ou a verificar · ⚠️ ponto de atenção

Testes executados no WSL (Linux) em 2026-10-10.

---

## 1. Linguagem e restrições
- [x] Escrito em C: linguagem compilada I, sem deflator (100% dos escores).
- [x] O payload da consulta DNS é montado programaticamente, byte a byte (`build_query` e `encode_qname` em `src/dns_encode.c`).
- [x] Não usa bibliotecas de comunicação nem de resolução (`getaddrinfo`, `gethostbyname`, `resolv.h`); apenas sockets UDP.
- [x] Compila sem erros nem warnings com `gcc -Wall -Wextra -std=c99` (`make`).

## 2. Linha de comando
- [x] O 1º argumento é o nome a resolver e o 2º é o IP do servidor DNS (`src/main.c`).
- [x] Com número errado de argumentos, mostra a mensagem de uso no stderr e sai com código 1. **Testado.**
- [x] IP inválido é rejeitado (`inet_pton` em `src/dns_client.c`). **Testado.**
  - ⚠️ Nesse caso o programa imprime o erro no stderr **e também** `Nao foi possível coletar entrada MX para ...` no stdout. Não viola o enunciado, mas a segunda mensagem é redundante.

## 3. Transporte (UDP)
- [x] Socket UDP (`SOCK_DGRAM`) para a porta 53 (`DNS_PORT` em `include/dns_types.h`).
- [x] Aguarda 2 segundos pela resposta (`TIMEOUT_SEC`, com prazo absoluto em `aguardar_resposta`).
- [x] Retransmite quando a resposta não chega, com até 3 tentativas (`MAX_TENTATIVAS`).
- [x] Depois da 3ª tentativa, informa o erro ao usuário.
- [x] Tempo total com servidor inexistente (`1.2.3.4`): **6,06 s**, ou seja, 3 × 2 s. **Testado.**

## 4. Formato do payload da consulta
- [x] Transaction ID: número aleatório de 16 bits (`rand() & 0xFFFF`, semente `time ^ pid`).
- [x] Flags = `0x0100` (consulta recursiva).
- [x] Questions = `0x0001`.
- [x] Answer RRs, Authority RRs e Additional RRs = `0x0000`.
- [x] Query: o Name informado no 1º argumento, com Type **MX** (15) e Class **IN** (1).
- [x] Faz apenas consultas do tipo MX.
- [ ] Inspecionar o pacote real no Wireshark ou com `sudo tcpdump -i any -X udp port 53`. Não foi possível neste ambiente, porque o tcpdump exige senha de sudo. Bytes esperados para `unb.br`:
  ```
  ID ID  01 00  00 01  00 00  00 00  00 00 | 03 75 6e 62 02 62 72 00  00 0f  00 01
  ```

## 5. Interpretação da resposta
- [x] Só aceita a resposta se o Transaction ID for igual ao da consulta.
- [x] RCODE = 3 (NXDOMAIN) é tratado como "domínio não encontrado".
- [x] RCODE = 0 sem registro MX nas Answers é tratado como "não possui entrada MX".
- [x] Extrai o nome do servidor de e-mail do RDATA do MX, com compressão de nomes (RFC 1035 §4.1.4).

## 6. Saída no stdout (texto idêntico ao do enunciado)
| Cenário | Comando | Saída esperada | Resultado |
|---|---|---|---|
| Sucesso | `./dns_client unb.br 8.8.8.8` | `unb.br <> unb-br.mail.protection.outlook.com` | [x] Testado |
| Domínio inexistente | `./dns_client imagdaskdasdasj.br 1.1.1.1` | `Dominio imagdaskdasdasj.br nao encontrado` | [x] Testado |
| Sem registro MX | `./dns_client fga.unb.br 8.8.8.8` | `Dominio fga.unb.br nao possui entrada MX` | [x] Testado |
| Servidor não responde | `./dns_client unb.br 1.2.3.4` | `Nao foi possível coletar entrada MX para unb.br` | [x] Testado |

## 7. Documentação (20% da nota), em `readme.md`
- [x] Qual sistema operacional foi usado.
- [x] Qual ambiente de desenvolvimento foi usado.
- [x] Como construir a aplicação.
- [x] Como executar a aplicação.
- [x] Quais são as telas (instruções de uso).
- [x] Quais são as limitações conhecidas.
- [ ] ⚠️ Confirmar se "Linux (Ubuntu/Debian)" descreve o ambiente real. Se o desenvolvimento foi no Windows com WSL, vale citar o WSL.

## 8. Entrega
- [ ] O ZIP se chama `nome_sobrenome_matricula_nome_sobrenome_matricula_trab01.zip`.
- [ ] O ZIP **não** contém executáveis nem objetos. Rodar `make clean` antes de compactar.
- [ ] O ZIP contém `src/`, `include/`, `Makefile` e `readme.md`. Decidir se a pasta `revisão/` entra.
- [ ] A entrega foi feita no ponto de coleta do Sigaa.

## 9. Riscos de robustez (não são requisitos do enunciado)
- ⚠️ `src/dns_parse.c` não confere limites. Não valida `recv_len` nem `RDLENGTH`, nem se os ponteiros de compressão ficam dentro do pacote. Uma resposta malformada pode causar leitura fora do buffer.
- ⚠️ `encode_qname` chama `exit()` dentro do módulo e não recebe o tamanho do buffer, então um domínio muito longo pode estourar o buffer de 512 bytes.
