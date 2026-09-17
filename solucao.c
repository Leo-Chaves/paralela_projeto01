#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int barramento_ocupado;
    int prioridade_herdada; 
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Monitor;

Monitor monitor_barramento;
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

void adquirir_barramento(const char* nome_thread, int is_alta_prioridade) {
    pthread_mutex_lock(&monitor_barramento.mutex);
    while (monitor_barramento.barramento_ocupado == 1) {
        if (is_alta_prioridade) {
            registrar_log(nome_thread, "Barramento ocupado! Aplicando HERANCA DE PRIORIDADE na Thread Baixa.");
            monitor_barramento.prioridade_herdada = 1; 
        } else {
            registrar_log(nome_thread, "Barramento ocupado. Entrando em Wait...");
        }
        pthread_cond_wait(&monitor_barramento.cond, &monitor_barramento.mutex);
    }
    monitor_barramento.barramento_ocupado = 1;
    registrar_log(nome_thread, "Adquiriu o barramento com sucesso.");
    pthread_mutex_unlock(&monitor_barramento.mutex);
}

void liberar_barramento(const char* nome_thread) {
    pthread_mutex_lock(&monitor_barramento.mutex);
    monitor_barramento.barramento_ocupado = 0;
    monitor_barramento.prioridade_herdada = 0; 
    registrar_log(nome_thread, "Liberou o barramento e restaurou prioridade normal.");
    pthread_cond_signal(&monitor_barramento.cond);
    pthread_mutex_unlock(&monitor_barramento.mutex);
}

void* thread_baixa(void* arg) {
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);
    registrar_log("Thread Baixa", "Iniciando...");
    adquirir_barramento("Thread Baixa", 0);
    
    registrar_log("Thread Baixa", "Realizando operacoes no barramento (Dividida em etapas)...");
    for(int i = 0; i < 5; i++) {
        while(cpu_monopolizada == 1) { usleep(50000); }
        usleep(100000); 
    }
    
    liberar_barramento("Thread Baixa");
    registrar_log("Thread Baixa", "Finalizada.");
    exibir_tempo_total("Thread Baixa", inicio);
    return NULL;
}

void* thread_media(void* arg) {
    usleep(100000); 
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);

    registrar_log("Thread Media", "Iniciando. Tentando preemptar e monopolizar a CPU...");
    
    // O Escalonador barra a Thread Média por causa da Herança de Prioridade
    while(monitor_barramento.prioridade_herdada == 1) {
        registrar_log("Thread Media", "BLOQUEADA! Thread Baixa esta com prioridade ALTA.");
        usleep(150000); 
    }
    
    cpu_monopolizada = 1; 
    registrar_log("Thread Media", "Conseguiu CPU. Processamento intensivo iniciado...");
    volatile long contador = 0;
    for(long i = 0; i < 2000000000L; i++) { contador++; }
    
    cpu_monopolizada = 0;
    registrar_log("Thread Media", "Finalizou calculos intensivos.");
    exibir_tempo_total("Thread Media", inicio);
    return NULL;
}

void* thread_alta(void* arg) {
    usleep(50000); 
    struct timespec inicio; clock_gettime(CLOCK_MONOTONIC, &inicio);

    registrar_log("Thread Alta ", "Iniciando requisicao critica...");
    adquirir_barramento("Thread Alta ", 1); 
    
    registrar_log("Thread Alta ", "Enviando dados rapidamente...");
    usleep(100000); 
    liberar_barramento("Thread Alta ");
    
    registrar_log("Thread Alta ", "Finalizada a tempo.");
    exibir_tempo_total("Thread Alta ", inicio);
    return NULL;
}

int main() {
    pthread_t tb, tm, ta;
    monitor_barramento.barramento_ocupado = 0;
    monitor_barramento.prioridade_herdada = 0;
    pthread_mutex_init(&monitor_barramento.mutex, NULL);
    pthread_cond_init(&monitor_barramento.cond, NULL);
    
    printf("--- INICIANDO SIMULACAO COM HERANCA DE PRIORIDADE (SOLUCAO) ---\n\n");
    pthread_create(&tb, NULL, thread_baixa, NULL);
    pthread_create(&ta, NULL, thread_alta, NULL);
    pthread_create(&tm, NULL, thread_media, NULL);
    
    pthread_join(tb, NULL); pthread_join(ta, NULL); pthread_join(tm, NULL);
    return 0;
}