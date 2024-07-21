/*
* helper functions for fb3-2
*/
#  include <stdio.h>
#  include <stdlib.h>
#  include <stdarg.h>
#  include <string.h>
#  include <math.h>
#  include "fb3-2.h"
/* symbol table */
struct symbol symtab[NHASH];
/* hash a symbol */
static unsigned
symhash(char *sym)
{
 unsigned int hash = 0;
 unsigned c;
 while(c = *sym++) hash = hash*9 ^ c;
 return hash;
}
struct symbol *
lookup(char* sym)
{
 struct symbol *sp = &symtab[symhash(sym)%NHASH];
 int scount = NHASH;           /* how many have we looked at */
 while(--scount >= 0) {
   if(sp->name && !strcmp(sp->name, sym)) { return sp; }
   if(!sp->name) {             /* new entry */
     sp->name = strdup(sym);
     sp->value = 0;
     sp->func = NULL;
     sp->syms = NULL;
     return sp;
   }
   if(++sp >= symtab+NHASH) sp = symtab; /* try the next entry */
 }
 yyerror("symbol table overflow\n");
 abort(); /* tried them all, table is full */
}
struct ast *
newast(int nodetype, struct ast *l, struct ast *r)
{
 struct ast *a = malloc(sizeof(struct ast));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = nodetype;
 a->l = l;
 a->r = r;
 return a;
}
struct ast *
newnum(double d)
{
 struct numval *a = malloc(sizeof(struct numval));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = 'K';
 a->number = d;
 return (struct ast *)a;
}
struct ast *
newcmp(int cmptype, struct ast *l, struct ast *r)
{
 struct ast *a = malloc(sizeof(struct ast));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = '0' + cmptype;
 a->l = l;
 a->r = r;
 return a;
}
struct ast *
newfunc(int functype, struct ast *l)
{
 struct fncall *a = malloc(sizeof(struct fncall));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = 'F';
 a->l = l;
 a->functype = functype;
 return (struct ast *)a;
}
struct ast *
newcall(struct symbol *s, struct ast *l)
{
 struct ufncall *a = malloc(sizeof(struct ufncall));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = 'C';
 a->l = l;
 a->s = s;
 return (struct ast *)a;
}
struct ast *
newref(struct symbol *s)
{
 struct symref *a = malloc(sizeof(struct symref));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = 'N';
 a->s = s;
 return (struct ast *)a;
}
struct ast *
newasgn(struct symbol *s, struct ast *v)
{
 struct symasgn *a = malloc(sizeof(struct symasgn));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = '=';
 a->s = s;
 a->v = v;
 return (struct ast *)a;
}
struct ast *
newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el)
{
 struct flow *a = malloc(sizeof(struct flow));
 if(!a) {
   yyerror("out of space");
   exit(0);
 }
 a->nodetype = nodetype;
 a->cond = cond;
 a->tl = tl;
 a->el = el;
 return (struct ast *)a;
}
/* free a tree of ASTs */
void
treefree(struct ast *a)
{
 switch(a->nodetype) {
   /* two subtrees */
 case '+':
 case '-':
 case '*':
 case '/':
 case '1':  case '2':  case '3':  case '4':  case '5':  case '6':
 case 'L':
   treefree(a->r);
   /* one subtree */
 case '|':
 case 'M': case 'C': case 'F':
   treefree(a->l);
   /* no subtree */
 case 'K': case 'N':
   break;
 case '=':
   free( ((struct symasgn *)a)->v);
   break;
   /* up to three subtrees */
 case 'I': case 'W':
   free( ((struct flow *)a)->cond);
   if( ((struct flow *)a)->tl) treefree( ((struct flow *)a)->tl);
   if( ((struct flow *)a)->el) treefree( ((struct flow *)a)->el);
   break;
   default: printf("internal error: free bad node %c\n", a->nodetype);
 }       
 free(a); /* always free the node itself */
}
struct symlist *
newsymlist(struct symbol *sym, struct symlist *next)
{
 struct symlist *sl = malloc(sizeof(struct symlist));
 if(!sl) {
   yyerror("out of space");
   exit(0);
 }
 sl->sym = sym;
 sl->next = next;
 return sl;
}
/* free a list of symbols */
void
symlistfree(struct symlist *sl)
{
 struct symlist *nsl;
 while(sl) {
   nsl = sl->next;
   free(sl);
   sl = nsl;
 }
}
 static double callbuiltin(struct fncall *);
static double calluser(struct ufncall *);
double
eval(struct ast *a)
{
 double v;
 if(!a) {
   yyerror("internal error, null eval");
   return 0.0;
 }
 switch(a->nodetype) {
   /* constant */
 case 'K': v = ((struct numval *)a)->number; break;
   /* name reference */
 case 'N': v = ((struct symref *)a)->s->value; break;
   /* assignment */
 case '=': v = ((struct symasgn *)a)->s->value =
     eval(((struct symasgn *)a)->v); break;
   /* expressions */
 case '+': v = eval(a->l) + eval(a->r); break;
 case '-': v = eval(a->l) - eval(a->r); break;
 case '*': v = eval(a->l) * eval(a->r); break;
 case '/': v = eval(a->l) / eval(a->r); break;
 case '|': v = fabs(eval(a->l)); break;
 case 'M': v = -eval(a->l); break;
   /* comparisons */
 case '1': v = (eval(a->l) > eval(a->r))? 1 : 0; break;
 case '2': v = (eval(a->l) < eval(a->r))? 1 : 0; break;
 case '3': v = (eval(a->l) != eval(a->r))? 1 : 0; break;
 case '4': v = (eval(a->l) == eval(a->r))? 1 : 0; break;
 case '5': v = (eval(a->l) >= eval(a->r))? 1 : 0; break;
 case '6': v = (eval(a->l) <= eval(a->r))? 1 : 0; break;
 /* control flow */
 /* null expressions allowed in the grammar, so check for them */
 /* if/then/else */
 case 'I': 
   if( eval( ((struct flow *)a)->cond) != 0) { //check the condition
     if( ((struct flow *)a)->tl) {             
       v = eval( ((struct flow *)a)->tl);
     } else
       v = 0.0;                /* a default value */
   } else {
     if( ((struct flow *)a)->el) {             
       v = eval(((struct flow *)a)->el);
     } else
       v = 0.0;                /* a default value */
   }
   break;
 /* while/do */
 case 'W':
   v = 0.0;            /* a default value */
//the true branch
//the false branch
   if( ((struct flow *)a)->tl) {
     while( eval(((struct flow *)a)->cond) != 0) //evaluate the condition
      v = eval(((struct flow *)a)->tl);         
//evaluate the target statements
   }
   break;                      /* value of last statement is value of while/do */
 /* list of statements */
 case 'L': eval(a->l); v = eval(a->r); break;
 case 'F': v = callbuiltin((struct fncall *)a); break;
 case 'C': v = calluser((struct ufncall *)a); break;
 default: printf("internal error: bad node %c\n", a->nodetype);
 }
 return v;
}
 static double
callbuiltin(struct fncall *f)
{
 enum bifs functype = f->functype;
 double v = eval(f->l);
switch(functype) {
case B_sqrt:
  return sqrt(v);
case B_exp:
  return exp(v);
case B_log:
  return log(v);
case B_print:
  printf("= %4.4g\n", v);
  return v;
default:
  yyerror("Unknown built-in function %d", functype);
  return 0.0;
}
}
/* define a function */
void
dodef(struct symbol *name, struct symlist *syms, struct ast *func)
{
 if(name->syms) symlistfree(name->syms);
 if(name->func) treefree(name->func);
 name->syms = syms;
 name->func = func;
}

void yyerror(char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}
int main()
{
    printf("> ");
    return yyparse();
}