#include "C:\NBT\nucleo.h"

/* variáveis globais */
APONTA_REG_CRIT a; /* cria variável a para acesso ao flag INDOS */
PTR_DESC_PROC prim = NULL; /* cabeça da fila dos processos prontos */
PTR_DESC_PROC p_salva;

PTR_DESC d_esc; /* ponteiro para o descritor da co-rotina do escalador */

void far inicia_semaforo(semaforo *sem, unsigned int n){
	sem->s = n;
	sem->Q = NULL;
}
void far insere_fila_bloqueados(semaforo *sem){	
	PTR_DESC_PROC p_aux;

	if (!sem->Q){
		sem->Q = prim;
		sem->Q->fila_sem = NULL;
		return;
	}
	p_aux = sem->Q;

	while (p_aux->fila_sem != NULL){
		p_aux = p_aux->fila_sem;
	}

	p_aux->fila_sem = prim;
	prim->fila_sem = NULL;
}

void far remove_fila_bloqueados(semaforo *sem){
	PTR_DESC_PROC p_aux;
	sem->Q->estado = ativo;
	p_aux = sem->Q;
	sem->Q = sem->Q->fila_sem;
	p_aux->fila_sem = NULL;
}

PTR_DESC_PROC far procura_proximo_ativo(){
	PTR_DESC_PROC p_aux;

	/* começa busca pelo próximo processo a partir do próximo descritor de processos apontador por prim */	
	p_aux = prim->prox_desc;

	while (p_aux->estado != ativo && p_aux->prox_desc != prim->prox_desc){
		p_aux = p_aux->prox_desc;
	}

	if (p_aux->prox_desc == prim->prox_desc && prim->estado != ativo){
		p_salva = prim;
		return NULL;
	}
	return p_aux;
}

char * estado_processo(PTR_DESC_PROC p) {
	switch(p->estado) {
		case ativo:
			return "ativo";
		case bloq_P:
			return "bloq_P";
		case bloq_Env:
			return "bloq_Env";
		case bloq_Rec:
			return "bloq_Rec";
		case terminado:
			return "terminado";
		default:
			return "erro";
	}
}

void far imprime_fila_processos(){
	PTR_DESC_PROC p_aux;
	p_aux = p_salva->prox_desc;
	do {
		printf("Nome: %s\t Estado: ", p_aux->nome);
		printf("%s\n", estado_processo(p_aux));
		p_aux = p_aux->prox_desc;
	} while (p_aux != p_salva->prox_desc);
}

void far imprime_fila_sem(PTR_DESC_PROC Q){
	PTR_DESC_PROC p = Q;
	while (p != NULL) {
		p = p->fila_sem;
	}
}

void far volta_dos(){
	disable();
	setvect(8, p_est->int_anterior);
	enable();
	imprime_fila_processos();
	exit(0);
}

void far P(semaforo *sem){
	PTR_DESC_PROC p_aux;
	disable();
	if (sem->s > 0){
		(sem->s)--;
		enable();
	} else {
		prim->estado = bloq_P;
		p_aux = prim;
		insere_fila_bloqueados(sem);
		prim = procura_proximo_ativo();
		if (prim == NULL)
			volta_dos();
		transfer(p_aux->contexto, prim->contexto);
	}
}

void far V(semaforo *sem){
	disable();
	if (sem->Q){
		remove_fila_bloqueados(sem);
	} else {
		(sem->s)++;
	}
	enable();
}

void far insere_fila_prontos(PTR_DESC_PROC p){	
	PTR_DESC_PROC q;
	if (!prim){
		prim = p;
		prim->prox_desc = prim;
		return;
	}
	
	q = prim;

	while (q->prox_desc != prim){
		q = q->prox_desc;
	}

	q->prox_desc = p;
	p->prox_desc = prim;
}

/* cria processo e adiciona na fila de processos prontos (por enquanto)
 |end_proc|  endereço de localização na memória do processo;
 |nome_proc| nome dado pelo usuário para identificação do processo */
void far cria_processo(void far(*end_proc)(), char nome_proc[35], int tamanho){
	PTR_DESC_PROC p_aux;
	int i;
	p_aux = (DESCRITOR_PROC*) malloc(sizeof (DESCRITOR_PROC));;
	if (p_aux == NULL){
		printf("\nMemória insuficiente para alocação de descritor\n");
		exit(1);
	}

	/* inicialização dos campos do descritor */
	strcpy(p_aux->nome, nome_proc);
	p_aux->estado = ativo;
	p_aux->contexto = cria_desc();
	newprocess(end_proc, p_aux->contexto);

	if (tamanho < 1){
		tamanho = 1;
	} else if (tamanho > 10) {
		tamanho = 10;
	}

	if ((p_aux->vet_msg = (mensagem*) malloc (tamanho * sizeof(mensagem))) == NULL){
		printf("\nMemória insuficiente para alocação de fila de mensagens\n");
		exit(1);
	}

	for (i = 0; i < tamanho; i++) {
		p_aux->vet_msg[i].flag = vazia;
	}

	p_aux->tam_msg = tamanho;
	p_aux->qtde_msg_recebidas = 0;

	insere_fila_prontos(p_aux);
}

void far escalador(){
	int i;
	p_est->p_origem = d_esc;
	p_est->p_destino = prim->contexto;
	p_est->num_vetor = 8;

	/* inicia ponteiro para Região Crítica do DOS */
	_AH = 0x34;
	_AL = 0x00;
	geninterrupt(0x21);
	a.x.bx1 = _BX;
	a.x.es1 = _ES;

	while(1){
		iotransfer();
		disable();
		
		/*  verifica se processo ainda está usando algum recurso do DOS (chamada ao DOS).
			Caso verdadeiro, processo está na R.C, portanto não escalona outro processo.
			Se falso, processo não está na R.C, portanto escalona outro processo normalmente.
		*/
		if (*a.y == 0) {
			prim = procura_proximo_ativo();
			if (prim == NULL)
				volta_dos();
			p_est->p_destino = prim->contexto;
		}
		enable();
	}
}

void far dispara_sistema(){
	PTR_DESC desc_dispara;
	d_esc = cria_desc();
	desc_dispara = cria_desc();
	newprocess(escalador, d_esc);
	transfer(desc_dispara, d_esc);
}

/* termina_processo para quando se deseja salvar a fatia de tempo restante que o processo teria caso não tivesse terminado
void far termina_processo(){
	PTR_DESC_PROC p_aux;
	disable();
	prim->estado = terminado;
	p_aux = prim;
	prim = procura_proximo_ativo();
	if (prim == NULL)
		volta_dos();
	transfer(p_aux->contexto, prim->contexto);
}*/

/* termina_processo que ignora fatia de tempo perdida pelo processo */
void far termina_processo(){
	disable();
	prim->estado = terminado;
	enable();
	while(1);
}

PTR_DESC_PROC far procura_processo_fila_descritores(char *nome_proc){
	PTR_DESC_PROC p_aux = prim->prox_desc;

	while (p_aux != prim) {
		if (strcmp(p_aux->nome, nome_proc) == 0 && p_aux->estado != terminado){
			return p_aux;
		}
		p_aux = p_aux->prox_desc;
	}

	return NULL;
}

int far envia(char* msg, char* receptor){
	PTR_DESC_PROC p_rec, p_aux;
	int i; 

	/*  Opções de retorno para envio de mensagens.
		'proc_inacessivel' é quando o processo não existe ou está terminado. */
	enum retorno_mensagens{sucesso = 0, proc_inacessivel = 1, fila_cheia = 2};

	disable();
	p_rec = procura_processo_fila_descritores(receptor);
	if (p_rec == NULL){
		enable();
		return proc_inacessivel; /* fracasso - não encontrou processo receptor ou o estado do mesmo está diferente de terminado */
	} 

	if (p_rec->qtde_msg_recebidas == p_rec->tam_msg){
		enable();
		return fila_cheia; /* fracasso - fila de mensagens cheia */
	}

	/* envia mensagem */
	i = 0;
	while (p_rec->vet_msg[i].flag == nova){
		i++;
	}

	p_rec->vet_msg[i].flag = nova;
	strcpy(p_rec->vet_msg[i].nome_emissor, prim->nome);
	strcpy(p_rec->vet_msg[i].msg, msg);
	
	p_rec->qtde_msg_recebidas++;

	if (p_rec->estado == bloq_Rec){
		p_rec->estado = ativo;
	}

	prim->estado = bloq_Env;
	p_aux = prim;

	prim = procura_proximo_ativo();

	if (prim == NULL)
		volta_dos();

	transfer(p_aux->contexto, prim->contexto);
	return sucesso; /* sucesso - mensagem foi enviada */
}

/* Recebe Não Seletivo */
void far recebe(char* msg, char* emissor){
	PTR_DESC_PROC p_aux;
	int i;
	disable();
	if (prim->qtde_msg_recebidas == 0) {
		prim->estado = bloq_Rec;
		p_aux = prim;
		prim = procura_proximo_ativo();
		if (prim == NULL)
			volta_dos();
		transfer(p_aux->contexto, prim->contexto);
		disable();
	}

	/* lê mensagem */
	i = 0;
	while (prim->vet_msg[i].flag == vazia) {
		i++;
	}

	prim->vet_msg[i].flag = vazia;
	strcpy(emissor, prim->vet_msg[i].nome_emissor);
	strcpy(msg, prim->vet_msg[i].msg);

	prim->qtde_msg_recebidas--;
	p_aux = procura_processo_fila_descritores(emissor);	
	p_aux->estado = ativo;
	enable();
}