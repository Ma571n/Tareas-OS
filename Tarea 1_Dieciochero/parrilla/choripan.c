#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "../recetas/choripan.h"

// Maximo buffer pa mandar insumos por pipe
#define INSUMOS_BUF 4096
#define INSUMOS_MAX 48

// Señales
static volatile sig_atomic_t le_cayo_la_seremi = 0;
static sigset_t mascara_original;
static sigset_t mascara_espera;
static sigset_t solo_sigint;

// Procesos activos para que los hijos no dejen pipes abiertos
static int *lista_en_fierro = NULL;
static int cant_en_fierro = 0;

static void handler_sigint(int sig) {
    (void)sig;
    le_cayo_la_seremi = 1;
}

// Handler para sigchld para que despierte el sigsuspend
static void handler_sigchld(int sig) {
    (void)sig;
}

// Reviso si cayo la seremi (sigint)
static void revisar_seremi(void) {
    sigprocmask(SIG_UNBLOCK, &solo_sigint, NULL);
    sigprocmask(SIG_BLOCK, &solo_sigint, NULL);
}

// Hash simple pa buscar id rapido
static unsigned long hash_str(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++)) h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

static int escribir_todo(int fd, const char *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(fd, buf + off, n - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

static void dormir_ms(int ms) {
    struct timespec req, rem;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) req = rem;
}

// Ve si un id esta en la lista de fallas
static int id_en_lista(const char *lista, const char *id) {
    if (!lista) return 0;
    size_t n = strlen(id);
    const char *p = lista;
    while (*p) {
        const char *fin = strchr(p, ',');
        size_t len = fin ? (size_t)(fin - p) : strlen(p);
        if (len == n && strncmp(p, id, n) == 0) return 1;
        if (!fin) break;
        p = fin + 1;
    }
    return 0;
}

// Se arman las relaciones de dependencias entre tareas
static int resolver_dependencias(Grafo *g) {
    int n = g->total_actividades;

    for (int i = 0; i < n; i++) {
        Actividad *a = &g->actividades[i];
        a->dependientes_idx = NULL;
        a->dependencias_idx = NULL;
        a->num_dependientes = 0;
        a->pid = -1;
        a->pipe_fd[0] = -1;
        a->pipe_fd[1] = -1;
    }

    size_t cap = 16;
    while (cap < (size_t)n * 2) cap <<= 1;
    int *tabla = malloc(cap * sizeof(int));
    if (!tabla) {
        perror("malloc tabla");
        return -1;
    }
    for (size_t k = 0; k < cap; k++) tabla[k] = -1;

    for (int i = 0; i < n; i++) {
        size_t pos = hash_str(g->actividades[i].id) & (cap - 1);
        while (tabla[pos] != -1) {
            if (strcmp(g->actividades[tabla[pos]].id, g->actividades[i].id) == 0) {
                fprintf(stderr, "Error: ID repetido '%s'\n", g->actividades[i].id);
                free(tabla);
                return -1;
            }
            pos = (pos + 1) & (cap - 1);
        }
        tabla[pos] = i;
    }

    // pasada 1: mapeo id a indices
    for (int i = 0; i < n; i++) {
        Actividad *a = &g->actividades[i];
        if (a->num_dependencias == 0) continue;
        a->dependencias_idx = malloc(sizeof(int) * (size_t)a->num_dependencias);
        if (!a->dependencias_idx) {
            perror("malloc dependencias_idx");
            free(tabla);
            return -1;
        }
        for (int j = 0; j < a->num_dependencias; j++) {
            const char *dep_id = a->dependencias_ids[j];
            size_t pos = hash_str(dep_id) & (cap - 1);
            int dep_idx = -1;
            while (tabla[pos] != -1) {
                if (strcmp(g->actividades[tabla[pos]].id, dep_id) == 0) {
                    dep_idx = tabla[pos];
                    break;
                }
                pos = (pos + 1) & (cap - 1);
            }
            if (dep_idx < 0) {
                fprintf(stderr, "Error: Dependencia '%s' no existe (actividad '%s')\n", dep_id, a->id);
                free(tabla);
                return -1;
            }
            a->dependencias_idx[j] = dep_idx;
            g->actividades[dep_idx].num_dependientes++;
        }
    }
    free(tabla);

    // pasada 2: lleno arreglo de los que dependen de mi
    for (int i = 0; i < n; i++) {
        Actividad *a = &g->actividades[i];
        if (a->num_dependientes == 0) continue;
        a->dependientes_idx = malloc(sizeof(int) * (size_t)a->num_dependientes);
        if (!a->dependientes_idx) {
            perror("malloc dependientes_idx");
            return -1;
        }
        a->num_dependientes = 0;
    }
    for (int i = 0; i < n; i++) {
        Actividad *a = &g->actividades[i];
        for (int j = 0; j < a->num_dependencias; j++) {
            Actividad *dep = &g->actividades[a->dependencias_idx[j]];
            dep->dependientes_idx[dep->num_dependientes++] = i;
        }
    }
    return 0;
}

// Si muere una tarea, cancelo las que dependian de ella
static void quemar_rama(Grafo *g, int idx) {
    Actividad *act = &g->actividades[idx];
    for (int i = 0; i < act->num_dependientes; i++) {
        int dep_idx = act->dependientes_idx[i];
        if (g->actividades[dep_idx].estado != TA_QUEMAO) {
            g->actividades[dep_idx].estado = TA_QUEMAO;
            printf("  [QUEMAO] [%s] %s -- cancelada (dependia de %s)\n",
                   g->actividades[dep_idx].id,
                   g->actividades[dep_idx].nombre,
                   act->id);
            quemar_rama(g, dep_idx);
        }
    }
}

// Lo que ejecuta el proceso hijo
static void ejecutar_hijo(Grafo *g, Actividad *act, int fd_insumos, int fd_resultado) {
    // cierro los pipes viejos pa no dejar leaks
    for (int s = 0; s < cant_en_fierro; s++) {
        Actividad *otra = &g->actividades[lista_en_fierro[s]];
        if (otra != act && otra->pipe_fd[0] >= 0) close(otra->pipe_fd[0]);
    }

    // Reseteo las senales en el hijo
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    sigprocmask(SIG_SETMASK, &mascara_original, NULL);

    // 1) leo lo que me mando el papa
    char buf[INSUMOS_BUF + 1];
    size_t len = 0;
    while (len < INSUMOS_BUF) {
        ssize_t r = read(fd_insumos, buf + len, INSUMOS_BUF - len);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) break;
        len += (size_t)r;
    }
    close(fd_insumos);
    buf[len] = '\0';

    if (len > 0) {
        char salida[2 * INSUMOS_BUF + 128];
        size_t o = (size_t)snprintf(salida, sizeof(salida), "  >> [%s] recibe insumos:\n", act->id);
        char *linea = strtok(buf, "\n");
        while (linea && o + 128 < sizeof(salida)) {
            o += (size_t)snprintf(salida + o, sizeof(salida) - o, "       - %s\n", linea);
            linea = strtok(NULL, "\n");
        }
        escribir_todo(STDOUT_FILENO, salida, o);
    }

    // 2) Simula la pega
    dormir_ms(act->tiempo_ms);

    // 3) Si le tocaba fallar sale con error
    if (act->simular_falla) _exit(2);

    // 4) Le avisa al papa que termine
    char msg[MSG_MAX];
    int n = snprintf(msg, sizeof(msg), "[%s] %s lista (%d ms)", act->id, act->nombre, act->tiempo_ms);
    if (n < 0) _exit(1);
    if (n >= (int)sizeof(msg)) n = (int)sizeof(msg) - 1;
    if (escribir_todo(fd_resultado, msg, (size_t)n + 1) < 0) _exit(1);
    close(fd_resultado);
    _exit(0);
}

// Crea pipes y lanza el fork
static int lanzar_actividad(Grafo *g, int idx) {
    Actividad *act = &g->actividades[idx];
    int res[2], ins[2];

    if (pipe(res) == -1) {
        perror("pipe");
        return -1;
    }
    if (pipe(ins) == -1) {
        int e = errno;
        perror("pipe");
        close(res[0]);
        close(res[1]);
        errno = e;
        return -1;
    }

    // Prepara insumos pa mandar por el pipe
    char buf[INSUMOS_BUF];
    size_t len = 0;
    int enviados = 0;
    for (int d = 0; d < act->num_dependencias; d++) {
        if (enviados >= INSUMOS_MAX) {
            int n = snprintf(buf + len, sizeof(buf) - len, "(+%d insumos mas)\n", act->num_dependencias - d);
            if (n > 0) len += (size_t)n;
            break;
        }
        const char *m = g->actividades[act->dependencias_idx[d]].mensaje;
        int n = snprintf(buf + len, sizeof(buf) - len, "%s\n", m);
        if (n > 0) len += (size_t)n;
        enviados++;
    }

    // Escribe en el pipe antes del fork
    int ok = (len == 0) || (escribir_todo(ins[1], buf, len) == 0);
    int e = errno;
    close(ins[1]);
    if (!ok) {
        close(ins[0]);
        close(res[0]);
        close(res[1]);
        errno = e;
        return -1;
    }

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        e = errno;
        perror("fork");
        close(ins[0]);
        close(res[0]);
        close(res[1]);
        errno = e;
        return -1;
    }

    if (pid == 0) {
        close(res[0]);
        ejecutar_hijo(g, act, ins[0], res[1]);
        _exit(1);
    }

    // codigo del padre
    close(res[1]);
    close(ins[0]);
    act->pipe_fd[0] = res[0];
    act->pipe_fd[1] = -1;
    act->pid = pid;
    act->estado = TA_EN_FIERRO;
    return 0;
}

// funcion principal pa correr la parrilla
int prender_parrilla(Grafo *g, int K) {
    int total = g->total_actividades;
    le_cayo_la_seremi = 0;
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (resolver_dependencias(g) != 0) return -1;

    // valido limite maximo de K por si los fd no dan
    long max_fd = 1024;
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur != RLIM_INFINITY) max_fd = (long)rl.rlim_cur;
    long tope = max_fd - 16;
    if (tope < 1) tope = 1;
    if (K > tope) {
        fprintf(stderr, "[AVISO] K=%d supera el limite de descriptores (%ld); se usa K=%ld\n", K, max_fd, tope);
        K = (int)tope;
    }
    if (K > total) K = total;

    int *cola = malloc(sizeof(int) * (size_t)total);
    int *en_fierro = malloc(sizeof(int) * (size_t)K);
    if (!cola || !en_fierro) {
        perror("malloc");
        free(cola);
        free(en_fierro);
        return -1;
    }
    int cab = 0, fin_cola = 0, corriendo = 0;
    lista_en_fierro = en_fierro;

    // bloqueo sigint y sigchld pa usarlas con sigsuspend
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler_sigint;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = handler_sigchld;
    sigaction(SIGCHLD, &sa, NULL);

    sigemptyset(&solo_sigint);
    sigaddset(&solo_sigint, SIGINT);
    sigset_t bloqueo;
    sigemptyset(&bloqueo);
    sigaddset(&bloqueo, SIGINT);
    sigaddset(&bloqueo, SIGCHLD);
    sigprocmask(SIG_BLOCK, &bloqueo, &mascara_original);
    mascara_espera = mascara_original;
    sigdelset(&mascara_espera, SIGINT);
    sigdelset(&mascara_espera, SIGCHLD);

    // reviso env pa fallas simuladas
    const char *lista_fallas = getenv("FALLAR");
    const char *pct_txt = getenv("FALLA_PCT");
    int pct_falla = pct_txt ? atoi(pct_txt) : 0;

    for (int i = 0; i < total; i++) {
        if (g->actividades[i].dependencias_restantes == 0) cola[fin_cola++] = i;
    }

    printf("\n=== COMIENZA LA FIESTA (K=%d, actividades=%d) ===\n\n", K, total);

    int error_interno = 0;
    while (!le_cayo_la_seremi) {
        revisar_seremi();
        if (le_cayo_la_seremi) break;

        // lanzo tareas que estan listas si hay cupo
        while (corriendo < K && cab < fin_cola && !le_cayo_la_seremi) {
            int i = cola[cab];
            Actividad *act = &g->actividades[i];
            act->simular_falla = id_en_lista(lista_fallas, act->id) ||
                                 (pct_falla > 0 && (rand() % 100) < pct_falla);

            printf("[LANZANDO] [%s] %s (%d ms)...\n", act->id, act->nombre, act->tiempo_ms);
            cant_en_fierro = corriendo;
            if (lanzar_actividad(g, i) == 0) {
                cab++;
                en_fierro[corriendo++] = i;
            } else if (corriendo > 0 &&
                       (errno == EAGAIN || errno == ENOMEM || errno == EMFILE || errno == ENFILE)) {
                break;
            } else {
                cab++;
                act->estado = TA_QUEMAO;
                printf("[ERROR] No se pudo lanzar [%s] %s\n", act->id, act->nombre);
                quemar_rama(g, i);
            }
            revisar_seremi();
        }

        if (le_cayo_la_seremi) break;
        if (corriendo == 0) break;

        // espero a que termine un hijo
        int status = 0;
        pid_t terminado = 0;
        while (!le_cayo_la_seremi && terminado == 0) {
            pid_t p = waitpid(-1, &status, WNOHANG);
            if (p > 0) {
                terminado = p;
            } else if (p == 0) {
                sigsuspend(&mascara_espera);
            } else if (errno != EINTR) {
                error_interno = 1;
                break;
            }
        }
        if (le_cayo_la_seremi || error_interno) break;

        // proceso el hijo que termino
        int slot = -1;
        for (int s = 0; s < corriendo; s++) {
            if (g->actividades[en_fierro[s]].pid == terminado) {
                slot = s;
                break;
            }
        }
        if (slot < 0) continue;

        int i = en_fierro[slot];
        en_fierro[slot] = en_fierro[--corriendo];
        Actividad *act = &g->actividades[i];

        char msg[MSG_MAX];
        memset(msg, 0, sizeof(msg));
        ssize_t bytes = read(act->pipe_fd[0], msg, sizeof(msg) - 1);
        close(act->pipe_fd[0]);
        act->pipe_fd[0] = -1;
        act->pid = -1;

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && bytes > 0) {
            act->estado = TA_SERVIO;
            memcpy(act->mensaje, msg, MSG_MAX);
            printf("[SERVIO] %s\n", msg);

            // aviso a los que dependian de mi
            for (int d = 0; d < act->num_dependientes; d++) {
                Actividad *dep = &g->actividades[act->dependientes_idx[d]];
                dep->dependencias_restantes--;
                if (dep->dependencias_restantes == 0 && dep->estado == TA_CRUDO) {
                    cola[fin_cola++] = act->dependientes_idx[d];
                }
            }
        } else {
            act->estado = TA_QUEMAO;
            if (WIFSIGNALED(status)) {
                printf("[QUEMAO] [%s] %s -- FALLO (senal %d)\n", act->id, act->nombre, WTERMSIG(status));
            } else {
                printf("[QUEMAO] [%s] %s -- FALLO (exit=%d)\n", act->id, act->nombre,
                       WIFEXITED(status) ? WEXITSTATUS(status) : -1);
            }
            quemar_rama(g, i);
        }
    }

    // si llego la seremi corto a todos los hijos
    if (le_cayo_la_seremi) {
        printf("\n[SEREMI] Llego la autoridad! Cerrando la fonda...\n");

        for (int s = 0; s < corriendo; s++) {
            kill(g->actividades[en_fierro[s]].pid, SIGTERM);
        }

        while (corriendo > 0) {
            int st;
            pid_t pid = waitpid(-1, &st, 0);
            if (pid > 0) {
                for (int s = 0; s < corriendo; s++) {
                    Actividad *a = &g->actividades[en_fierro[s]];
                    if (a->pid == pid) {
                        close(a->pipe_fd[0]);
                        a->pipe_fd[0] = -1;
                        a->pid = -1;
                        a->estado = TA_QUEMAO;
                        printf("  [CANCELADA] [%s] %s\n", a->id, a->nombre);
                        en_fierro[s] = en_fierro[--corriendo];
                        break;
                    }
                }
            } else if (errno != EINTR) {
                break;
            }
        }
    }

    sigprocmask(SIG_SETMASK, &mascara_original, NULL);
    free(cola);
    free(en_fierro);

    // resumen final
    printf("\n=== RESUMEN DE LA FONDA ===\n");
    int ok = 0, fail = 0, pendientes = 0;
    for (int i = 0; i < total; i++) {
        switch (g->actividades[i].estado) {
            case TA_SERVIO:   ok++;         break;
            case TA_QUEMAO:   fail++;       break;
            case TA_EN_FIERRO:
            case TA_CRUDO:    pendientes++; break;
        }
    }
    printf("Completadas: %d | Fallidas/Canceladas: %d | Sin ejecutar: %d\n", ok, fail, pendientes);

    if (le_cayo_la_seremi) return 130;

    if (pendientes > 0) {
        printf("[AVISO] %d actividades no se pudieron ejecutar (posible dependencia circular)\n", pendientes);
    }
    return (fail > 0 || pendientes > 0) ? 1 : 0;
}