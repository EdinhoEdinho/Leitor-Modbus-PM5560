# Leitor Modbus PM5560

Este projeto contém um leitor Modbus TCP em C para o medidor Schneider Electric PM5560.

O programa:
- conecta em um dispositivo Modbus TCP
- lê valores `float32` em Holding Registers
- permite informar `IP`, `porta`, `unit_id` e endereços das tags em tempo de execução
- imprime correntes e tensões no terminal

## Arquivos do projeto

- `modbus_pm5560.c` - código-fonte principal do leitor Modbus
- `.vscode/launch.json` - configuração para rodar e debugar com `F5`
- `.vscode/tasks.json` - tarefa de build automática no VSCode

## Requisitos

- Windows
- VSCode
- compilador C com `gcc`
- `gdb` para depuração
- biblioteca `libmodbus`

## Como compilar

No terminal, na pasta do projeto:

```bash
gcc modbus_pm5560.c -o modbus_pm5560.exe -lmodbus
```

Se a `libmodbus` estiver em outro diretório, pode ser necessário informar os caminhos de include e library:

```bash
gcc modbus_pm5560.c -o modbus_pm5560.exe -IC:\libs\libmodbus\include -LC:\libs\libmodbus\lib -lmodbus
```

## Como executar

```bash
.\modbus_pm5560.exe
```

O programa vai pedir:

- IP do medidor
- porta Modbus
- `unit_id`
- endereços das tags de corrente
- endereços das tags de tensão

## Como rodar com F5 no VSCode

1. Abra a pasta do projeto no VSCode
2. Confirme que `gcc`, `gdb` e `libmodbus` estão instalados
3. Pressione `F5`
4. Escolha a configuração `Debug modbus_pm5560`, se solicitado

## Formato de leitura

O leitor assume que cada `float32` ocupa 2 registradores Modbus de 16 bits.

Se o valor lido aparecer incorreto, o motivo mais comum é a ordem dos registradores. Nesse caso, ajuste a função `regs_to_float()` em `modbus_pm5560.c`.

## Exemplo de uso

Exemplo de entrada no console:

```text
Digite o IP do medidor: ***.**.**.***
Digite a porta Modbus [502]: 502
Digite o unit_id do medidor [6]: 6
Endereco da tag IA: 2999
Endereco da tag IB: 3001
...
```

## Observações

- O código lê uma tag por vez, sempre em 2 registradores.
- Os nomes das tags são apenas identificadores de exibição.
- Os endereços devem ser informados conforme o mapa Modbus do equipamento.



