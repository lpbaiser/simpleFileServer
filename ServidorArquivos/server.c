#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "connection.h"
#include "dynamicList.h"
#include <pthread.h>
#include <semaphore.h>
#include "workerThread.h"

#define BUFFER_SIZE 2048
List *lista;
pthread_mutex_t mutexAddLista;
sem_t full;
pthread_t *threads;

void* echoThread(void* args) {

    connection_t* connection;

    connection = (connection_t*) args;

    //Buffer usado para receber e enviar echo
    uint8_t buffer[BUFFER_SIZE];

    //Enviar uma mensagem de boas-vindas ao cliente
    const char* hello = "Servidor Echo v0.01 -- Bem Vindo(a)!";
    CONN_send(connection, (void*) hello, strlen(hello), 0);

    //A idéia de um servidor echo é simplesmente mandar de volta a mensagem recebida
    while (1) {
        //aguardar a mensagem
        CONN_receive(connection, buffer, BUFFER_SIZE, 0);

        //imprimir a mensagem recebida e o tamanho da mensagem
        printf("recebi '%s' do cliente (%s:%s)... (len = %d)\n",
                buffer,
                CONN_getPeerName(connection),
                CONN_getPeerPort(connection),
                strlen(buffer));

        //enviar de volta a mensagem recebida.
        CONN_send(connection, buffer, strlen(buffer) + 1, 0);

        //detectar se a mensagem "sair" foi enviada.. se sim, fechar o servidor
        if (!strcmp(buffer, "sair")) {
            break;
        }
    }

    //fechar a conexão
    CONN_close(connection);

}

void *dispatcher(connection_t* connection, int listenSock, char* port) {

    while (1) {//dispatcher
        //Aceitar conexões. Esta função retorna uma conexão (usando um novo socket) ou NULL.
        //listenSocket ainda pode ser chamado para aceitar outras conexões!
        connection = CONN_accept(listenSock);

        //Verificar se houve sucesso ao conectar
        if (!connection) {
            fprintf(stderr, "Não foi possível conectar ao cliente remoto na porta %s!\n", port);
            return (EXIT_FAILURE);
        }

        pthread_t* t;
        t = malloc(sizeof (pthread_t));

            pthread_create(t, NULL,(void*)aguardaRequisicao, (void*) connection);
    }
}

int main(int argc, char** argv) {
    //init
    pthread_mutex_init(&mutexAddLista,NULL);
    sem_init(&full, 0, 0);
    //Socket usado para aguardar a conexão
    int listenSock;

    //Conexão ao cliente
    connection_t* connection;

    //Porta a ser usada pelo servidor... (use portas altas (10000+) para evitar conflitos com serviços já sendo executados)
    char* port;
    int nThreads;
    //Verficar se a porta foi passada como argumento
    if (argc < 2) {
        fprintf(stderr, "uso: %s porta\n", argv[0]);
        return EXIT_FAILURE;
    }

    //porta é o primeiro argumento
    port = argv[1];
    nThreads = atoi(argv[2]);
    
    int i;
    for (i = 0; i < nThreads; i++) {

        pthread_create(&threads[i], NULL, wakeThread, NULL);
    }

    //Abrir socket para arguardar conexões
    listenSock = CONN_listenTo((char*) port);

    //criando a lista
    lista = createList();
    initializeList(lista);

    dispatcher(connection, listenSock, port);
    //fechar o socket
    close(listenSock);

    return EXIT_SUCCESS;

}

