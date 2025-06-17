#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(minp, LOG_LEVEL_INF);

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


static const struct bt_le_adv_param *adv_param =
    BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use
                                                                          identity address */
                    800,   /* Min Advertising Interval 500ms (800*0.625ms), upto 16383 */
                    801,   /* Max Advertising Interval 500.625ms (801*0.625ms), upto 16384 */
                    NULL); /* Set to NULL for undirected advertising */

static struct k_work adv_work;
static struct bt_conn *my_conn = NULL;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {};

static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
    } else {
        LOG_INF("Advertising successfully started");
    }
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void recycled_cb(void)
{
    LOG_INF("Connection object recycled. Restarting advertising.");
    if (my_conn) {
        bt_conn_unref(my_conn);
        my_conn = NULL;
    }
    advertising_start();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }
    LOG_INF("Connected");
    my_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);
    if (my_conn) {
        bt_conn_unref(my_conn);
        my_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled_cb,
};

void main(void)
{
    int err;
    int blink_status = 0;

    LOG_INF("Starting minimal BLE peripheral");

    if (err) {
        LOG_ERR("LEDs init failed (err %d)", err);
        return;
    }

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();
    int cnt = 0;
    while (1) {
        LOG_INF("MAIN ALIVE%d", cnt++);
        k_sleep(K_MSEC(1000));
    }
}
