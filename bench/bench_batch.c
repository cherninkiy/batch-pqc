#include "batch_adapters.h"
#include "batch_bench.h"
#include "batch_signing.h"
#include "utils/message_gen.h"
#include "utils/timer.h"
#include "gost3411-2012-core.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <limits.h>

#define BATCH_HASH_BITS 256u
#define BATCH_HASH_SIZE (BATCH_HASH_BITS / 8u)

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
    double time_batch_ms;
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
    unsigned long long value;

    if (s == NULL || out == NULL) {
        return -1;
    }

    errno = 0;
    value = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }

    *out = (size_t)value;
    return 0;
}

static int parse_u64(const char *s, uint64_t *out) {
    char *end = NULL;
    unsigned long long value;

    if (s == NULL || out == NULL) {
        return -1;
    }

    errno = 0;
    value = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }

    *out = (uint64_t)value;
    return 0;
}

static int parse_int(const char *s, int *out) {
    char *end = NULL;
    long value;

    if (s == NULL || out == NULL) {
        return -1;
    }

    errno = 0;
    value = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }

    *out = (int)value;
    return 0;
}

static int parse_args(int argc, char **argv, bench_config *cfg) {
    int index;

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

    for (index = 1; index < argc; ++index) {
        const char *arg = argv[index];

        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        }
        if (strcmp(arg, "--algo") == 0 && index + 1 < argc) {
            cfg->algo = argv[++index];
            continue;
        }
        if (strcmp(arg, "--params") == 0 && index + 1 < argc) {
            cfg->params = argv[++index];
            continue;
        }
        if (strcmp(arg, "--batch-size") == 0 && index + 1 < argc) {
            if (parse_size(argv[++index], &cfg->batch_size) != 0 || cfg->batch_size == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--iters") == 0 && index + 1 < argc) {
            if (parse_size(argv[++index], &cfg->iters) != 0 || cfg->iters == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--warmup") == 0 && index + 1 < argc) {
            if (parse_size(argv[++index], &cfg->warmup) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--msg-size") == 0 && index + 1 < argc) {
            if (parse_size(argv[++index], &cfg->msg_size) != 0 || cfg->msg_size == 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--seed") == 0 && index + 1 < argc) {
            if (parse_u64(argv[++index], &cfg->seed) != 0) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--verify") == 0 && index + 1 < argc) {
            if (parse_int(argv[++index], &cfg->verify) != 0 ||
                (cfg->verify != 0 && cfg->verify != 1)) {
                return -1;
            }
            continue;
        }
        if (strcmp(arg, "--out-csv") == 0 && index + 1 < argc) {
            cfg->out_csv = argv[++index];
            continue;
        }
        if (strcmp(arg, "--out-json") == 0 && index + 1 < argc) {
            cfg->out_json = argv[++index];
            continue;
        }
        return -1;
    }

    return 0;
}

static double get_peak_memory_mb(void) {
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }

    return (double)usage.ru_maxrss / 1024.0;
}

static void streebog256_hash(const uint8_t *in, size_t in_len, uint8_t *out) {
    GOST34112012Context ctx;

    GOST34112012Init(&ctx, 256u);
    GOST34112012Update(&ctx, in, in_len);
    GOST34112012Final(&ctx, out);
    GOST34112012Cleanup(&ctx);
}

static int run_one(const bb_algorithm *algo, const bench_config *cfg, bench_result *out) {
    batch_bb_adapter_ctx adapter_ctx;
    batch_signer_t *signer = NULL;
    batch_signature_t *signature = NULL;
    uint8_t *sk = NULL;
    uint8_t *pk = NULL;
    uint8_t *messages_buf = NULL;
    const uint8_t **messages = NULL;
    size_t *msg_lens = NULL;
    uint8_t *serialized = NULL;
    size_t sk_len = 0;
    size_t pk_len = 0;
    size_t total_sig_bytes = 0;
    size_t total_iters;
    size_t iteration;
    bb_timer timer;

    sk = (uint8_t *)malloc(algo->secret_key_bytes);
    pk = (uint8_t *)malloc(algo->public_key_bytes);

    /* check for overflow when computing messages buffer size */
    if (cfg->batch_size > 0 && cfg->msg_size > SIZE_MAX / cfg->batch_size) {
        goto fail;
    }
    messages_buf = (uint8_t *)malloc(cfg->batch_size * cfg->msg_size);

    /* check for overflow when allocating pointer/length arrays */
    if (cfg->batch_size > 0 && sizeof(*messages) > SIZE_MAX / cfg->batch_size) {
        goto fail;
    }
    messages = (const uint8_t **)calloc(cfg->batch_size, sizeof(*messages));
    if (cfg->batch_size > 0 && sizeof(*msg_lens) > SIZE_MAX / cfg->batch_size) {
        goto fail;
    }
    msg_lens = (size_t *)calloc(cfg->batch_size, sizeof(*msg_lens));
    if (sk == NULL || pk == NULL || messages_buf == NULL || messages == NULL || msg_lens == NULL) {
        goto fail;
    }

    bb_timer_start(&timer);
    if (algo->keygen(sk, algo->secret_key_bytes, &sk_len,
                     pk, algo->public_key_bytes, &pk_len) != BB_OK) {
        goto fail;
    }
    bb_timer_stop(&timer);
    out->keygen_ms = bb_timer_elapsed_ms(&timer);

    if (batch_bb_adapter_init(&adapter_ctx, algo, sk, sk_len, pk, pk_len) != 0) {
        goto fail;
    }

    signer = batch_signer_create(BATCH_HASH_SIZE,
                                 batch_bb_signature_capacity(&adapter_ctx),
                                 streebog256_hash,
                                 batch_bb_sign_callback,
                                 &adapter_ctx);
    if (signer == NULL) {
        goto fail;
    }

    out->time_batch_ms = 0.0;
    out->verify_ms = 0.0;
    out->serialize_time_ms = 0.0;

    total_iters = cfg->warmup + cfg->iters;
    for (iteration = 0; iteration < total_iters; ++iteration) {
        const int measured = iteration >= cfg->warmup;
        size_t message_index;
        size_t serialized_len = 0;

        batch_signer_reset(signer);
        if (signature != NULL) {
            batch_signature_free(signature);
            signature = NULL;
        }

        for (message_index = 0; message_index < cfg->batch_size; ++message_index) {
            uint8_t *message = messages_buf + (message_index * cfg->msg_size);

            bb_fill_message(message,
                            cfg->msg_size,
                            cfg->seed,
                            iteration * cfg->batch_size + message_index);
            messages[message_index] = message;
            msg_lens[message_index] = cfg->msg_size;

            if (batch_signer_add_message(signer, message, cfg->msg_size) != 0) {
                goto fail;
            }
        }

        if (measured) {
            bb_timer_start(&timer);
            signature = batch_signer_sign(signer);
            bb_timer_stop(&timer);
            out->time_batch_ms += bb_timer_elapsed_ms(&timer);
        } else {
            signature = batch_signer_sign(signer);
        }

        if (signature == NULL) {
            goto fail;
        }

        if (batch_signature_serialize(signature, NULL, &serialized_len) != 0) {
            goto fail;
        }

        if (measured) {
            uint8_t *grown = (uint8_t *)realloc(serialized, serialized_len == 0 ? 1 : serialized_len);

            if (grown == NULL) {
                goto fail;
            }
            serialized = grown;

            bb_timer_start(&timer);
            if (batch_signature_serialize(signature, serialized, &serialized_len) != 0) {
                bb_timer_stop(&timer);
                goto fail;
            }
            bb_timer_stop(&timer);
            out->serialize_time_ms += bb_timer_elapsed_ms(&timer);
            total_sig_bytes += serialized_len;
        }

        if (cfg->verify) {
            int ok;

            if (measured) {
                bb_timer_start(&timer);
                ok = batch_verify(messages,
                                  msg_lens,
                                  cfg->batch_size,
                                  signature,
                                  BATCH_HASH_SIZE,
                                  streebog256_hash,
                                  batch_bb_verify_callback,
                                  &adapter_ctx);
                bb_timer_stop(&timer);
                out->verify_ms += bb_timer_elapsed_ms(&timer);
            } else {
                ok = batch_verify(messages,
                                  msg_lens,
                                  cfg->batch_size,
                                  signature,
                                  BATCH_HASH_SIZE,
                                  streebog256_hash,
                                  batch_bb_verify_callback,
                                  &adapter_ctx);
            }

            if (!ok) {
                goto fail;
            }
        }
    }

    out->algorithm = algo->name;
    out->params = cfg->params;
    out->batch_size = cfg->batch_size;
    out->peak_memory_mb = get_peak_memory_mb();
    out->total_sig_size_mb = ((double)total_sig_bytes / (double)cfg->iters) / (1024.0 * 1024.0);

    batch_signature_free(signature);
    batch_signer_free(signer);
    free(sk);
    free(pk);
    free(messages_buf);
    free(messages);
    free(msg_lens);
    free(serialized);
    return 0;

fail:
    batch_signature_free(signature);
    batch_signer_free(signer);
    free(sk);
    free(pk);
    free(messages_buf);
    free(messages);
    free(msg_lens);
    free(serialized);
    return -1;
}

static void print_result(const bench_result *result) {
    printf("algo=%s params=%s batch=%zu keygen_ms=%.3f batch_ms=%.3f verify_ms=%.3f serialize_ms=%.3f peak_mem_mb=%.2f total_sig_mb=%.3f\n",
           result->algorithm,
           result->params,
           result->batch_size,
           result->keygen_ms,
           result->time_batch_ms,
           result->verify_ms,
           result->serialize_time_ms,
           result->peak_memory_mb,
           result->total_sig_size_mb);
}

static int write_csv(FILE *file, const bench_result *result) {
    return fprintf(file,
                   "%s,%s,%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                   result->algorithm,
                   result->params,
                   result->batch_size,
                   0.0,
                   result->time_batch_ms,
                   0.0,
                   result->peak_memory_mb,
                   result->total_sig_size_mb,
                   result->serialize_time_ms) < 0
               ? -1
               : 0;
}

static int write_json(FILE *file,
                      const bench_result *results,
                      size_t count,
                      const bench_config *cfg) {
    size_t index;

    if (fprintf(file,
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

    for (index = 0; index < count; ++index) {
        const bench_result *result = &results[index];

        if (fprintf(file,
                    "    {\"algorithm\":\"%s\",\"params\":\"%s\",\"batch_size\":%zu,"
                    "\"time_seq_ms\":%.6f,\"time_batch_ms\":%.6f,"
                    "\"speedup\":%.6f,\"peak_memory_MB\":%.6f,"
                    "\"total_sig_size_MB\":%.6f,\"serialize_time_ms\":%.6f}%s\n",
                    result->algorithm,
                    result->params,
                    result->batch_size,
                    0.0,
                    result->time_batch_ms,
                    0.0,
                    result->peak_memory_mb,
                    result->total_sig_size_mb,
                    result->serialize_time_ms,
                    index + 1 == count ? "" : ",") < 0) {
            return -1;
        }
    }

    return fprintf(file, "  ]\n}\n") < 0 ? -1 : 0;
}

int main(int argc, char **argv) {
    bench_config cfg;
    bench_result *results = NULL;
    size_t results_count = 0;
    FILE *csv = NULL;
    FILE *json = NULL;
    size_t index;
    int parse_result;

    parse_result = parse_args(argc, argv, &cfg);
    if (parse_result == 1) {
        return 0;
    }
    if (parse_result != 0) {
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

    for (index = 0; index < bb_algorithm_count(); ++index) {
        const bb_algorithm *algo = bb_algorithm_at(index);

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