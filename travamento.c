#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int barramento_ocupado;
    pthread_mutex_t mutex;
    pthread_cond_t cond; 
} Monitor;

Monitor monitor_barramento;

// Variável que simula a existência de apenas 1 processador (como na sonda)
int cpu_monopolizada = 0; 

void registrar_log(const char* nome_thread, const char* acao) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); 
    printf("[%ld.%09ld] [%s]: %s\n", ts.tv_sec, ts.tv_nsec, nome_thread, acao);
}

void exibir_tempo_total(const char* nome_thread, struct timespec inicio) {
    struct timespec fim;
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_gasto = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    printf("\n>>> [RESULTADO] A %s levou %.4f segundos do inicio ao fim. <<<\n\n", nome_thread, tempo_gasto);
}

void adquirir_barramento(const char* nome_thread) {
    pthread_mutex_lock(&monitor_barramento.mutex);
    while (monitor_barramento.barramento_ocupado == 1) {
        registrar_log(nome_thread, "Barramento ocupado. Entrando em estado bloqueado (Wait)...");
        pthread_cond_wait(&monitor_barramento.cond, &monitor_barramento.mutex);
    }
    monitor_barramento.barramento_ocupado = 1;
    registrar_log(nome_thread, "Adquiriu o barramento com sucesso.");
    pthread_mutex_unlock(&monitor_barramento.mutex);
}

void liberar_barramento(const char* nome_thread) {
    pthread_mutex_lock(&monitor_barramento.mutex);
    monitor_barramento.barramento_ocupado = 0;
    registrar_log(nome_thread, "Liberou o barramento.");
    pthread_cond_signal(&monitor_barramento.cond);
    pthread_mutex_unlock(&monitor_barramento.mutex);
}

void* thread_baixa(void* arg) {
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);
    registrar_log("Thread Baixa", "Iniciando...");
    adquirir_barramento("Thread Baixa");
    
    registrar_log("Thread Baixa", "Realizando escrita lenta no barramento (Dividida em etapas)...");
    
    // Simula a escrita lenta em 5 etapas
    for(int i = 0; i < 5; i++) {
        // Se a Thread Média tomou a CPU, a Baixa fica congelada aqui (Simulação de Preempção)
        while(cpu_monopolizada == 1) {
            usleep(50000); 
        }
        usleep(100000); // Trabalho real da etapa
    }
    
    liberar_barramento("Thread Baixa");
    registrar_log("Thread Baixa", "Finalizada.");
    exibir_tempo_total("Thread Baixa", inicio);
    return NULL;
}

void* thread_media(void* arg) {
    usleep(100000); // Espera a Baixa e a Alta começarem
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);

    registrar_log("Thread Media", "Iniciando. PREEMPTANDO a Baixa e Monopolizando a CPU...");
    cpu_monopolizada = 1; // Tira a CPU da Thread Baixa
    
    volatile long contador = 0;
    for(long i = 0; i < 2000000000L; i++) { contador++; }
    
    cpu_monopolizada = 0; // Devolve a CPU
    registrar_log("Thread Media", "Finalizou calculos intensivos. Devolveu a CPU.");
    exibir_tempo_total("Thread Media", inicio);
    return NULL;
}

void* thread_alta(void* arg) {
    usleep(50000); 
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);

    registrar_log("Thread Alta ", "Iniciando requisicao critica de tempo real...");
    adquirir_barramento("Thread Alta ");
    
    registrar_log("Thread Alta ", "Enviando dados...");
    usleep(100000); 
    liberar_barramento("Thread Alta ");
    
    registrar_log("Thread Alta ", "Finalizada.");
    exibir_tempo_total("Thread Alta ", inicio);
    return NULL;
}

int main() {
    pthread_t tb, tm, ta;
    monitor_barramento.barramento_ocupado = 0;
    pthread_mutex_init(&monitor_barramento.mutex, NULL);
    pthread_cond_init(&monitor_barramento.cond, NULL);
    
    printf("--- INICIANDO SIMULADOR MARS PATHFINDER (O TRAVAMENTO) ---\n\n");
    pthread_create(&tb, NULL, thread_baixa, NULL);
    pthread_create(&ta, NULL, thread_alta, NULL);
    pthread_create(&tm, NULL, thread_media, NULL);
    
    pthread_join(tb, NULL); pthread_join(ta, NULL); pthread_join(tm, NULL);
    return 0;
}