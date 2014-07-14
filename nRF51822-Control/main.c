/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_hrs_eval_main main.c
 * @{
 * @ingroup ble_sdk_app_hrs_eval
 * @brief Main file for Heart Rate Service Sample Application for nRF51822 evaluation board
 *
 * This file contains the source code for a sample application using the Heart Rate service
 * (and also Battery and Device Information services) for the nRF51822 evaluation board (PCA10001).
 * This application uses the @ref ble_sdk_lib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include "ble_ams_c.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "led.h"
#include "device_manager.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_debug_assert_handler.h"
#include "pstorage.h"
#include "app_trace.h"
#include "ble_hci.h"




#define IS_SRVC_CHANGED_CHARACT_PRESENT      0                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define BOND_DELETE_ALL_BUTTON_ID            BUTTON_0                       /**< Button used for deleting all bonded centrals during startup. */

#define DEVICE_NAME                          "AMS"                               /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                    "Oltica"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                     40                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS           0                                        /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER                  0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS                 4                                          /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE              5                                          /**< Size of timer operation queues. */

#define BATTERY_LEVEL_MEAS_INTERVAL          APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */

#define HEART_RATE_MEAS_INTERVAL             APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER) /**< Heart rate measurement interval (ticks). */
#define MIN_HEART_RATE                       60                                         /**< Minimum heart rate as returned by the simulated measurement function. */
#define MAX_HEART_RATE                       300                                        /**< Maximum heart rate as returned by the simulated measurement function. */
#define HEART_RATE_CHANGE                    2                                          /**< Value by which the heart rate is incremented/decremented during button press. */

#define APP_GPIOTE_MAX_USERS                 1                                          /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY               APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)   /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define MESSAGE_BUFFER_SIZE             18

#define MIN_CONN_INTERVAL                    MSEC_TO_UNITS(500, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL                    MSEC_TO_UNITS(1000, UNIT_1_25_MS)          /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                        0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                     MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY        APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT         3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_TIMEOUT                    30                                         /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                       1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                       0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES            BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                        0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE               7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE               16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                            0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


static ble_ams_c_t                      m_ams_c;
static uint8_t                          m_apple_message_buffer[MESSAGE_BUFFER_SIZE];
static ble_gap_adv_params_t             m_adv_params;
static uint8_t                          m_ams_uuid_type;

static bool                                  m_memory_access_in_progress = false;       /**< Flag to keep track of ongoing operations on persistent memory. */
static dm_application_instance_t             m_app_handle;                              /**< Application identifier allocated by device manager */
static dm_handle_t                           m_peer_handle;                                       /**< Identifes the peer that is currently connected. */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt);

static void sys_evt_dispatch(uint32_t sys_evt);


/*****************************************************************************
* Error Handling Functions
*****************************************************************************/


/**@brief Function for error handling, which is called when an error has occurred. 
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // This call can be used for debug purposes during application development.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}




/*****************************************************************************
* Static Timeout Handling Functions
*****************************************************************************/

/**@brief Function for handling button events.
 *
 * @param[in]   pin_no   The pin number of the button pressed.
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    if (button_action == APP_BUTTON_PUSH)
    {
        switch (pin_no)
        {
            case BUTTON_0:
                ble_ams_send_rc_command(&m_ams_c, BLE_AMS_REMOTE_COMMAND_TOGGLE_PLAY_PAUSE);
                break;
                
            case BUTTON_1:
                ble_ams_send_rc_command(&m_ams_c, BLE_AMS_REMOTE_COMMAND_NEXT_TRACK);
                break;
                
            default:
                APP_ERROR_HANDLER(pin_no);
                break;
        }
    }    
}


/*****************************************************************************
* Static Initialization Functions
*****************************************************************************/

/**@brief Function for the Timer initialization.
 *
* @details Initializes the timer module. This creates and starts application timers.
*/
static void timers_init(void)
{
    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);

}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_WATCH);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    ble_uuid_t    ams_uuid;
    
    ams_uuid.uuid = ((ble_ams_base_uuid128.uuid128[12]) | (ble_ams_base_uuid128.uuid128[13] << 8));
    ams_uuid.type = m_ams_uuid_type;
    
    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));
    
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    advdata.uuids_complete.uuid_cnt = 0;
    advdata.uuids_complete.p_uuids  = NULL;
    advdata.uuids_solicited.uuid_cnt = 1;
    advdata.uuids_solicited.p_uuids  = &ams_uuid;
    
    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}

static void on_ams_c_evt(ble_ams_c_evt_t * p_evt)
{
    uint32_t err_code = NRF_SUCCESS;
    
    switch (p_evt->evt_type)
    {
        case BLE_AMS_C_EVT_DISCOVER_COMPLETE:
            err_code = dm_security_setup_req(&m_peer_handle);
            APP_ERROR_CHECK(err_code);
            break;
            
        default:
            //No implementation needed
            break;
    }
}

static void apple_notification_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    ble_ams_c_init_t  ams_init_obj;
    ble_uuid_t        service_uuid;
    uint32_t          err_code;
    
    err_code = sd_ble_uuid_vs_add(&ble_ams_base_uuid128, &m_ams_uuid_type);
    APP_ERROR_CHECK(err_code);
    
    err_code = sd_ble_uuid_vs_add(&ble_ams_rc_base_uuid128, &service_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    err_code = sd_ble_uuid_vs_add(&ble_ams_eu_base_uuid128, &service_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    err_code = sd_ble_uuid_vs_add(&ble_ams_ea_base_uuid128, &service_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    memset(&ams_init_obj, 0, sizeof(ams_init_obj));
    memset(m_apple_message_buffer, 0, MESSAGE_BUFFER_SIZE);
    
    ams_init_obj.evt_handler         = on_ams_c_evt;
    ams_init_obj.message_buffer_size = MESSAGE_BUFFER_SIZE;
    ams_init_obj.p_message_buffer    = m_apple_message_buffer;
    ams_init_obj.error_handler       = apple_notification_error_handler;
    
    err_code = ble_ams_c_init(&m_ams_c, &ams_init_obj);
    APP_ERROR_CHECK(err_code);
    
    err_code = ble_ams_c_service_load(&m_ams_c);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));
    
    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
                                           api_result_t           event_result)
{
    APP_ERROR_CHECK(event_result);
    ble_ams_c_on_device_manager_evt(&m_ams_c, p_handle, p_event);
    switch(p_event->event_id)
    {
        case DM_EVT_CONNECTION:
            m_peer_handle = (*p_handle);
            break;
    }
    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 */
static void device_manager_init(void)
{
    uint32_t                err_code;
    dm_init_param_t         init_data;
    dm_application_param_t  register_param;
    
    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);
    
    // Clear all bonded centrals if the "delete all bonds" button is pushed.
    err_code = app_button_is_pushed(BOND_DELETE_ALL_BUTTON_ID, &init_data.clear_persistent_data);
    APP_ERROR_CHECK(err_code);
    
    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);
    
    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));
    
    register_param.sec_param.timeout      = SEC_PARAM_TIMEOUT;
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;
    
    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    // Enable BLE stack 
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GPIOTE module.
 */
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}


/**@brief Function for initializing the button module.
 */
static void buttons_init(void)
{
    // Configure HR_INC_BUTTON_PIN_NO and HR_DEC_BUTTON_PIN_NO as wake up buttons and also configure
    // for 'pull up' because the eval board does not have external pull up resistors connected to
    // the buttons.
    static app_button_cfg_t buttons[] =
    {
        {HR_INC_BUTTON_PIN_NO, false, BUTTON_PULL, button_event_handler},
        {HR_DEC_BUTTON_PIN_NO, false, BUTTON_PULL, button_event_handler}  // Note: This pin is also BONDMNGR_DELETE_BUTTON_PIN_NO
    };
    
    APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
}


/*****************************************************************************
* Static Start Functions
*****************************************************************************/

/**@brief Function for starting the application timers.
 */
static void application_timers_start(void)
{

}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code;
    
    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    
    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    m_adv_params.p_peer_addr = NULL;                           // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    
    m_adv_params.interval = APP_ADV_INTERVAL;
    m_adv_params.timeout  = 0;
    
    err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);

    led_start();
}


/**@brief Function for putting the chip in System OFF Mode
 */
static void system_off_mode_enter(void)
{
    uint32_t err_code;
    uint32_t count;
    
    // Verify if there is any flash access pending, if yes delay starting advertising until 
    // it's complete.
    err_code = pstorage_access_status_get(&count);
    APP_ERROR_CHECK(err_code);
    
    if (count != 0)
    {
        m_memory_access_in_progress = true;
        return;
    }

    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/*****************************************************************************
* Static Event Handling Functions
*****************************************************************************/

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t        err_code = NRF_SUCCESS;
    static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            led_stop();
            err_code = app_button_enable();
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            
            // Stop detecting button presses when not connected.
            err_code = app_button_disable();
            APP_ERROR_CHECK(err_code);
            
            err_code = ble_ams_c_service_store();
            APP_ERROR_CHECK(err_code);
            
            advertising_start();
            break;
            
        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
            {
                advertising_start();
            }
            break;
            
        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server and Client timeout events.
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
            
        default:
            // No implementation needed.
            break;
    }
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_ams_c_on_ble_evt(&m_ams_c, p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
}


/*****************************************************************************
* Main Function
*****************************************************************************/

/**@brief Function for the application main entry.
 */
int main(void)
{
    uint32_t err_code;

    timers_init();
    gpiote_init();
    buttons_init();
    ble_stack_init();
    device_manager_init();

    // Initialize Bluetooth Stack parameters.
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();

    // Start advertising.
    advertising_start();

    // Enter main loop.
    for (;;)
    {
        // Switch to a low power state until an event is available for the application
        err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);
    }
}

/**
 * @}
 */
