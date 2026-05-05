/* Funcoes Auxiliares para uma calculadora avancada */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "calcAvancada.h"

struct symbol symtab[NHASH];
extern FILE *yyin;
extern FILE *yyout;

/* funcoes em C para TS */
static unsigned symhash(char *sym) {
  unsigned int hash = 0;
  unsigned c;
    while(c = *sym++) hash = hash*9 ^ c;
    return hash;
}

struct symbol *lookup(char *sym) {
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;

    while(--scount >= 0) {
        if(sp->name && !strcasecmp(sp->name, sym))
            return sp;

        if(!sp->name) { /* nova entrada na TS */
            sp->name = strdup(sym);
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }

        if(++sp >= symtab + NHASH) sp = symtab; /* tenta a prox. entrada */
    }
    yyerror("overflow na tab simbolos\n");
    abort(); /* tabela estah cheia */
}

struct ast *newast(int nodetype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newnum(double d) {
    struct numval *a = malloc(sizeof(struct numval));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = 'K';
    a->number = d;
    return (struct ast *)a;
}

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newfunc(int functype, struct ast *l) {
    struct fncall *a = malloc(sizeof(struct fncall));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    return (struct ast *)a;
}

struct ast *newcall(struct symbol *s, struct ast *l) {
    struct ufncall *a = malloc(sizeof(struct ufncall));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = 'C';
    a->l = l;
    a->s = s;
    return (struct ast *)a;
}

struct ast *newref(struct symbol *s) {
    struct symref *a = malloc(sizeof(struct symref));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = 'N';
    a->s = s;
    return (struct ast *)a;
}

struct ast *newasgn(struct symbol *s, struct ast *v) {
    struct symasgn *a = malloc(sizeof(struct symasgn));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = '=';
    a->s = s;
    a->v = v;
    return (struct ast *)a;
}

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el) {
    struct flow *a = malloc(sizeof(struct flow));
    if(!a) { yyerror("sem espaco"); exit(0); }
    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;
    return (struct ast *)a;
}

void treefree(struct ast *a) {
    if (!a) {
        return;
    }
    switch(a->nodetype) {
        /* duas subarvores */
        case '+': case '-': case '*': case '/': case 'L':
        case '&': case '|':
        case '1': case '2': case '3': case '4': case '5': case '6':
            treefree(a->r);
            treefree(a->l);
            break;
        /* uma subarvore */
        case 'C': case 'F':
            treefree(a->l);
        /* sem subarvore */
        case 'K': case 'N':
            break;
        case '=':
            treefree(((struct symasgn *)a)->v);
            break;
        /* acima de 3 subarvores */
        case 'I': case 'W':
            treefree(((struct flow *)a)->cond);
            treefree( ((struct flow *)a)->tl);
            treefree( ((struct flow *)a)->el);
            break;
        default: printf("erro interno: free bad node %c\n", a->nodetype);
    }
    free(a); /* sempre libera o proprio no */
}

struct symlist *newsymlist(struct symbol *sym, struct symlist *next) {
    struct symlist *sl = malloc(sizeof(struct symlist));
    if(!sl) { yyerror("sem espaco"); exit(0); }
    sl->sym = sym;
    sl->next = next;
    return sl;
}

void symlistfree(struct symlist *sl) {
    struct symlist *nsl;
    while(sl) {
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct fncall *f) {
    enum bifs functype = f->functype;
    double v = eval(f->l);
    switch(functype) {
        case B_sqrt: return sqrt(v);
        case B_exp: return exp(v);
        case B_log: return log(v);
        case B_print: fprintf(yyout, "= %4.4g\n", v); return v;
        default: yyerror("Funcao pre-definida desconhecida"); return 0.0;
    }
}

static double calluser(struct ufncall *f) {
    struct symbol *fn = f->s; /* nome da funcao */
    struct symlist *sl; /* argumentos da funcao */
    struct ast *args = f->l; /* argumentos usados na chamada */
    double *oldval, *newval; /* para salvar valores de argumentos do escopo global */
    double v;
    int nargs;
    int i;

    if(!fn->func) {
        yyerror("chamada para funcao indefinida");
        return 0;
    }

    /* contar argumentos */
    sl = fn->syms;
    for(nargs = 0; sl; sl = sl->next) nargs++;

    /* prepara para salvar argumentos */
    oldval = (double *)malloc(nargs * sizeof(double));
    newval = (double *)malloc(nargs * sizeof(double));
    if(!oldval || !newval) {
        yyerror("Sem espaco em %s", fn->name); return 0.0;
    }

    /* avaliacao de argumentos passados na chamada */
    for(i = 0; i < nargs; i++) {
        if(!args) {
            yyerror("poucos argumentos na chamada da funcao %s", fn->name);
            free(oldval); free(newval);
            return 0.0;
        }
        if(args->nodetype == 'L') { /* se eh uma lista de nos */
            newval[i] = eval(args->l);
            args = args->r;
        } else { /* se eh o final da lista (ultimo arg) */
            newval[i] = eval(args);
            args = NULL;
        }
    }

    /* salvar valores originais e injetar os valores dos argumentos chamados no escopo */
    sl = fn->syms;
    for(i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }
    free(newval);

    /* avaliacao real do corpo da funcao */
    v = eval(fn->func);

    /* recolocar os valores originais globais das variaveis */
    sl = fn->syms;
    for(i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        s->value = oldval[i];
        sl = sl->next;
    }
    free(oldval);
    return v;
}

void dodef(struct symbol *name, struct symlist *syms, struct ast *func) {
    if(name->syms) symlistfree(name->syms);
    if(name->func) treefree(name->func);
    name->syms = syms;
    name->func = func;
}

double eval(struct ast *a) {
    double v;
    if(!a) {
        yyerror("erro interno, null eval");
        return 0.0;
    }

    switch(a->nodetype) {
        /* constante */
        case 'K': v = ((struct numval *)a)->number; break;
        /* referencia de nome */
        case 'N': v = ((struct symref *)a)->s->value; break;
        /* atribuicao */
        case '=': v = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v); break;

        /* expressoes aritimeticas */
        case '+': v = eval(a->l) + eval(a->r); break;
        case '-': v = eval(a->l) - eval(a->r); break;
        case '*': v = eval(a->l) * eval(a->r); break;
        case '/': v = eval(a->l) / eval(a->r); break;

        /* comparacoes */
        case '1': v = (eval(a->l) > eval(a->r)) ? 1 : 0; break;
        case '2': v = (eval(a->l) < eval(a->r)) ? 1 : 0; break;
        case '3': v = (eval(a->l) != eval(a->r)) ? 1 : 0; break;
        case '4': v = (eval(a->l) == eval(a->r)) ? 1 : 0; break;
        case '5': v = (eval(a->l) >= eval(a->r)) ? 1 : 0; break;
        case '6': v = (eval(a->l) <= eval(a->r)) ? 1 : 0; break;

        case '&': v = (eval(a->l) != 0 && eval(a->r) != 0) ? 1 : 0; break;
        case '|': v = (eval(a->l) != 0 || eval(a->r) != 0) ? 1 : 0; break;

        /* if/then/else */
        case 'I':
            if(eval(((struct flow *)a)->cond) != 0) {
                if(((struct flow *)a)->tl) {
                    v = eval(((struct flow *)a)->tl);
                } else v = 0.0;
            } else {
                if(((struct flow *)a)->el) {
                    v = eval(((struct flow *)a)->el);
                } else v = 0.0;
            }
            break;

        /* while/do */
        case 'W':
            v = 0.0;
            if(((struct flow *)a)->tl) {
                while(eval(((struct flow *)a)->cond) != 0) {
                    v = eval(((struct flow *)a)->tl);
                }
            }
            break;

        /* lista de comandos (L) */
        case 'L': eval(a->l); v = eval(a->r); break;

        /* funcoes */
        case 'F': v = callbuiltin((struct fncall *)a); break;
        case 'C': v = calluser((struct ufncall *)a); break;

        default: printf("erro interno: bad node %c\n", a->nodetype);
    }
    return v;
}

void yyerror(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


int main(int argc, char **argv) {
    if(argc > 1){
        FILE * arquivoEntrada = fopen(argv[1], "r");
        if(!arquivoEntrada){
            printf("Arquivo não encontrado\n");
            return 1;
        }
        yyin = arquivoEntrada;
    }
    //Segundo parâmetro é o arquivo de saída
    if(argc > 2){
        FILE * arquivoSaida = fopen(argv[2], "w");
        if(!arquivoSaida){
            printf("Arquivo não encontrado\n");
            return 1;
        }
        yyin = arquivoSaida;
    } else {
        yyout = stdout; 
        printf("> ");
    }

    return yyparse();
}