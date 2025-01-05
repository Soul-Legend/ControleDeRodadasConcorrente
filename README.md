# ControleDeRodadasConcorrente
Código que faz o Controle e Simulação de Rodadas Grátis em um Bar utilizando programação concorrente POSIX PThreads.

Um bar resolveu liberar um número específico de rodadas grátis para seus N clientes presentes no
estabelecimento antes de fechar. Esse bar possui G garçons. Cada garçom consegue atender a um
número limitado Gn de clientes por vez e esta capacidade é igual para todos os garçons. Cada garçom
somente vai para a copa para buscar os pedidos quando todos os Gn clientes que ele pode atender
tiverem feito pedido. Antes de fazer um novo pedido, cada cliente conversa com seus amigos durante
um tempo aleatório. Após ter seu pedido atendido, um cliente pode fazer um novo pedido após
consumir sua bebida (o que também leva um tempo aleatório). Uma nova rodada (R) somente pode
ocorrer quando foram atendidos todos os clientes que conseguiram fazer pedidos na rodada anterior.
Nem todos os clientes precisam pedir uma bebida a cada rodada. A simulação acaba quando o número
de rodadas R é atingido.

O código permite a passagem por parâmetro para o programa: (i) o número de
clientes presentes no estabelecimento (N); (ii) o número de garçons que estão trabalhando (G); (iii) a
capacidade de atendimento dos garçons (Gn); (iv) o número de rodadas grátis que serão liberadas no
bar (R); (v) tempo máximo (em milissegundos) que um cliente pode ficar conversando antes de fazer
um novo pedido; (vi) tempo máximo (em milissegundos) que um cliente fica consumindo a bebida.

## O programa recebe os parâmetros nessa ordem via linha de comando:
./programa <clientes> <garcons> <clientes/garcon> <rodadas> <max.conversa> <max.consumo>

Cada garçom e cada cliente é representado por threads.

## As seguintes regras devem ser respeitadas:

• Os pedidos dos clientes são atendidos pelos garçons em ordem de chegada na fila de pedidos
de cada garçom (a solução não deve permitir que clientes furem essa fila);

• O garçom só pode ir para a copa quando tiver recebido seus Gn pedidos;

• O programa deve mostrar a evolução da simulação, portanto planeje bem o que será
apresentado. Deve ficar claro o que está acontecendo no bar a cada rodada. Os pedidos dos
clientes, os atendimentos pelos garçons, os deslocamentos para o pedido, a garantia de ordem
de atendimento, etc.
