#include "batch_bench.h"
#include "utils/message_gen.h"
#include "utils/timer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

typedef struct bench_config {
    const char *algo;
    const char *params;
    size_t batch_size;
    size_t iters;
    size_t warmup;
    size_t msg_size;
    uint64_t seed;
    int verify;
    const char *out_csv;
    const char *out_json;
} bench_config;

typedef struct bench_result {
    const char *algorithm;
    const char *params;
    size_t batch_size;
    double keygen_ms;
    double time_seq_ms;
    double verify_ms;
    double serialize_time_ms;
    double peak_memory_mb;
    double total_sig_size_mb;
} bench_result;

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("  --algo <name|all>      Algorithm name or all (default: all)\n");
    printf("  --params <name>        Paramset tag for result output (default: default)\n");
    printf("  --batch-size <n>       Messages per iteration (default: 1)\n");
    printf("  --iters <n>            Iterations (default: 100)\n");
    printf("  --warmup <n>           Warmup iterations (default: 0)\n");
    printf("  --msg-size <n>         Message size in bytes (default: 1024)\n");
    printf("  --seed <n>             Deterministic message seed (default: 1)\n");
    printf("  --verify <0|1>         Run verify pass (default: 1)\n");
    printf("  --out-csv <path>       Output CSV path\n");
    printf("  --out-json <path>      Output JSON path\n");
    printf("  --help                 Show this message\n");
}

static int parse_size(const char *s, size_t *out) {
    char *end = NULL;
    unsigned long long v;

    if (s == NULL || out == NULL) {
        return -1;
    }
    errno = 0;
    v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

static int parse_u64(const char *s, uint64_t *out) {
    char *end = NULL;
    unsigned long long v;

    if (s == NULL || out == NULL) {
        return -1;
    }
    errno = 0;
    v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }
    *out = (uint64_t)v;
    return 0;
}

static int parse_int(const char *s, int *out) {
    char *end = NULL;
    long v;

    if (s == NULL || out == NULL) {
        return -1;
    }
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }
    *out = (int)v;
    return 0;
}

static int parse_args(int argc, char **argv, bench_config *cfg) {
    int i;

    cfg->algo = "all";
    cfg->params = "default";
    cfg->batch_size = 1;
    cfg->iters = 100;
    cfg->warmup = 0;
    cfg->msg_size = BB_DEFAULT_MESSAGE_SIZE;
    cfg->seed = 1;
    cfg->verify = 1;
    cfg->out_csv = NULL;
    cfg->out_json = NULL;

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        }
        if (strcmp(arg, "--algo") == 0 && i + 1 < argc) {
            cfg->algo = argv[++i];
            continue;
        }
        if (strcmp(arg, "--params") == 0 && i + 1 < argc) {
            cfg->params = argv[++i];
            continue;
        }
        if (strcmp(arg, "--batch-size") == 0 && i + 1 < argc) {
            if (parse_size(argv[++i], &cfg->batch_size) != 0 || cfg->batch_size == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--iters") == 0 && i + 1 < argc) {
            if (parse_size(argv[++i], &cfg->iters) != 0 || cfg->iters == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--warmup") == 0 && i + 1 < argc) {
            if (parse_size(argv[++i], &cfg->warmup) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--msg-size") == 0 && i + 1 < argc) {
            if (parse_size(argv[++i], &cfg->msg_size) != 0 || cfg->msg_size == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--seed") == 0 && i + 1 < argc) {
            if (parse_u64(argv[++i], &cfg->seed) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--verify") == 0 && i + 1 < argc) {
            if (parse_int(argv[++i], &cfg->verify) != 0 || (cfg->verify != 0 && cfg->verify != 1)) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--out-csv") == 0 && i + 1 < argc) {
            cfg->out_csv = argv[++i];
            continue;
        }
        if (strcmp(arg, "--out-json") == 0 && i + 1 < argc) {
            cfg->out_json = argv[++i];
            continue;
        }
        return -1;
    }

    return 0;
}

static double get_peak_memory_mb(void) {
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0) {
        return 0.0;
    }
    return (double)ru.ru_maxrss / 1024.0;
}

static int run_one(const bb_algorithm *algo, const bench_config *cfg, bench_result *out) {
    uint8_t *sk = NULL;
    uint8_t *pk = NULL;
    uint8_t *msg = NULL;
    uint8_t *sigs = NULL;
    size_t *sig_lens = NULL;
    uint8_t *serialized = NULL;
    size_t sk_len = 0;
    size_t pk_len = 0;
    size_t total_sig_bytes = 0;
    size_t total_iters;
    size_t it;
    bb_timer timer;

    sk = (uint8_t *)malloc(algo->secret_key_bytes);
    pk = (uint8_t *)malloc(algo->public_key_bytes);
    msg = (uint8_t *)malloc(cfg->msg_size);
    sigs = (uint8_t *)malloc(cfg->batch_size * algo->signature_bytes);
    sig_lens = (size_t *)calloc(cfg->batch_size, sizeof(size_t));
    if (sk == NULL || pk == NULL || msg == NULL || sigs == NULL || sig_lens == NULL) {
        goto fail;
    }

    bb_timer_start(&timer);
    if (algo->keygen(sk, algo->secret_key_bytes, &sk_len,
                     pk, algo->public_key_bytes, &pk_len) != BB_OK) {
        goto fail;
    }
    bb_timer_stop(&timer);
    out->keygen_ms = bb_timer_elapsed_ms(&timer);

    out->time_seq_ms = 0.0;
    out->verify_ms = 0.0;
    out->serialize_time_ms = 0.0;

    total_iters = cfg->warmup + cfg->iters;

    for (it = 0; it < total_iters; ++it) {
        const int measured = it >= cfg->warmup;
        size_t batch_sig_bytes = 0;
        size_t i;

        for (i = 0; i < cfg->batch_size; ++i) {
            size_t sig_len = 0;
            uint8_t *sig_slot = sigs + (i * algo->signature_bytes);

            bb_fill_message(msg, cfg->msg_size, cfg->seed, it * cfg->batch_size + i);

            if (measured) {
                bb_timer_start(&timer);
                if (algo->sign(sk, sk_len, pk, pk_len,
                               msg, cfg->msg_size,
                               sig_slot, algo->signature_bytes,
                               &sig_len) != BB_OK) {
                    goto fail;
                }
                bb_timer_stop(&timer);
                out->time_seq_ms += bb_timer_elapsed_ms(&timer);
            } else {
                if (algo->sign(sk, sk_len, pk, pk_len,
                               msg, cfg->msg_size,
                               sig_slot, algo->signature_bytes,
                               &sig_len) != BB_OK) {
                    goto fail;
                }
            }

            sig_lens[i] = sig_len;
            batch_sig_bytes += sig_len;

            if (measured) {
                total_sig_bytes += sig_len;
            }

            if (cfg->verify) {
                if (measured) {
                    bb_timer_start(&timer);
                    if (algo->verify(pk, pk_len, msg, cfg->msg_size,
                                     sig_slot, sig_len) != BB_OK) {
                        goto fail;
                    }
                    bb_timer_stop(&timer);
                    out->verify_ms += bb_timer_elapsed_ms(&timer);
                } else {
                    if (algo->verify(pk, pk_len, msg, cfg->msg_size,
                                     sig_slot, sig_len) != BB_OK) {
                        goto fail;
                    }
                }
            }
        }

        if (measured) {
            serialized = (uint8_t *)realloc(serialized, batch_sig_bytes == 0 ? 1 : batch_sig_bytes);
            if (serialized == NULL) {
                goto fail;
            }

            {
                size_t cursor = 0;
                bb_timer_start(&timer);
                for (i = 0; i < cfg->batch_size; ++i) {
                    memcpy(serialized + cursor,
                           sigs + (i * algo->signature_bytes),
                           sig_lens[i]);
                    cursor += sig_lens[i];
                }
                bb_timer_stop(&timer);
                out->serialize_time_ms += bb_timer_elapsed_ms(&timer);
            }
        }
    }

    out->algorithm = algo->name;
    out->params = cfg->params;
    out->batch_size = cfg->batch_size;
    out->peak_memory_mb = get_peak_memory_mb();
    out->total_sig_size_mb = ((double)total_sig_bytes / (double)cfg->iters) / (1024.0 * 1024.0);

    free(sk);
    free(pk);
    free(msg);
    free(sigs);
    free(sig_lens);
    free(serialized);
    return 0;

fail:
    free(sk);
    free(pk);
    free(msg);
    free(sigs);
    free(sig_lens);
    free(serialized);
    return -1;
}

static void print_result(const bench_result *r) {
        printf("algo=%s params=%s batch=%zu keygen_ms=%.3f seq_ms=%.3f verify_ms=%.3f serialize_ms=%.3f peak_mem_mb=%.2f total_sig_mb=%.3f\n",
           r->algorithm,
           r->params,
           r->batch_size,
           r->keygen_ms,
           r->time_seq_ms,
           r->verify_ms,
           r->serialize_time_ms,
           r->peak_memory_mb,
           r->total_sig_size_mb);
}

static int write_csv(FILE *f, const bench_result *r) {
    return fprintf(
        f,
        "%s,%s,%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        r->algorithm,
        r->params,
        r->batch_size,
        r->time_seq_ms,
        0.0,
        0.0,
        r->peak_memory_mb,
        r->total_sig_size_mb,
        r->serialize_time_ms) < 0
               ? -1
               : 0;
}

static int write_json(FILE *f, const bench_result *results, size_t count,
                      const bench_config *cfg) {
    size_t i;
    if (fprintf(f,
                "{\n"
                "  \"config\": {\n"
                "    \"algo\": \"%s\",\n"
                "    \"params\": \"%s\",\n"
                "    \"batch_size\": %zu,\n"
                "    \"iters\": %zu,\n"
                "    \"warmup\": %zu,\n"
                "    \"msg_size\": %zu,\n"
                "    \"seed\": %llu,\n"
                "    \"verify\": %d\n"
                "  },\n"
                "  \"results\": [\n",
                cfg->algo,
                cfg->params,
                cfg->batch_size,
                cfg->iters,
                cfg->warmup,
                cfg->msg_size,
                (unsigned long long)cfg->seed,
                cfg->verify) < 0) {
        return -1;
    }

    for (i = 0; i < count; ++i) {
        const bench_result *r = &results[i];
        if (fprintf(f,
                    "    {\"algorithm\":\"%s\",\"params\":\"%s\",\"batch_size\":%zu,"
                    "\"time_seq_ms\":%.6f,\"time_batch_ms\":%.6f,"
                    "\"speedup\":%.6f,\"peak_memory_MB\":%.6f,"
                    "\"total_sig_size_MB\":%.6f,\"serialize_time_ms\":%.6f}%s\n",
                    r->algorithm,
                    r->params,
                    r->batch_size,
                    r->time_seq_ms,
                    0.0,
                    0.0,
                    r->peak_memory_mb,
                    r->total_sig_size_mb,
                    r->serialize_time_ms,
                    i + 1 == count ? "" : ",") < 0) {
            return -1;
        }
    }

    return fprintf(f, "  ]\n}\n") < 0 ? -1 : 0;
}

int main(int argc, char **argv) {
    bench_config cfg;
    bench_result *results = NULL;
    size_t results_count = 0;
    FILE *csv = NULL;
    FILE *json = NULL;
    size_t i;
    int parse_rc;

    parse_rc = parse_args(argc, argv, &cfg);
    if (parse_rc == 1) {
        return 0;
    }
    if (parse_rc != 0) {
        print_usage(argv[0]);
        return 1;
    }

    results = (bench_result *)calloc(bb_algorithm_count(), sizeof(bench_result));
    if (results == NULL) {
        return 1;
    }

    if (cfg.out_csv != NULL) {
        csv = fopen(cfg.out_csv, "w");
        if (csv == NULL) {
            free(results);
            return 1;
        }
        fprintf(csv,
            "algorithm,params,batch_size,time_seq_ms,time_batch_ms,speedup,"
                "peak_memory_MB,total_sig_size_MB,serialize_time_ms\n");
    }

    for (i = 0; i < bb_algorithm_count(); ++i) {
        const bb_algorithm *algo = bb_algorithm_at(i);

        if (strcmp(cfg.algo, "all") != 0 && strcmp(cfg.algo, algo->name) != 0) {
            continue;
        }

        if (run_one(algo, &cfg, &results[results_count]) != 0) {
            if (csv != NULL) {
                fclose(csv);
            }
            free(results);
            return 1;
        }

        print_result(&results[results_count]);
        if (csv != NULL && write_csv(csv, &results[results_count]) != 0) {
            fclose(csv);
            free(results);
            return 1;
        }

        ++results_count;
    }

    if (results_count == 0) {
        if (csv != NULL) {
            fclose(csv);
        }
        free(results);
        return 1;
    }

    if (csv != NULL) {
        fclose(csv);
    }

    if (cfg.out_json != NULL) {
        json = fopen(cfg.out_json, "w");
        if (json == NULL) {
            free(results);
            return 1;
        }
        if (write_json(json, results, results_count, &cfg) != 0) {
            fclose(json);
            free(results);
            return 1;
        }
        fclose(json);
    }

    free(results);
    return 0;
}
