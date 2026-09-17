import threading
import time

THREAD_BAIXA = "Thread Baixa"
THREAD_MEDIA = "Thread Media"
THREAD_ALTA = "Thread Alta"

barramento_ocupado = False
mutex = threading.Lock()
not_busy = threading.Condition(mutex)

cpu_ocupada = False

def registrar_log(nome_thread, msg):
    print(f"[{time.monotonic()}] [{nome_thread}]: {msg}\n")

def exibir_tempo_total(nome_thread, inicio):
    tempo_gasto = time.monotonic() - inicio
    print(f"\n>>> [RESULTADO] A {nome_thread} levou {tempo_gasto:.4f} segundos do inicio ao fim. <<<\n")


def adquirir_barramento(nome_thread):
    global barramento_ocupado
    not_busy.acquire()

    while barramento_ocupado:
        registrar_log(nome_thread, "Barramento ocupado. Thread vai esperar...")
        not_busy.wait()

    barramento_ocupado = True
    registrar_log(nome_thread, "Adquiriu o barramento com sucesso.")

    not_busy.release()


def liberar_barramento(nome_thread):
    global barramento_ocupado
    not_busy.acquire()

    barramento_ocupado = False
    registrar_log(nome_thread, "Liberou o barramento.")
    not_busy.notify()

    not_busy.release()


def thread_baixa():
    global cpu_ocupada
    inicio = time.monotonic()
    registrar_log(THREAD_BAIXA, "Iniciando...")
    adquirir_barramento(THREAD_BAIXA)

    registrar_log(THREAD_BAIXA, "Realizando escrita lenta no barramento (Dividida em etapas)...")

    for _ in range(5):
        while cpu_ocupada:
            registrar_log(THREAD_BAIXA, "CPU está ocupada. Aguardando liberação.")
            time.sleep(0.05)
        time.sleep(0.1)  # Trabalho real da etapa

    liberar_barramento(THREAD_BAIXA)
    registrar_log(THREAD_BAIXA, "Finalizada.")
    exibir_tempo_total(THREAD_BAIXA, inicio)


def thread_media():
    global cpu_ocupada
    time.sleep(0.05)  # Acorda logo para pegar a CPU
    inicio = time.monotonic()

    registrar_log(THREAD_MEDIA, "Iniciando. PREEMPTANDO a Baixa e Monopolizando a CPU...")

    cpu_ocupada = True

    contador = 0
    # Aumentado para 50 milhões para gerar um gargalo de tempo perceptível
    for i in range(50000000):
        contador += 1

    cpu_ocupada = False
    registrar_log(THREAD_MEDIA, "Finalizou calculos intensivos. Devolveu a CPU.")
    exibir_tempo_total(THREAD_MEDIA, inicio)


def thread_alta():
    time.sleep(0.02)
    inicio = time.monotonic()

    registrar_log(THREAD_ALTA, "Iniciando requisicao critica de tempo real...")
    adquirir_barramento(THREAD_ALTA)

    registrar_log(THREAD_ALTA, "Enviando dados...")
    time.sleep(0.1)
    liberar_barramento(THREAD_ALTA)

    registrar_log(THREAD_ALTA, "Finalizada.")
    exibir_tempo_total(THREAD_ALTA, inicio)


def main():
    print("--- INICIANDO SIMULADOR MARS PATHFINDER (O TRAVAMENTO) ---\n")

    tb = threading.Thread(target=thread_baixa)
    ta = threading.Thread(target=thread_alta)
    tm = threading.Thread(target=thread_media)

    tb.start()
    ta.start()
    tm.start()

    tb.join()
    ta.join()
    tm.join()


if __name__ == "__main__":
    main()