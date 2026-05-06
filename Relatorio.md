# Trabalho Prático 1 - Construção de Analisadores Léxico e Sintático
**Disciplina:** Compiladores (2026.1)
**Instituição:** UTFPR - Câmpus Ponta Grossa
**Aluno(s):** Rainier Ryu Waki (a2664542), Leandro Martins Machado Hyeda (a2664488)

---

## Apresentação da Linguagem
A linguagem implementada consiste em uma extensão de uma Calculadora Avançada (Programação Básica). Foram adicionadas estruturas de controle de fluxo (`FOR`) e operadores lógicos (`AND`, `OR`). A linguagem suporta declaração de variáveis locais, chamadas de funções definidas pelo usuário, funções matemáticas embutidas (`sqrt`, `exp`, `log`, `print`) e blocos condicionais/repetição (`IF/THEN/ELSE`, `WHILE/DO`).

## Regras de Análise Léxica e Diagramas
A análise léxica reconhece os seguintes padrões:
* **Funções pré-definidas:** `sqrt`, `exp`, `log`, `print`.
* **Terminais de Controle:** `if`, `then`, `else`, `while`, `do`, `let`, `for`.
* **Operadores Lógicos:** `&&` (AND), `||` (OR).
* **Operadores Relacionais:** `>`, `<`, `!=`, `==`, `>=`, `<=`.
* **Aritméticos/Atribuição:** `+`, `-`, `*`, `/`, `=`.
* **Delimitadores:** `;`, `(`, `)`.
* **Identificadores (`NAME`):** Obrigatoriamente iniciam com uma letra, seguidos de letras ou números opcionais.
* **Numéricos (`NUMBER`):** Ponto flutuante com suporte a notação científica.

**OBS:** Um agrupamento conceitual foi feito acima, mas o diagrama de transições reflete exatamente o funcionamento do scanner, cada token diferente precisa de um estado de aceitação distinto (pois retornam códigos diferentes ao parser: IF, THEN, ELSE…).

![Diagrama Do Debuggex com as regras léxicas](DiagramaTransicao.png)

## Descrição do Analisador Léxico (FLEX)
Implementado no arquivo `calcAvancada.l`. Utiliza expressões regulares para tokenizar o fluxo de entrada.
* **Ignorados:** Espaços em branco (`[ \t\r]`) e comentários (linhas iniciadas com `//`).
* **Conversão de Tipos:** Valores numéricos são convertidos via `atof` e armazenados em `yylval.d`. Nomes de variáveis são buscados/inseridos na Tabela de Símbolos usando a função `lookup(yytext)` e armazenados em `yylval.s`.
* **Operadores de Múltiplos Caracteres:** Operadores como `>=`, `&&` e palavras-chave retornam tokens nomeados (`CMP`, `AND`, `FOR`) para o Bison.

## Descrição da Tabela de Símbolos (TS)
Gerenciada estaticamente no arquivo `calcAvancada.c`.
* **Estrutura:** Um array `symtab` de tamanho fixo (`NHASH = 9997`).
* **Resolução de Colisão:** Hashing simples (`hash*9 ^ c`) com tratamento de colisão por sondagem linear.
* **Conteúdo:** A estrutura `symbol` armazena o nome da variável (`name`), seu valor numérico atual (`value`), ponteiro para raiz da AST de função (`func`) e lista de argumentos (`syms`).

## Regras de Análise Sintática (BNF)

A análise sintática foi desenvolvida utilizando a ferramenta Bison. O analisador processa o fluxo de tokens fornecido pelo Flex e constrói uma Árvore Sintática Abstrata (AST) de forma incremental.

A construção da Árvore se dá por ações semânticas do Bison, que instanciam estruturas de dados em C (nós da AST). Cada produção da gramática retorna um ponteiro para um nó específico, como `struct flow` para estruturas de controle ou `struct symasgn` para atribuições. A avaliação real (execução) ocorre apenas após a redução completa de um comando, disparada pela função `eval()`.

Para viabilizar uma gramática de expressões compacta e eficiente, as ambiguidades foram resolvidas por meio de diretivas de precedência e associatividade no arquivo `calcAvancada.y`. Os operadores AND e OR foram configurados com %left, garantindo que expressões como `a || b && c` sejam avaliadas da esquerda para a direita. A precedência desses operadores lógicos foi definida como a segunda mais baixa da gramática, assegurando que operações aritméticas e comparações sejam resolvidas prioritariamente.

O suporte ao laço FOR foi implementado através de uma reorganização dos ponteiros para reutilizar a lógica de um laço WHILE. Ao reconhecer a assinatura `FOR(init; cond; inc) list`, a inicialização é posicionada como o primeiro comando de uma lista, seguida pelo nó de repetição que contém, ao fim de seu corpo original, a expressão de incremento. Essa abordagem garante consistência semântica e simplifica o interpretador (eval).

#### Gramática

``` bnf

<calclist> ::= ε
             | <calclist> <stmt> EOL
             | <calclist> LET NAME ( <symlist> ) = <list> EOL
             | <calclist> error EOL

<stmt> ::= IF <exp> THEN <list>
         | IF <exp> THEN <list> ELSE <list>
         | WHILE <exp> DO <list>
         | FOR ( <exp> ; <exp> ; <exp> ) <list>
         | <exp>

<list> ::= ε
         | <stmt> ; <list>

<exp> ::= <exp> CMP <exp>
        | <exp> + <exp>
        | <exp> - <exp>
        | <exp> * <exp>
        | <exp> / <exp>
        | <exp> AND <exp>
        | <exp> OR <exp>
        | ( <exp> )
        | NUMBER
        | NAME
        | NAME = <exp>
        | FUNC ( <explist> )
        | NAME ( <explist> )

<explist> ::= <exp>
            | <exp> , <explist>

<symlist> ::= NAME
            | NAME , <symlist>
            
```

#### Diagrama:

![Diagrama Do Railroad](DiagramaSintatico.png)

## Conjunto de Testes

A leitura e escrita (I/O) foram alteradas para suportar arquivos via yyin e yyout, permitindo testes em lote.

##### Teste 0: Operadores Lógicos

Entrada:

``` c
verdadeiro = 1
falso = 0//Teste comentario
if (verdadeiro && falso) then print(999); else print(0);
if (verdadeiro || falso) then print(111); else print(0);
```

Saída Esperada: `= 0 \n = 111`

##### Teste 1: Operadores Lógicos com Comparadores

Entrada:

``` c
x = 10
if (x > 5 && x < 15) then print(x);
```

Saída Esperada: `= 10`

##### Teste 2: Laço FOR e Acumuladores

Entrada:
``` c
x = 0
while x < 3 do x = x + 1;//While ainda funciona
soma = 0
for(i = 1; i <= 5; i = i + 1) soma = soma + i;
soma

```

Saída Esperada: `>= 6` e `= 15` (Acumulo do i e soma de 1+2+3+4+5)

##### Teste 3: Precedência de Operadores e Parênteses

Entrada:
``` c
a = 10
b = 5
c = a + b * 2
print(c)
d = (a + b) * 2
print(d)
```

Saída Esperada: `= 20 \n = 30`

##### Teste 4: Tratamento de Erro Sintático

Entrada:

```c
a = 10
b = 5 + * 3
for(i = 0 i < 5; i = i + 1) a = a + 1;
print(a)
```

Saída Esperada: `error: syntax error` (O parser captura a justaposição dos tokens aritméticos + e * na mesma expressão, bem como a falta do ponto-e-vírgula no segundo FOR).

##### Teste 5: Funções Pré-definidas e de Usuário

Entrada:
``` c
sqrt(144) //outras funcoes ainda funcionam
let loop(x, y) = for(i=0;i<y;i=i+1) x = x * 2;; print(x);
loop(10, 3)
```

Saída Esperada: `= 12 \n = 80 \n = 80` (A função aplica 10 * 2 três vezes sequenciais).

## Uso de Inteligência Artificial

Foi utilizado ferramentas de IA (Gemini e Deepseek) para revisão e indicação dos erros cometidos durante o processo de programação, como:

### Correções de código

1. No arquivo `calcAvancada.c`:
    * falta de recursão na função `treefree()`, bem como a ausência da liberação dos filhos esquerdos em nós com dois filhos.

2. No arquivo `calcAvancada.y`:
    * indicações de quais funções do `calcAvancada.c` chamar para criar um laço FOR adequadamente em `stmt`.

3. Criação do arquivo Makefile.

### Relatório

1. "Tradução" da gramática em BNF (feita por nós) para EBNF e REGEX, afim de gerar os dois diagramas pedidos; 

2. Revisão conceitual e gramatical dos textos.