#include <system.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Estrutura para Registros do endereço do flag INDOS retornado pela função 0x34 */
typedef struct registros {
	unsigned int bx1, es1;
} regis;

/* Union da estrutura Regis para uma variável de 2 bytes (char) */
typedef union k {
	regis x;
	char far *y;
} APONTA_REG_CRIT;


/* Estrutura do Descritor de Processo (BCP) */
typedef struct desc_p {
	char nome[35];
	enum {ativo, terminado} estado;
	PTR_DESC contexto;
	unsigned int prioridade; 
	struct desc_p *prox_desc;
} DESCRITOR_PROC;

/* ponteiro do tipo DESCRITOR_PROC */
typedef DESCRITOR_PROC *PTR_DESC_PROC;

/* Estrutura auxiliar para filas de prioridade */
typedef struct {
    PTR_DESC_PROC inicio; /* Cabeça da lista */
    PTR_DESC_PROC fim; /* Final da lista */
} FILA_PRIORIDADE;

extern void far iniciar_filas_processos();

extern PTR_DESC_PROC far prox_processo_nivel();

extern void far mudar_nivel_prioridade();

extern PTR_DESC_PROC far procura_proximo_ativo();

extern char* estado_processo(PTR_DESC_PROC p);

extern void far imprime_fila_processos();

extern void far volta_dos();

extern void far insere_fila_prontos(PTR_DESC_PROC p, int nivel);

extern void far cria_processo(void far(*end_proc)(), char nome_proc[35], unsigned int prioridade);

extern void far informacoes_processo(char nome_processo[], unsigned int *prioridade);

extern void far escalador();

extern void far dispara_sistema();

extern void far termina_processo();
