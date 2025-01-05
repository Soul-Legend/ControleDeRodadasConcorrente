#include "cQueue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Definições globais -----------------------------------------------
struct Cliente {
  int id;
  pthread_t thread;
};

struct Garcom {
  int id;
  Queue_t fila_garcom;
  pthread_t thread;
};

// Parâmetros do programa -------------------------------------------
int num_clientes;
int num_garcons;
int capacidade_garcom;
int num_rodadas;
int max_conversa;
int max_consumo;

// Variáveis globais ------------------------------------------------
int rodada_atual = 0;
bool esta_fechado = false;

// Usada para sincronizar os prints entre os clientes e os garçons.
int *cliente_atendido_por = NULL;

bool *garcom_pode_atender = NULL;
int *garcom_chamado_por = NULL;

// Semáforos binários usados para sincronização.
sem_t *clientes_sem = NULL;
sem_t *garcons_sem = NULL;

pthread_mutex_t num_garcons_entregaram_mutex;
pthread_mutex_t garcom_chamado_mutex;

pthread_mutex_t *garcom_atendimento_mutex = NULL;

pthread_cond_t rodada_passou_cond;
pthread_mutex_t rodada_passou_mutex;

// Funções relativas ao cliente -------------------------------------

/// @brief Função responsável por simular o tempo de conversa
/// em milissegundos de um cliente. O tempo gasto é aleatório
/// até um máximo de `max_conversa` milissegundos.
/// @param id ID do cliente que irá conversar.
void conversaComAmigos(int id) {
  printf("[Cliente %3d] Começou a conversar.\n", id);
  fflush(stdout);

  usleep((rand() % max_conversa) * 1000);
}

/// @brief Função resposável por simular um pedido de um cliente.
/// Ao cliente realizar seu pedido, este é colocado na fila de pedidos
/// compartilhada entre os clientes e realizamos um `sem_post` no semáforos
/// de pedidos para liberar um garçom para atender esse pedido.
/// @param id ID do cliente que irá realizar um pedido.
/// @return verdadeiro se pedido do cliente pode ser atendido na rodada.
bool fazPedido(int id) {
  static int proximo_garcom = 0;

  // Atualiza a variável próximo garçom para o garçom seguinte.
  pthread_mutex_lock(&garcom_chamado_mutex);
  int garcom_chamado = proximo_garcom;
  proximo_garcom = (proximo_garcom + 1) % num_garcons;
  pthread_mutex_unlock(&garcom_chamado_mutex);

  pthread_mutex_lock(&garcom_atendimento_mutex[garcom_chamado]);

  if (!garcom_pode_atender[garcom_chamado]) {
    pthread_mutex_unlock(&garcom_atendimento_mutex[garcom_chamado]);

    return false;
  }

  printf("[Cliente %3d] Fez um pedido.\n", id);
  fflush(stdout);

  garcom_chamado_por[garcom_chamado] = id;

  // Acorda o garçom.
  sem_post(&garcons_sem[garcom_chamado]);
  // Espera o garçom confirmar o pedido.
  sem_wait(&clientes_sem[id]);

  pthread_mutex_unlock(&garcom_atendimento_mutex[garcom_chamado]);

  return true;
}

/// @brief Função responsável por simular o tempo de espera de
/// um cliente pelo seu pedido.
/// @param id ID do cliente que irá esperar.
void esperaPedido(int id) {
  printf("[Cliente %3d] Está esperando.\n", id);
  fflush(stdout);

  // Libera o garçom para atender outros clientes.
  sem_post(&garcons_sem[cliente_atendido_por[id]]);
  // Espera o pedido ser entregue.
  sem_wait(&clientes_sem[id]);
}

/// @brief Função resposável por simular o recebimento do pedido.
/// @param id ID do cliente que irá receber o pedido.
void recebePedido(int id) {
  printf("[Cliente %3d] Recebeu o seu pedido.\n", id);
  fflush(stdout);

  // Sinaliza para o garçom atendente que o cliente recebeu o seu
  // pedido.
  sem_post(&garcons_sem[cliente_atendido_por[id]]);
}

/// @brief Função responsável por simular o tempo de consumo
/// em milissegundos de um cliente. O tempo gasto é aleatório
/// até um máximo de `max_consumo` milissegundos.
/// @param id ID do cliente que irá consumir.
void consomePedido(int id) {
  printf("[Cliente %3d] Começou a consumir.\n", id);
  fflush(stdout);

  usleep((rand() % max_consumo) * 1000);
}

void *cliente_worker(void *arg) {
  struct Cliente dados = *(struct Cliente *)arg;

  while (!esta_fechado) {
    int rodada_cliente = rodada_atual;

    conversaComAmigos(dados.id);

    bool conseguiu_fazer_pedido = fazPedido(dados.id);

    if (conseguiu_fazer_pedido) {
      esperaPedido(dados.id);
      recebePedido(dados.id);
      consomePedido(dados.id);
    }

    // Sincroniza o cliente com o contador de rodadas.
    pthread_mutex_lock(&rodada_passou_mutex);
    ++rodada_cliente;
    if (rodada_cliente > rodada_atual) {
      // O cliente espera até a próxima rodada para fazer pedido de novo.
      pthread_cond_wait(&rodada_passou_cond, &rodada_passou_mutex);
    }
    pthread_mutex_unlock(&rodada_passou_mutex);
  }

  return NULL;
}

// Funções relativas aos garçons ------------------------------------

/// @brief Função responsável por simular o recebimento de pedidos
/// de um garçom. Ao garçom receber o máximo de pedidos, retiramos
/// da fila de pedidos compartilhada entre os clientes uma quantidade
/// limitada de ´capacidade_garcom´ pedidos.
/// @param id ID do garçom que irá receber o máximo de pedidos.
/// @param fila_garcom Fila de pedidos do garçom.
void recebeMaximoPedidos(int id, Queue_t *fila_garcom) {
  printf("[Garçom  %3d] Começou a receber pedidos.\n", id);
  fflush(stdout);

  for (int i = 0; i < capacidade_garcom; ++i) {
    // Espera alguém fazer um pedido para esse garçom.
    sem_wait(&garcons_sem[id]);

    // Garçom não pode mais receber pedidos nessa rodada.
    if (i == capacidade_garcom - 1) {
      garcom_pode_atender[id] = false;
    }

    int cliente_id = garcom_chamado_por[id];
    cliente_atendido_por[cliente_id] = id;

    q_push(fila_garcom, &cliente_id);

    printf("[Garçom  %3d] Pedido do cliente %3d foi recebido.\n", id,
           cliente_id);
    fflush(stdout);

    // Informa o cliente que o pedido foi confirmado.
    sem_post(&clientes_sem[cliente_id]);
    // Espera o cliente executar esperaPedido, para os logs estarem
    // sincronizados.
    sem_wait(&garcons_sem[id]);
  }

  printf("[Garçom  %3d] Recebeu o máximo de pedidos.\n", id);
  fflush(stdout);
}

/// @brief Função responsável por simular o registro dos pedidos
/// de um garçom.
/// @param id ID do garçom que irá registrar seus pedidos.
void registraPedidos(int id) {
  printf("[Garçom  %3d] Foi para a copa.\n", id);
  fflush(stdout);
}

/// @brief Função responsável por simular a entrega dos pedidos
/// de um garçom. Ao garçom entregar seus pedidos, esvaziamos
/// sua fila interna de pedidos e liberamos os semáforos dos
/// clientes em espera.
/// @param id ID do garçom que irá entregar seus pedidos
/// @param fila_garcom Fila de pedidos do garçom.
void entregaPedidos(int id, Queue_t *fila_garcom) {
  while (!q_isEmpty(fila_garcom)) {
    int cliente_id;

    q_pop(fila_garcom, &cliente_id);

    printf("[Garçom  %3d] Entregou para %3d.\n", id, cliente_id);
    fflush(stdout);

    // Termina a espera do cliente pelo pedido.
    sem_post(&clientes_sem[cliente_id]);
    // Garçom espera cliente receber o pedido antes de entregar para o próximo.
    sem_wait(&garcons_sem[id]);
  }
}

/// @brief Função responsável por iniciar uma nova rodada na simulação. Também é
/// responsável por verificar se o número de rodadas foi atingido e por acordar
/// clientes e garçons.
void iniciarNovaRodada(void) {
  // Verifica se essa foi a última rodada.
  esta_fechado = rodada_atual >= (num_rodadas - 1);

  if (!esta_fechado) {
    printf("\n[BAR] *** Início da rodada %d. ***\n\n", rodada_atual + 2);
    fflush(stdout);
  }

  // Acorda os garçons.
  garcom_pode_atender[0] = !esta_fechado;
  for (int i = 1; i < num_garcons; ++i) {
    garcom_pode_atender[i] = !esta_fechado;

    sem_post(&garcons_sem[i]);
  }

  // Incrementa a rodada e acorda os clientes que estão esperando a rodada
  // passar.
  pthread_mutex_lock(&rodada_passou_mutex);
  ++rodada_atual;
  pthread_cond_broadcast(&rodada_passou_cond);
  pthread_mutex_unlock(&rodada_passou_mutex);
}

void *garcom_worker(void *arg) {
  static int num_garcons_entregaram = 0;

  struct Garcom dados = *(struct Garcom *)arg;

  while (!esta_fechado) {
    recebeMaximoPedidos(dados.id, &dados.fila_garcom);
    registraPedidos(dados.id);
    entregaPedidos(dados.id, &dados.fila_garcom);

    // O último garçom a terminar de entregar seus pedidos deve sinalizar o
    // garçom zero para o início de uma nova rodada.
    pthread_mutex_lock(&num_garcons_entregaram_mutex);
    ++num_garcons_entregaram;
    if (num_garcons_entregaram == num_garcons) {
      sem_post(&garcons_sem[0]);
    }
    pthread_mutex_unlock(&num_garcons_entregaram_mutex);

    // Após atender a sua capacidade máxima de pedidos, o garçom deve esperar
    // até uma nova rodada começar.
    sem_wait(&garcons_sem[dados.id]);

    // O garçom zero age como chefe de mesa e é responsável por incrementar a
    // rodada e acordar os outros garçons.
    if (dados.id == 0) {
      num_garcons_entregaram = 0;

      iniciarNovaRodada();
    }
  }

  return NULL;
}

// Funções relativas ao bar -----------------------------------------

/// @brief Inicializa o "Bar". Faz a alocação de memória para as
/// variáveis utilizadas e inicializa os mutexes e semáforos.
void inicializarBar(void) {
  pthread_mutex_init(&num_garcons_entregaram_mutex, NULL);
  pthread_mutex_init(&garcom_chamado_mutex, NULL);

  pthread_mutex_init(&rodada_passou_mutex, NULL);
  pthread_cond_init(&rodada_passou_cond, NULL);

  clientes_sem = calloc(num_clientes, sizeof(sem_t));
  garcons_sem = calloc(num_garcons, sizeof(sem_t));

  cliente_atendido_por = calloc(num_clientes, sizeof(int));
  garcom_chamado_por = calloc(num_garcons, sizeof(int));

  garcom_pode_atender = calloc(num_garcons, sizeof(bool));

  garcom_atendimento_mutex = calloc(num_garcons, sizeof(pthread_mutex_t));
  for (int i = 0; i < num_garcons; ++i) {
    pthread_mutex_init(&garcom_atendimento_mutex[i], NULL);
  }

  printf("[BAR] *** Início da rodada 1. ***\n\n");
  fflush(stdout);
}

/// @brief Fecha o "Bar". Destrói os mutexes utilizados no código,
/// libera memória alocada.
void fecharBar(void) {
  pthread_mutex_destroy(&num_garcons_entregaram_mutex);
  pthread_mutex_destroy(&garcom_chamado_mutex);

  pthread_mutex_destroy(&rodada_passou_mutex);
  pthread_cond_destroy(&rodada_passou_cond);

  for (int i = 0; i < num_garcons; ++i) {
    pthread_mutex_destroy(&garcom_atendimento_mutex[i]);
  }
  free(garcom_atendimento_mutex);

  free(garcom_pode_atender);

  free(clientes_sem);
  free(garcons_sem);

  free(cliente_atendido_por);
  free(garcom_chamado_por);

  printf("[BAR] Fechou!\n");
  fflush(stdout);
}

// ------------------------------------------------------------------

int main(int argc, const char *argv[]) {
  if (argc < 7) {
    fprintf(
        stderr,
        "Forneça os argumentos corretos:\n\nUso: ./programa <clientes> "
        "<garcons> <clientes/garcom> <rodadas> <max.conversa> <max.consumo>\n");

    return EXIT_FAILURE;
  }

  // Inicializa variáveis com valores recebidos pelo terminal.
  num_clientes = atoi(argv[1]);
  num_garcons = atoi(argv[2]);
  capacidade_garcom = atoi(argv[3]);
  num_rodadas = atoi(argv[4]);
  max_conversa = atoi(argv[5]);
  max_consumo = atoi(argv[6]);

  if (num_clientes <= 0 || num_garcons <= 0 || capacidade_garcom <= 0 ||
      num_rodadas <= 0 || max_conversa <= 0 || max_consumo <= 0) {
    fprintf(stderr,
            "Nenhum parâmetro pode assumir valor menor ou igual à zero.\n");

    return EXIT_FAILURE;
  }

  // Certifica-se de que o número de clientes é suficiente para todos os garçons
  // atenderem em uma rodada.
  if (num_clientes < num_garcons * capacidade_garcom) {
    fprintf(stderr, "O número de clientes (N) não deve ser menor que o número "
                    "de garçons (G) multiplicado pela capacidade de atendimento"
                    " (Gn).\n");

    return EXIT_FAILURE;
  }

  // Inicializa função rand com uma semente diferente a cada execução.
  srand(time(NULL));

  inicializarBar();

  struct Garcom garcons[num_garcons];

  for (int i = 0; i < num_garcons; ++i) {
    sem_init(&garcons_sem[i], 0, 0);

    garcom_pode_atender[i] = true;

    garcons[i].id = i;

    q_init(&garcons[i].fila_garcom, sizeof(int), capacidade_garcom, FIFO,
           false);

    pthread_create(&garcons[i].thread, NULL, garcom_worker, &garcons[i]);
  }

  struct Cliente clientes[num_clientes];

  for (int i = 0; i < num_clientes; ++i) {
    sem_init(&clientes_sem[i], 0, 0);

    clientes[i].id = i;

    pthread_create(&clientes[i].thread, NULL, cliente_worker, &clientes[i]);
  }

  for (int i = 0; i < num_clientes; ++i) {
    pthread_join(clientes[i].thread, NULL);

    sem_destroy(&clientes_sem[i]);
  }

  printf("\n[BAR] *** Todos os clientes foram para casa. ***\n\n");
  fflush(stdout);

  for (int i = 0; i < num_garcons; ++i) {
    pthread_join(garcons[i].thread, NULL);

    sem_destroy(&garcons_sem[i]);
    q_kill(&garcons[i].fila_garcom);
  }

  printf("[BAR] *** Todos os garçons foram para casa. ***\n\n");
  fflush(stdout);

  fecharBar();

  return EXIT_SUCCESS;
}
