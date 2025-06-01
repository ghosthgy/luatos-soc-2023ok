#ifndef LUAT_ONEWIRE_H
#define LUAT_ONEWIRE_H

#define LUAT_ONEWIRE_MODE_GPIO 1
#define LUAT_ONEWIRE_MODE_HW   2

typedef struct luat_onewire_ctx
{
    int32_t id;
    int32_t mode;
    void* userdata;
}luat_onewire_ctx_t;


int luat_onewire_setup(luat_onewire_ctx_t* ctx);
int luat_onewire_read(luat_onewire_ctx_t* ctx, char* buff, size_t len);
int luat_onewire_write(luat_onewire_ctx_t* ctx, const char* buff, size_t len);
int luat_onewire_close(luat_onewire_ctx_t* ctx);

int luat_onewire_setup_gpio(luat_onewire_ctx_t* ctx);
int luat_onewire_read_gpio(luat_onewire_ctx_t* ctx, char* buff, size_t len);
int luat_onewire_write_gpio(luat_onewire_ctx_t* ctx, const char* buff, size_t len);
int luat_onewire_close_gpio(luat_onewire_ctx_t* ctx);

typedef int (*onewire_setup)(luat_onewire_ctx_t* ctx);
typedef int (*onewire_read)(luat_onewire_ctx_t* ctx, char* buff, size_t len);
typedef int (*onewire_write)(luat_onewire_ctx_t* ctx, const char* buff, size_t len);
typedef int (*onewire_close)(luat_onewire_ctx_t* ctx);

typedef struct luat_onewire_opt {
    int32_t mode;
    onewire_setup setup;
    onewire_read read;
    onewire_write write;
    onewire_close close;
}luat_onewire_opt_t;

int luat_onewire_set_hw_opt(const luat_onewire_opt_t* ctx);

int luat_onewire_ds18b20(const luat_onewire_ctx_t* ctx, int check_crc, int32_t *re);

int luat_onewire_dht(luat_onewire_ctx_t* ctx, int32_t* temp, int32_t* hm, int check_crc);

#endif
