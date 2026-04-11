#include "batch_adapters.h"

int batch_bb_adapter_init(batch_bb_adapter_ctx* ctx,
                          const bb_algorithm* algorithm,
                          const uint8_t* sk,
                          size_t sk_len,
                          const uint8_t* pk,
                          size_t pk_len) {
    if (ctx == NULL || algorithm == NULL || algorithm->sign == NULL || algorithm->verify == NULL) {
        return -1;
    }
    if (sk == NULL || pk == NULL) {
        return -1;
    }

    ctx->algorithm = algorithm;
    ctx->sk = sk;
    ctx->sk_len = sk_len;
    ctx->pk = pk;
    ctx->pk_len = pk_len;
    return 0;
}

size_t batch_bb_signature_capacity(const batch_bb_adapter_ctx* ctx) {
    if (ctx == NULL || ctx->algorithm == NULL) {
        return 0;
    }
    return ctx->algorithm->signature_bytes;
}

int batch_bb_sign_callback(void* user_ctx,
                           const uint8_t* msg,
                           size_t msg_len,
                           uint8_t* sig,
                           size_t sig_capacity,
                           size_t* sig_len) {
    batch_bb_adapter_ctx* ctx = (batch_bb_adapter_ctx*)user_ctx;
    bb_status status;

    if (ctx == NULL || ctx->algorithm == NULL || ctx->algorithm->sign == NULL ||
        msg == NULL || sig == NULL || sig_len == NULL) {
        return 0;
    }

    status = ctx->algorithm->sign(ctx->sk,
                                  ctx->sk_len,
                                  ctx->pk,
                                  ctx->pk_len,
                                  msg,
                                  msg_len,
                                  sig,
                                  sig_capacity,
                                  sig_len);
    return status == BB_OK ? 1 : 0;
}

int batch_bb_verify_callback(void* user_ctx,
                             const uint8_t* msg,
                             size_t msg_len,
                             const uint8_t* sig,
                             size_t sig_len) {
    batch_bb_adapter_ctx* ctx = (batch_bb_adapter_ctx*)user_ctx;
    bb_status status;

    if (ctx == NULL || ctx->algorithm == NULL || ctx->algorithm->verify == NULL ||
        msg == NULL || sig == NULL) {
        return 0;
    }

    status = ctx->algorithm->verify(ctx->pk, ctx->pk_len, msg, msg_len, sig, sig_len);
    return status == BB_OK ? 1 : 0;
}
